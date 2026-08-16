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
template<typename T> size_t TypeId() {
	static size_t id = nextTypeId++;
	return id;
}
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
		if (id >= entity_index.size())
			entity_index.resize(id + 1);

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

	template<typename T> void RemoveComponent(tgl::EntityId e) {
		tgl::LocalEntityId id = e.id;
		if (id >= entity_index.size())
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

	void RemoveEntityData(tgl::EntityId e) {
		tgl::LocalEntityId id = e.id;
		if (id >= entity_index.size())
			return;

		EntityRecord &rec = entity_index[id];
		if (!rec.table)
			return;

		auto moved = rec.table->RemoveRow(rec.row);
		if (moved)
			entity_index[*moved] = {rec.table, rec.row};

		rec = EntityRecord{};
	}

	template<typename T> T *GetComponent(tgl::EntityId e) const {
		tgl::LocalEntityId id = e.id;
		if (id >= entity_index.size())
			return nullptr;

		const EntityRecord &rec = entity_index[id];
		if (!rec.table)
			return nullptr;

		Column<T> *col = rec.table->template GetColumn<T>();
		return col ? col->Get(rec.row) : nullptr;
	}

	// Whether e's archetype table contains ALL of Components - a single
	// bitset comparison against the table's own signature, regardless of how
	// many components are queried, instead of one GetComponent lookup per
	// component.
	template<typename... Components> bool HasComponents(tgl::EntityId e) const {
		tgl::LocalEntityId id = e.id;
		if (id >= entity_index.size())
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
	struct EntityRecord {
		ArchetypeTable *table = nullptr;
		size_t row = 0;
	};

	SceneId scene_id = 0;
	std::vector<EntityRecord> entity_index;
	std::vector<std::unique_ptr<ArchetypeTable>> tables;

	ArchetypeTable *GetOrCreateTable(Signature sig) {
		for (auto &table : tables)
			if (table->GetSignature() == sig)
				return table.get();
		tables.emplace_back(std::make_unique<ArchetypeTable>(sig));
		return tables.back().get();
	}
};
