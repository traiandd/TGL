#pragma once

#include <array>
#include <bit>
#include <bitset>
#include <cassert>
#include <memory>
#include <optional>
#include <vector>

#include "entity_id.hpp"
#include "glm/fwd.hpp"

namespace tgl {

class EntityInstance;
class Scene;
template<typename... Components> class Archetype;
}; // namespace tgl

// A program-wide dense index per component type, used to build Signatures
// below. Not related to tgl::Archetype (the compile-time component-set
// *view* used by callers) - this is the storage side: which entities live
// in which ArchetypeTable.
constexpr size_t kMaxComponentTypes = 64;
using Signature = std::bitset<kMaxComponentTypes>;

namespace component_data_detail {
inline size_t nextTypeId = 0;

// Assigned eagerly, once, at program startup - an inline variable template
// initializes via ordinary static initialization rather than a
// lazily-guarded function-local static ("magic static"), so reading it is a
// bare load with no runtime guard check. The trade: unlike a magic static,
// nothing protects a read that happens before this specific slot's own
// initializer has run - if some other global's constructor reached
// TypeId<T>() during static init, before T's slot initialized (init order
// across translation units is unspecified), it would silently read 0
// instead of the real id. Nothing in this engine touches ComponentData
// from global constructors today; keep it that way.
template<typename T> inline size_t component_type_id = nextTypeId++;

template<typename T> size_t TypeId() { return component_type_id<T>; }

} // namespace component_data_detail
using component_data_detail::TypeId;

template<typename... Components> Signature MakeSignature() {
	static Signature sig = [] {
		Signature s;
		(s.set(TypeId<Components>()), ...);
		return s;
	}();
	return sig;
}

// Calls f(index) for every set bit in sig, jumping straight to each one
// instead of testing all kMaxComponentTypes bits - used to walk exactly a
// table's real columns (its signature already records which are populated)
// rather than scanning the full column array.
template<typename Func> void ForEachSetBit(Signature sig, Func f) {
	static_assert(kMaxComponentTypes <= 64);
	unsigned long long bits = sig.to_ullong();
	while (bits) {
		f(static_cast<size_t>(std::countr_zero(bits)));
		bits &= bits - 1; // clear the lowest set bit
	}
}

// Type-erased column of one component type within an ArchetypeTable. Rows
// across all of a table's columns are kept in lockstep by construction: a
// row is only ever appended to a table by filling every column for it in
// one go (see ComponentData::AddComponent/RemoveComponent), and removed via
// RemoveRow, which swap-removes every column at once.
struct IColumn {
	virtual ~IColumn() = default;
	virtual void SwapRemove(size_t row) = 0;
	virtual void MoveFrom(IColumn &other, size_t other_row) = 0;
	virtual std::unique_ptr<IColumn> CreateEmptyLike() const = 0;
};

template<typename T> class Column : public IColumn {
  public:
	void PushBack(T value) { data.push_back(std::move(value)); }

	void SwapRemove(size_t row) override {
		if (row != data.size() - 1)
			data[row] = std::move(data.back());
		data.pop_back();
	}

	void MoveFrom(IColumn &other, size_t other_row) override {
		auto &src = static_cast<Column<T> &>(other);
		data.push_back(std::move(src.data[other_row]));
	}

	std::unique_ptr<IColumn> CreateEmptyLike() const override { return std::make_unique<Column<T>>(); }

	T *Get(size_t row) { return &data[row]; }

  private:
	std::vector<T> data;
};

// All entities with the exact same set of components
// live here, one row per entity, one column per component type - the
// actual archetype/table storage. tgl::Archetype<Components...> (a view
// over a single entity) is built from row data found by scanning tables
// whose signature is a superset of the query.
class ArchetypeTable {
  public:
	explicit ArchetypeTable(Signature sig) : signature(sig) {}

	template<typename T> void EnsureColumn() {
		size_t id = TypeId<T>();
		if (!columns[id])
			columns[id] = std::make_unique<Column<T>>();
	}

	template<typename T> Column<T> *GetColumn() { return static_cast<Column<T> *>(columns[TypeId<T>()].get()); }

	IColumn *GetOrCreateColumnLike(size_t type_id, const IColumn &example) {
		if (!columns[type_id])
			columns[type_id] = example.CreateEmptyLike();
		return columns[type_id].get();
	}

	std::array<std::unique_ptr<IColumn>, kMaxComponentTypes> &Columns() { return columns; }

	size_t AppendEntity(tgl::LocalEntityId entity) {
		row_to_entity.push_back(entity);
		return row_to_entity.size() - 1;
	}

	// Swap-removes `row` from every column and from the row/entity mapping.
	// Returns the entity that got moved into `row` from the back, if any -
	// the caller must update that entity's index entry to point at `row`.
	std::optional<tgl::LocalEntityId> RemoveRow(size_t row) {
		ForEachSetBit(signature, [&](size_t id) { columns[id]->SwapRemove(row); });

		size_t last = row_to_entity.size() - 1;
		if (row == last) {
			row_to_entity.pop_back();
			return std::nullopt;
		}
		tgl::LocalEntityId moved = row_to_entity[last];
		row_to_entity[row] = moved;
		row_to_entity.pop_back();
		return moved;
	}

	tgl::LocalEntityId EntityAt(size_t row) const { return row_to_entity[row]; }
	size_t Size() const { return row_to_entity.size(); }
	Signature GetSignature() const { return signature; }

  private:
	Signature signature;
	std::array<std::unique_ptr<IColumn>, kMaxComponentTypes> columns;
	std::vector<tgl::LocalEntityId> row_to_entity;
};

class ComponentData {
  public:
	ComponentData(SceneId id = 0) : scene_id(id) {}

	ComponentData(ComponentData &&) = default;

	void SetSceneId(SceneId id) { scene_id = id; }

	template<typename T> void AddComponent(tgl::EntityId e, T component) {
		tgl::LocalEntityId id = e.id;
		EnsureEntityIndexCapacity(id);

		EntityRecord &rec = entity_index[id];
		ArchetypeTable *old_table = rec.table;
		size_t type_id = TypeId<T>();

		if (old_table && old_table->GetSignature().test(type_id)) {
			// Entity already has T: overwrite in place, no table transition.
			*old_table->template GetColumn<T>()->Get(rec.row) = std::move(component);
			return;
		}

		size_t old_row = rec.row;
		Signature new_sig = old_table ? old_table->GetSignature() : Signature{};
		new_sig.set(type_id);

		ArchetypeTable *new_table = GetOrCreateTable(new_sig);
		new_table->template EnsureColumn<T>();

		if (old_table) {
			auto &old_columns = old_table->Columns();
			ForEachSetBit(old_table->GetSignature(), [&](size_t tid) { new_table->GetOrCreateColumnLike(tid, *old_columns[tid])->MoveFrom(*old_columns[tid], old_row); });
		}
		new_table->template GetColumn<T>()->PushBack(std::move(component));
		size_t new_row = new_table->AppendEntity(id);

		if (old_table) {
			auto moved = old_table->RemoveRow(old_row);
			if (moved)
				entity_index[*moved] = {old_table, old_row};
		}

		rec = {new_table, new_row};
	}

	// Inserts a brand-new entity with its full component set known up front,
	// straight into its final ArchetypeTable - unlike AddComponent, which
	// transitions one table per call, this never touches an intermediate
	// table. Only valid for an entity id that has no existing record.
	template<typename... Components> void CreateEntity(tgl::EntityId e, Components... components) {
		tgl::LocalEntityId id = e.id;
		EnsureEntityIndexCapacity(id);

		Signature sig = MakeSignature<Components...>();
		ArchetypeTable *table = GetOrCreateTable(sig);
		(table->template EnsureColumn<Components>(), ...);
		(table->template GetColumn<Components>()->PushBack(std::move(components)), ...);
		size_t row = table->AppendEntity(id);
		entity_index[id] = {table, row};
	}

	template<typename T> void RemoveComponent(tgl::EntityId e) {
		tgl::LocalEntityId id = e.id;
		if (id >= entity_index_size_)
			return;

		EntityRecord &rec = entity_index[id];
		ArchetypeTable *old_table = rec.table;
		size_t type_id = TypeId<T>();
		if (!old_table || !old_table->GetSignature().test(type_id))
			return;

		size_t old_row = rec.row;
		Signature new_sig = old_table->GetSignature();
		new_sig.reset(type_id);

		ArchetypeTable *new_table = GetOrCreateTable(new_sig);
		auto &old_columns = old_table->Columns();
		// new_sig is old_table's signature with type_id already cleared, so
		// its set bits are exactly the columns to carry over.
		ForEachSetBit(new_sig, [&](size_t tid) { new_table->GetOrCreateColumnLike(tid, *old_columns[tid])->MoveFrom(*old_columns[tid], old_row); });
		size_t new_row = new_table->AppendEntity(id);

		auto moved = old_table->RemoveRow(old_row);
		if (moved)
			entity_index[*moved] = {old_table, old_row};

		rec = {new_table, new_row};
	}

	// What Locate() resolves an EntityId to - same fields as the private
	// EntityRecord below, exposed as their own tiny type so a caller (e.g.
	// tgl::ArchetypeData) can cache them without depending on ComponentData's
	// private storage layout.
	struct EntityLocation {
		ArchetypeTable *table = nullptr;
		size_t row = 0;
	};

	// Resolves e once to its current table+row - the same lookup
	// GetComponent/GetComponentUnchecked repeat on every call, exposed
	// directly so a caller doing several column lookups on the same entity
	// (e.g. one Archetype's several Get<T>() calls) can resolve once at
	// construction and reuse it, instead of re-deriving it - and re-resolving
	// the entity's Scene through SceneManager - on every single T. Same
	// contract as GetComponentUnchecked: only valid for an id whose entity is
	// known (dynamically) to exist.
	EntityLocation Locate(tgl::EntityId e) const {
		tgl::LocalEntityId id = e.id;
		assert(id < entity_index_size_);

		const EntityRecord &rec = entity_index[id];
		assert(rec.table);
		return {rec.table, rec.row};
	}

	template<typename T> T *GetComponent(tgl::EntityId e) const {
		tgl::LocalEntityId id = e.id;
		if (id >= entity_index_size_)
			return nullptr;

		const EntityRecord &rec = entity_index[id];
		if (!rec.table)
			return nullptr;

		Column<T> *col = rec.table->template GetColumn<T>();
		return col ? col->Get(rec.row) : nullptr;
	}

	// Skips every check GetComponent has to make, on the assumption that the
	// caller already knows e currently has T - true for any Archetype<...>
	// obtained through ForEach, whose own signature-vs-query check is what
	// establishes the guarantee (see ForEach below). Not for general use: an
	// id that was never created, or an entity missing T, is UB here instead
	// of a graceful nullptr - the asserts catch that in debug builds, but
	// NDEBUG builds trust the caller completely.
	template<typename T> T &GetComponentUnchecked(tgl::EntityId e) const {
		tgl::LocalEntityId id = e.id;
		assert(id < entity_index_size_);

		const EntityRecord &rec = entity_index[id];
		assert(rec.table);

		Column<T> *col = rec.table->template GetColumn<T>();
		assert(col);
		return *col->Get(rec.row);
	}

	// Whether e's archetype table contains ALL of Components - a single
	// bitset comparison against the table's own signature, regardless of how
	// many components are queried, instead of one GetComponent lookup per
	// component.
	template<typename... Components> bool HasComponents(tgl::EntityId e) const {
		tgl::LocalEntityId id = e.id;
		if (id >= entity_index_size_)
			return false;

		const EntityRecord &rec = entity_index[id];
		if (!rec.table)
			return false;

		Signature query = MakeSignature<Components...>();
		return (rec.table->GetSignature() & query) == query;
	}

	// Visits every entity whose archetype table contains ALL of Components
	// (it may have others besides). Iterates only matching tables' rows -
	// no scanning over entities that lack the queried components.
	template<typename... Components, typename Func> void ForEach(Func f) {
		Signature query = MakeSignature<Components...>();
		for (auto &table : tables) {
			if ((table->GetSignature() & query) != query)
				continue;
			for (size_t row = 0; row < table->Size(); row++)
				f(tgl::Archetype<Components...>(tgl::EntityId(table->EntityAt(row), scene_id)));
		}
	}

	// Number of distinct archetype tables currently in use (not component
	// type count, since a type can appear across many tables).
	size_t size() { return tables.size(); }

  private:
	// Not public: destroying an entity's row mid-frame (e.g. from inside a
	// ForEach callback) can swap another entity into the current row and
	// desync that ForEach's iteration (see ForEach's row < table->Size()
	// loop above). Scene::DeleteEntity queues instead, and
	// Scene::FlushDeleteQueue - the only caller of this - runs once per
	// frame, after systems have finished iterating.
	friend class tgl::Scene;

	void RemoveEntityData(tgl::EntityId e) {
		tgl::LocalEntityId id = e.id;
		if (id >= entity_index_size_)
			return;

		EntityRecord &rec = entity_index[id];
		if (!rec.table)
			return;

		auto moved = rec.table->RemoveRow(rec.row);
		if (moved)
			entity_index[*moved] = {rec.table, rec.row};

		rec = EntityRecord{};
	}

	struct EntityRecord {
		ArchetypeTable *table = nullptr;
		size_t row = 0;
	};

	SceneId scene_id = 0;
	// Mirrors entity_index.size() - std::vector<T>::size() computes
	// (end-begin)/sizeof(T) on every call (visible as extra load+sub+shr in
	// GetComponent's disassembly); every bounds check here instead reads
	// this directly, and only EnsureEntityIndexCapacity (the sole place
	// entity_index grows) needs to keep it in sync.
	size_t entity_index_size_ = 0;
	std::vector<EntityRecord> entity_index;
	std::vector<std::unique_ptr<ArchetypeTable>> tables;

	void EnsureEntityIndexCapacity(tgl::LocalEntityId id) {
		if (id >= entity_index_size_) {
			entity_index.resize(id + 1);
			entity_index_size_ = id + 1;
		}
	}

	ArchetypeTable *GetOrCreateTable(Signature sig) {
		for (auto &table : tables)
			if (table->GetSignature() == sig)
				return table.get();
		tables.emplace_back(std::make_unique<ArchetypeTable>(sig));
		return tables.back().get();
	}
};
