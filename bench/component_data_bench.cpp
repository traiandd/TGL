#define ANKERL_NANOBENCH_IMPLEMENT
#include "nanobench/nanobench.h"

#include "implementations/current/component_data.hpp"
#include "implementations/dense_array/component_data.hpp"
#include "implementations/legacy/component_data.hpp"

#include "flecs/flecs.h"
#include "entt/entt.hpp"
#include "gaia/gaia.h"

#include "tgl/scene.hpp"
#include "tgl/scene_manager.hpp"

#include <algorithm>
#include <cstdint>
#include <random>

// See bench/implementations/ for what's being compared. GetComponent/
// AddComponent benchmark bare ComponentData instances with fabricated
// EntityIds; ForEach needs a real Scene registered through SceneManager
// (see the ForEach block in main() for why).

using tgl::EntityId;

namespace {
struct Position {
	float x, y, z;
};
struct Velocity {
	float dx, dy, dz;
};
struct Health {
	int hp;
};
} // namespace

// Archetype<T> requires an ArchetypeExtender<T, Derived> specialization for
// every component type it's instantiated with (see tgl/archetype.hpp);
// empty ones are enough here since ForEach just needs Archetype to build.
namespace tgl {
template<typename Derived> class ArchetypeExtender<Position, Derived> {};
template<typename Derived> class ArchetypeExtender<Velocity, Derived> {};
template<typename Derived> class ArchetypeExtender<Health, Derived> {};
} // namespace tgl

namespace {
constexpr int kEntities = 10000;

template<typename Data> void Populate(Data &data, SceneId scene_id = 1) {
	for (int i = 0; i < kEntities; i++) {
		EntityId e{static_cast<uint32_t>(i), scene_id};
		data.AddComponent(e, Position{1, 2, 3});
		data.AddComponent(e, Velocity{0.1f, 0.2f, 0.3f});
		data.AddComponent(e, Health{100});
	}
}

// Half the entities get both Position and Velocity, a quarter get only
// Position, a quarter get only Velocity - for the ForEach<Position, Velocity>
// benchmark below, which needs a mix of matching/non-matching entities to be
// a meaningful query rather than a full-table scan.
template<typename Data> void PopulateMixed(Data &data, SceneId scene_id = 1) {
	for (int i = 0; i < kEntities; i++) {
		EntityId e{static_cast<uint32_t>(i), scene_id};
		int bucket = i % 4;
		if (bucket < 2) {
			data.AddComponent(e, Position{1, 2, 3});
			data.AddComponent(e, Velocity{0.1f, 0.2f, 0.3f});
		} else if (bucket == 2) {
			data.AddComponent(e, Position{1, 2, 3});
		} else {
			data.AddComponent(e, Velocity{0.1f, 0.2f, 0.3f});
		}
	}
}

// Heavy/realistic stress test for GetComponent. Populate/PopulateMixed above
// put every entity into one of at most 4 archetype tables total, so after
// the first few calls, GetComponent's rec.table/columns/data pointers are
// always the same hot cache lines - that's why its miss% is always 0.0% in
// the benchmark below, however many entities there are.
//
// This section instead builds kNumArchetypes (hundreds) of distinct
// component combinations from a shared pool, and shuffles which entity gets
// which one. That's not meant to look like named game entity kinds - it's
// modeling how a real game actually ends up with hundreds of distinct
// ArchetypeTables over time, as differently-shaped entities are created and
// as components get added/removed at runtime (flecs' own docs discuss this
// under "table fragmentation"). With that many distinct, separately
// heap-allocated tables/columns in play, consecutive GetComponent calls in
// the loop below land in a different one almost every time - the scenario
// the small benchmark structurally can't reach.
struct Mana {
	float value, regen;
};
struct Armor {
	float value;
};
struct Damage {
	float amount;
};
struct Sprite {
	int texture_id;
	float u, v;
};
struct AIState {
	int state;
	float timer;
};
struct Inventory {
	int slots[4];
};
struct Collider {
	float radius;
};
struct Rotation {
	float yaw, pitch, roll;
};
struct Scale {
	float x, y, z;
};
struct Lifetime {
	float remaining;
};
struct Team {
	int id;
};
struct StatusEffect {
	int flags;
};

// A genuinely large component (40,000 bytes) - unlike the structs above,
// touching one guarantees real memory traffic rather than a single cache
// line, since it's ~625x the size of one. Only a bounded subset of entities
// get one (see kBigDataEntities) - giving every one of millions of entities
// a component this size would run into hundreds of GB, not the few GB this
// machine can actually spare.
struct BigData {
	int values[10000];
};

constexpr int kHeavyEntities = 2000000;
constexpr int kBigDataEntities = 25000; // ~1GB of BigData per implementation
constexpr int kNumArchetypes = 500;
constexpr int kNumOptionalComponents = 14;

// Which of the 14 optional component types archetype `i` includes - fixed
// once at startup so every ComponentData implementation populates with the
// exact same set of archetypes.
struct ArchetypeMask {
	bool has[kNumOptionalComponents];
};

std::vector<ArchetypeMask> BuildArchetypeMasks() {
	std::vector<ArchetypeMask> masks(kNumArchetypes);
	std::mt19937 rng(999); // fixed seed: reproducible across runs
	std::bernoulli_distribution coin(0.35);
	for (auto &m : masks)
		for (bool &b : m.has)
			b = coin(rng);
	return masks;
}

template<typename Data> void PopulateArchetype(Data &data, const ArchetypeMask &mask, EntityId e) {
	data.AddComponent(e, Position{1, 2, 3}); // always present, so GetComponent<Position> always succeeds
	if (mask.has[0])
		data.AddComponent(e, Velocity{0.1f, 0.2f, 0.3f});
	if (mask.has[1])
		data.AddComponent(e, Health{100});
	if (mask.has[2])
		data.AddComponent(e, Mana{50, 1.f});
	if (mask.has[3])
		data.AddComponent(e, Armor{10});
	if (mask.has[4])
		data.AddComponent(e, Damage{10});
	if (mask.has[5])
		data.AddComponent(e, Sprite{0, 0, 0});
	if (mask.has[6])
		data.AddComponent(e, AIState{0, 0.f});
	if (mask.has[7])
		data.AddComponent(e, Inventory{{0, 0, 0, 0}});
	if (mask.has[8])
		data.AddComponent(e, Collider{1.f});
	if (mask.has[9])
		data.AddComponent(e, Rotation{0, 0, 0});
	if (mask.has[10])
		data.AddComponent(e, Scale{1, 1, 1});
	if (mask.has[11])
		data.AddComponent(e, Lifetime{1.f});
	if (mask.has[12])
		data.AddComponent(e, Team{1});
	if (mask.has[13])
		data.AddComponent(e, StatusEffect{0});
}

template<typename Data> void PopulateHeavy(Data &data, SceneId scene_id = 1) {
	static const std::vector<ArchetypeMask> masks = BuildArchetypeMasks();

	std::vector<int> archetype_of(kHeavyEntities);
	for (int i = 0; i < kHeavyEntities; i++)
		archetype_of[i] = i % kNumArchetypes;
	std::mt19937 rng(12345); // fixed seed: reproducible across runs
	std::shuffle(archetype_of.begin(), archetype_of.end(), rng);

	for (int i = 0; i < kHeavyEntities; i++) {
		EntityId e{static_cast<uint32_t>(i), scene_id};
		PopulateArchetype(data, masks[archetype_of[i]], e);
	}
}

// Separate from PopulateHeavy deliberately: this used to add BigData onto
// PopulateHeavy's own entities, but PopulateHeavy is called once per
// implementation by *both* RunGetComponentHeavy and RunGetComponentBigData
// - that silently populated BigData 6 times over instead of 3, and since
// each BigData allocation is well under glibc's mmap threshold, the freed
// heap from an earlier populate call doesn't reliably get returned to the
// OS before the next one runs. That's what drove this benchmark process to
// get OOM-killed. Keeping BigData in its own minimal population (only
// kBigDataEntities entities, populated once per implementation) bounds the
// worst case to about kBigDataEntities * sizeof(BigData) * 3 implementations.
template<typename Data> void PopulateBigData(Data &data, SceneId scene_id = 1) {
	for (int i = 0; i < kBigDataEntities; i++) {
		EntityId e{static_cast<uint32_t>(i), scene_id};
		data.AddComponent(e, BigData{});
	}
}

template<typename Data> void RunGetComponentHeavy(ankerl::nanobench::Bench &bench, const char *name) {
	Data data;
	PopulateHeavy(data);

	bench.run(name, [&] {
		for (int i = 0; i < kHeavyEntities; i++) {
			EntityId e{static_cast<uint32_t>(i), 1};
			ankerl::nanobench::doNotOptimizeAway(data.template GetComponent<Position>(e));
		}
	});
}

// Deliberately does NOT read BigData's contents, only fetches the pointer -
// GetComponent computes column->data() + row * sizeof(T), pure address
// arithmetic that never touches the component's bytes, so its cost should
// be independent of sizeof(T). Walking the array's cache lines here would
// measure memory bandwidth, not GetComponent - swamping the comparison
// entirely (every implementation converged to ~1.7us regardless of lookup
// strategy, which was the tell that this was testing the wrong thing).
template<typename Data> void RunGetComponentBigData(ankerl::nanobench::Bench &bench, const char *name) {
	Data data;
	PopulateBigData(data);

	bench.run(name, [&] {
		for (int i = 0; i < kBigDataEntities; i++) {
			EntityId e{static_cast<uint32_t>(i), 1};
			ankerl::nanobench::doNotOptimizeAway(data.template GetComponent<BigData>(e));
		}
	});
}

template<typename Data> void RunGetComponent(ankerl::nanobench::Bench &bench, const char *name) {
	Data data;
	Populate(data);

	bench.run(name, [&] {
		for (int i = 0; i < kEntities; i++) {
			EntityId e{static_cast<uint32_t>(i), 1};
			ankerl::nanobench::doNotOptimizeAway(data.template GetComponent<Position>(e));
			ankerl::nanobench::doNotOptimizeAway(data.template GetComponent<Velocity>(e));
			ankerl::nanobench::doNotOptimizeAway(data.template GetComponent<Health>(e));
		}
	});
}

void RunGetComponentChecked(ankerl::nanobench::Bench &bench, const char *name, ComponentData &data, SceneId scene_id) {
	bench.run(name, [&] {
		for (int i = 0; i < kEntities; i++) {
			EntityId e{static_cast<uint32_t>(i), scene_id};
			ankerl::nanobench::doNotOptimizeAway(data.GetComponent<Position>(e));
			ankerl::nanobench::doNotOptimizeAway(data.GetComponent<Velocity>(e));
			ankerl::nanobench::doNotOptimizeAway(data.GetComponent<Health>(e));
		}
	});
}

// Archetype<T...> resolves e's table+row exactly once, at construction (see
// ArchetypeData/ComponentData::Locate in tgl/archetype.hpp,
// tgl/component_data.hpp) - Get<T>() then reads straight off that cached
// table/row, with no further SceneManager/entity_index lookup at all.
//
// An earlier version of this benchmark had Get<T>() re-resolve
// GetSceneData() -> GetScene() -> SceneManager::Get() independently on every
// single call (three times per entity here) - that hit a function-local
// static's guard check (see scene_manager.cpp) three times over and came out
// slower than the checked path below despite skipping its null checks. The
// checks were never the bottleneck; the redundant per-call SceneManager
// indirection was - this version removes that instead of trying to out-run
// it with fewer branches.
//
// Construction (which is what still resolves through SceneManager, once per
// entity) happens up front, outside bench.run - same reason Populate() runs
// before RunGetComponent's own bench.run above. Timing construction here
// would measure Locate()/SceneManager::Get(), not Get<T>() itself.
void RunArchetypeGetComponent(ankerl::nanobench::Bench &bench, const char *name, SceneId scene_id) {
	std::vector<tgl::Archetype<Position, Velocity, Health>> archetypes;
	archetypes.reserve(kEntities);
	for (int i = 0; i < kEntities; i++)
		archetypes.emplace_back(tgl::EntityInstance(tgl::EntityId{static_cast<uint32_t>(i), scene_id}));

	bench.run(name, [&] {
		for (auto &arche : archetypes) {
			ankerl::nanobench::doNotOptimizeAway(arche.Get<Position>());
			ankerl::nanobench::doNotOptimizeAway(arche.Get<Velocity>());
			ankerl::nanobench::doNotOptimizeAway(arche.Get<Health>());
		}
	});
}

template<typename Data> void RunAddComponent(ankerl::nanobench::Bench &bench, const char *name) {
	bench.run(name, [&] {
		Data data;
		Populate(data);
		ankerl::nanobench::doNotOptimizeAway(data);
	});
}

// Same reasoning as RunAddComponentEntt below: bench_impl::entt_ecs::
// ComponentData's GetComponent also pays an extra vector-bounds-check and
// entt::null comparison that only exist to serve the wrapper's own
// EnsureEntity bookkeeping, not something a real EnTT caller holding a
// std::vector<entt::entity> would pay. Entities are created/populated up
// front, outside bench.run, same reason RunGetComponentFlecs does - this
// should measure try_get<T>() itself, not entity creation.
void RunGetComponentEntt(ankerl::nanobench::Bench &bench, const char *name) {
	entt::registry registry;
	std::vector<entt::entity> entities;
	entities.reserve(kEntities);
	for (int i = 0; i < kEntities; i++) {
		auto entity = registry.create();
		registry.emplace<Position>(entity, 1.f, 2.f, 3.f);
		registry.emplace<Velocity>(entity, 0.1f, 0.2f, 0.3f);
		registry.emplace<Health>(entity, 100);
		entities.push_back(entity);
	}

	bench.run(name, [&] {
		for (auto entity : entities) {
			ankerl::nanobench::doNotOptimizeAway(registry.try_get<Position>(entity));
			ankerl::nanobench::doNotOptimizeAway(registry.try_get<Velocity>(entity));
			ankerl::nanobench::doNotOptimizeAway(registry.try_get<Health>(entity));
		}
	});
}

// Native EnTT usage, not routed through bench_impl::entt_ecs::ComponentData -
// that wrapper's AddComponent pays for EnsureEntity's bounds-check/entt::null
// test and an unordered_map insert on every call, entirely to let ForEach
// reconstruct a tgl::EntityId afterwards (see the wrapper's own comment).
// None of that is what a real EnTT program pays to create an entity and
// emplace components onto it, so timing it through the wrapper would
// overstate EnTT's real AddComponent cost. This instead does exactly what
// EnTT's own docs show: registry.create() then registry.emplace<T>() per
// component, directly.
void RunAddComponentEntt(ankerl::nanobench::Bench &bench, const char *name) {
	bench.run(name, [&] {
		entt::registry registry;
		for (int i = 0; i < kEntities; i++) {
			auto entity = registry.create();
			registry.emplace<Position>(entity, 1.f, 2.f, 3.f);
			registry.emplace<Velocity>(entity, 0.1f, 0.2f, 0.3f);
			registry.emplace<Health>(entity, 100);
		}
		ankerl::nanobench::doNotOptimizeAway(registry);
	});
}

// Native flecs usage - flecs::world::entity()/entity::set<T>() directly, no
// wrapper class in between. flecs::entity is already the handle a real flecs
// program would hold onto and call set()/try_get() on directly, so there's
// no translation layer here to accidentally tax the way entt's ForEach
// wrapper did (see RunAddComponentEntt's comment).
void RunAddComponentFlecs(ankerl::nanobench::Bench &bench, const char *name) {
	bench.run(name, [&] {
		flecs::world world;
		for (int i = 0; i < kEntities; i++) {
			flecs::entity entity = world.entity();
			entity.set<Position>({1, 2, 3});
			entity.set<Velocity>({0.1f, 0.2f, 0.3f});
			entity.set<Health>({100});
		}
		ankerl::nanobench::doNotOptimizeAway(world);
	});
}

// ecs_bulk_init's intended fast path: creates all kEntities entities directly
// into a single, already-known table in one call, instead of each entity
// making 3 separate archetype-table moves the way RunAddComponentFlecs's
// chained set<T>() calls do (see that function's comment). This is what
// flecs itself recommends for populating many identically-shaped entities at
// once - the fair "flecs at its best" number for this scenario.
void RunAddComponentFlecsBulk(ankerl::nanobench::Bench &bench, const char *name) {
	bench.run(name, [&] {
		flecs::world world;

		// world.component<T>() registers T (if not already) and returns its
		// flecs::id_t - ecs_bulk_init needs these ahead of time since it
		// writes straight into the target table's columns, not through
		// set<T>()'s own registration path.
		ecs_id_t position_id = world.component<Position>();
		ecs_id_t velocity_id = world.component<Velocity>();
		ecs_id_t health_id = world.component<Health>();

		std::vector<Position> positions(kEntities, Position{1, 2, 3});
		std::vector<Velocity> velocities(kEntities, Velocity{0.1f, 0.2f, 0.3f});
		std::vector<Health> healths(kEntities, Health{100});
		void *data[3] = {positions.data(), velocities.data(), healths.data()};

		ecs_bulk_desc_t desc{};
		desc.count = kEntities;
		desc.ids[0] = position_id;
		desc.ids[1] = velocity_id;
		desc.ids[2] = health_id;
		desc.data = data;

		ecs_bulk_init(world, &desc);
		ankerl::nanobench::doNotOptimizeAway(world);
	});
}

// Entities are created and populated up front, outside bench.run - same
// reason RunGetComponent/RunArchetypeGetComponent set up before timing: this
// should measure try_get<T>() itself, not entity creation.
void RunGetComponentFlecs(ankerl::nanobench::Bench &bench, const char *name) {
	flecs::world world;
	std::vector<flecs::entity> entities;
	entities.reserve(kEntities);
	for (int i = 0; i < kEntities; i++) {
		flecs::entity entity = world.entity();
		entity.set<Position>({1, 2, 3});
		entity.set<Velocity>({0.1f, 0.2f, 0.3f});
		entity.set<Health>({100});
		entities.push_back(entity);
	}

	bench.run(name, [&] {
		for (auto &entity : entities) {
			ankerl::nanobench::doNotOptimizeAway(entity.try_get<Position>());
			ankerl::nanobench::doNotOptimizeAway(entity.try_get<Velocity>());
			ankerl::nanobench::doNotOptimizeAway(entity.try_get<Health>());
		}
	});
}

// Native gaia-ecs usage - gaia::ecs::World::add<T>()/get<T>() directly, no
// wrapper. gaia is explicitly performance/iteration-speed-oriented and
// archetype/chunk-based like TGL and flecs; its own README documents the
// same "one archetype move per structural change" cost model flecs has (see
// its "Bulk editing" section) - so, having already been burned once by not
// doing this for flecs, both this naive per-entity-per-add version and
// gaia's own batched-creation API (copy_n, see RunAddComponentGaiaBulk) are
// benchmarked from the start rather than as an afterthought.
void RunAddComponentGaia(ankerl::nanobench::Bench &bench, const char *name) {
	bench.run(name, [&] {
		gaia::ecs::World world;
		for (int i = 0; i < kEntities; i++) {
			gaia::ecs::Entity entity = world.add();
			world.add<Position>(entity, {1, 2, 3});
			world.add<Velocity>(entity, {0.1f, 0.2f, 0.3f});
			world.add<Health>(entity, {100});
		}
		ankerl::nanobench::doNotOptimizeAway(world);
	});
}

// gaia's own documented fast path for creating many identically-shaped
// entities: build one entity with the full archetype once, then copy_n()
// duplicates it directly into the target archetype's chunk storage, without
// repeating the per-component archetype moves RunAddComponentGaia pays for
// on every single entity.
void RunAddComponentGaiaBulk(ankerl::nanobench::Bench &bench, const char *name) {
	bench.run(name, [&] {
		gaia::ecs::World world;
		gaia::ecs::Entity entity = world.add();
		world.add<Position>(entity, {1, 2, 3});
		world.add<Velocity>(entity, {0.1f, 0.2f, 0.3f});
		world.add<Health>(entity, {100});
		world.copy_n(entity, kEntities - 1);
		ankerl::nanobench::doNotOptimizeAway(world);
	});
}

// Entities are created via the same batched copy_n() path up front, outside
// bench.run - this should measure get<T>() itself, not entity creation.
void RunGetComponentGaia(ankerl::nanobench::Bench &bench, const char *name) {
	gaia::ecs::World world;
	gaia::ecs::Entity first = world.add();
	world.add<Position>(first, {1, 2, 3});
	world.add<Velocity>(first, {0.1f, 0.2f, 0.3f});
	world.add<Health>(first, {100});

	std::vector<gaia::ecs::Entity> entities;
	entities.reserve(kEntities);
	entities.push_back(first);
	world.copy_n(first, kEntities - 1, [&](gaia::ecs::Entity newEntity) { entities.push_back(newEntity); });

	bench.run(name, [&] {
		for (auto entity : entities) {
			ankerl::nanobench::doNotOptimizeAway(world.get<Position>(entity));
			ankerl::nanobench::doNotOptimizeAway(world.get<Velocity>(entity));
			ankerl::nanobench::doNotOptimizeAway(world.get<Health>(entity));
		}
	});
}

// Data must already be populated with the same EntityIds/scene_id as a real
// Scene registered through SceneManager - Archetype<T>'s constructor always
// validates through that Scene, regardless of which ComponentData
// implementation's storage ForEach is walking. See main()'s ForEach block.
//
// Caveat versus RunForEachGaia below: gaia's Query is built once, outside
// bench.run - its archetype-matching is resolved and cached up front, so
// q.each() only pays for iteration. ComponentData::ForEach re-scans every
// table in tables (see component_data.hpp) on every single call inside this
// timed loop, matching against Components fresh each time - there's no
// equivalent up-front caching on this side. That scan is free in this
// specific benchmark (Populate() puts every entity into the one shared
// archetype, so there's exactly one table to check), but it scales with the
// number of distinct archetypes in the ComponentData, not with what's
// actually being iterated - the same "table fragmentation" cost the Heavy
// benchmark's own comments flag elsewhere in this file. With many
// archetypes and a narrow query, this comparison would no longer be fair.
template<typename Data> void RunForEach(ankerl::nanobench::Bench &bench, const char *name, Data &data) {
	bench.run(name, [&] {
		float sum = 0;
		data.template ForEach<Position>([&](tgl::Archetype<Position> entity) { sum += entity.Get<Position>().x; });
		ankerl::nanobench::doNotOptimizeAway(sum);
	});
}

// Native EnTT iteration via registry.view<T>().each() - unlike
// bench_impl::entt_ecs::ComponentData::ForEach (used by RunForEach above for
// current/dense_array/legacy), this doesn't reconstruct a tgl::EntityId or a
// tgl::Archetype per entity - that translation only exists to let the
// wrapper hand back what ComponentData::ForEach's interface promises, and
// isn't something a real EnTT program pays for iterating its own view.
void RunForEachEntt(ankerl::nanobench::Bench &bench, const char *name) {
	entt::registry registry;
	for (int i = 0; i < kEntities; i++) {
		auto entity = registry.create();
		registry.emplace<Position>(entity, 1.f, 2.f, 3.f);
	}

	bench.run(name, [&] {
		float sum = 0;
		registry.view<Position>().each([&](Position &p) { sum += p.x; });
		ankerl::nanobench::doNotOptimizeAway(sum);
	});
}

// Native gaia-ecs iteration via Query::each - entities are created up front
// via the same batched copy_n() path used elsewhere in this file, outside
// bench.run, so this measures the query/iteration itself, not entity
// creation. Same reasoning as RunForEachEntt above: gaia's own Query/Iter
// types are exactly what a real gaia program would iterate with, no
// wrapper-induced EntityId/Archetype reconstruction in between.
void RunForEachGaia(ankerl::nanobench::Bench &bench, const char *name) {
	gaia::ecs::World world;
	gaia::ecs::Entity first = world.add();
	world.add<Position>(first, {1, 2, 3});
	world.copy_n(first, kEntities - 1);

	gaia::ecs::Query q = world.query().all<Position>();

	bench.run(name, [&] {
		float sum = 0;
		q.each([&](const Position &p) { sum += p.x; });
		ankerl::nanobench::doNotOptimizeAway(sum);
	});
}

// Native flecs iteration via a flecs::query<T> built once outside bench.run
// - same fairness convention as RunForEachGaia above: query resolution is
// paid for up front, so q.each() only times iteration itself.
void RunForEachFlecs(ankerl::nanobench::Bench &bench, const char *name) {
	flecs::world world;
	for (int i = 0; i < kEntities; i++)
		world.entity().set<Position>({1, 2, 3});

	flecs::query<Position> q = world.query<Position>();

	bench.run(name, [&] {
		float sum = 0;
		q.each([&](Position &p) { sum += p.x; });
		ankerl::nanobench::doNotOptimizeAway(sum);
	});
}

// Same population as RunForEach above, but writes to each entity's own
// Position in place instead of summing into one shared accumulator. Every
// addss in RunForEach's unrolled block depends on the previous one (they all
// write the same xmm0), so its lower instruction count didn't reduce
// wall-clock time there - floating-point add latency was already the
// bottleneck, not instruction throughput. Writing to a different row per
// iteration has no such cross-iteration dependency, so this isolates
// whether the unrolled ForEach's ~4x fewer instructions actually shows up as
// real time once the CPU isn't stuck waiting on a serial reduction chain.
template<typename Data> void RunForEachIndependent(ankerl::nanobench::Bench &bench, const char *name, Data &data) {
	bench.run(name, [&] {
		data.template ForEach<Position>([&](tgl::Archetype<Position> entity) { entity.Get<Position>().x += 1.f; });
		ankerl::nanobench::doNotOptimizeAway(data);
	});
}

void RunForEachIndependentEntt(ankerl::nanobench::Bench &bench, const char *name) {
	entt::registry registry;
	for (int i = 0; i < kEntities; i++) {
		auto entity = registry.create();
		registry.emplace<Position>(entity, 1.f, 2.f, 3.f);
	}

	bench.run(name, [&] {
		registry.view<Position>().each([&](Position &p) { p.x += 1.f; });
		ankerl::nanobench::doNotOptimizeAway(registry);
	});
}

void RunForEachIndependentGaia(ankerl::nanobench::Bench &bench, const char *name) {
	gaia::ecs::World world;
	gaia::ecs::Entity first = world.add();
	world.add<Position>(first, {1, 2, 3});
	world.copy_n(first, kEntities - 1);

	gaia::ecs::Query q = world.query().all<Position &>();

	bench.run(name, [&] {
		q.each([&](Position &p) { p.x += 1.f; });
		ankerl::nanobench::doNotOptimizeAway(world);
	});
}

void RunForEachIndependentFlecs(ankerl::nanobench::Bench &bench, const char *name) {
	flecs::world world;
	for (int i = 0; i < kEntities; i++)
		world.entity().set<Position>({1, 2, 3});

	flecs::query<Position> q = world.query<Position>();

	bench.run(name, [&] {
		q.each([&](Position &p) { p.x += 1.f; });
		ankerl::nanobench::doNotOptimizeAway(world);
	});
}

// current has real multi-component archetype queries: this visits only the
// table(s) whose signature contains both Position and Velocity.
template<typename Data> void RunForEachTwoNative(ankerl::nanobench::Bench &bench, const char *name, Data &data) {
	bench.run(name, [&] {
		float sum = 0;
		data.template ForEach<Position, Velocity>([&](tgl::Archetype<Position, Velocity> entity) { sum += entity.Get<Position>().x + entity.Get<Velocity>().dx; });
		ankerl::nanobench::doNotOptimizeAway(sum);
	});
}

// Native EnTT two-component iteration via registry.view<Position,
// Velocity>().each() - entt's multi-type view only visits entities owning
// both components, its own equivalent of current's archetype query. Same
// 50/25/25 both/Position-only/Velocity-only split as PopulateMixed, so the
// view actually has to filter rather than match every entity.
void RunForEachTwoEntt(ankerl::nanobench::Bench &bench, const char *name) {
	entt::registry registry;
	for (int i = 0; i < kEntities; i++) {
		auto entity = registry.create();
		int bucket = i % 4;
		if (bucket < 2) {
			registry.emplace<Position>(entity, 1.f, 2.f, 3.f);
			registry.emplace<Velocity>(entity, 0.1f, 0.2f, 0.3f);
		} else if (bucket == 2) {
			registry.emplace<Position>(entity, 1.f, 2.f, 3.f);
		} else {
			registry.emplace<Velocity>(entity, 0.1f, 0.2f, 0.3f);
		}
	}

	bench.run(name, [&] {
		float sum = 0;
		registry.view<Position, Velocity>().each([&](Position &p, Velocity &v) { sum += p.x + v.dx; });
		ankerl::nanobench::doNotOptimizeAway(sum);
	});
}

// Native flecs two-component iteration via flecs::query<Position, Velocity>,
// built once outside bench.run - same fairness convention as RunForEachFlecs
// above. Same 50/25/25 mixed population as RunForEachTwoEntt.
void RunForEachTwoFlecs(ankerl::nanobench::Bench &bench, const char *name) {
	flecs::world world;
	for (int i = 0; i < kEntities; i++) {
		flecs::entity entity = world.entity();
		int bucket = i % 4;
		if (bucket < 2) {
			entity.set<Position>({1, 2, 3});
			entity.set<Velocity>({0.1f, 0.2f, 0.3f});
		} else if (bucket == 2) {
			entity.set<Position>({1, 2, 3});
		} else {
			entity.set<Velocity>({0.1f, 0.2f, 0.3f});
		}
	}

	flecs::query<Position, Velocity> q = world.query<Position, Velocity>();

	bench.run(name, [&] {
		float sum = 0;
		q.each([&](Position &p, Velocity &v) { sum += p.x + v.dx; });
		ankerl::nanobench::doNotOptimizeAway(sum);
	});
}

// Native gaia-ecs two-component iteration via Query::all<Position&,
// Velocity>(), built once outside bench.run - same fairness convention as
// RunForEachGaia above. Same 50/25/25 mixed population as RunForEachTwoEntt.
void RunForEachTwoGaia(ankerl::nanobench::Bench &bench, const char *name) {
	gaia::ecs::World world;
	for (int i = 0; i < kEntities; i++) {
		gaia::ecs::Entity entity = world.add();
		int bucket = i % 4;
		if (bucket < 2) {
			world.add<Position>(entity, {1, 2, 3});
			world.add<Velocity>(entity, {0.1f, 0.2f, 0.3f});
		} else if (bucket == 2) {
			world.add<Position>(entity, {1, 2, 3});
		} else {
			world.add<Velocity>(entity, {0.1f, 0.2f, 0.3f});
		}
	}

	gaia::ecs::Query q = world.query().all<Position &>().all<Velocity>();

	bench.run(name, [&] {
		float sum = 0;
		q.each([&](Position &p, const Velocity &v) { sum += p.x + v.dx; });
		ankerl::nanobench::doNotOptimizeAway(sum);
	});
}

// dense_array/legacy have no multi-component ForEach, so they do what any
// engine without archetype support has to: ForEach over one component, then
// a manual per-row has-check for the other. Uses data.GetComponent directly
// (not Archetype<Velocity>::Get) so it's this implementation's own lookup
// being timed, not the real Scene's.
template<typename Data> void RunForEachTwoManual(ankerl::nanobench::Bench &bench, const char *name, Data &data) {
	bench.run(name, [&] {
		float sum = 0;
		data.template ForEach<Position>([&](tgl::Archetype<Position> entity) {
			tgl::EntityId id = static_cast<tgl::EntityInstance>(entity).GetId();
			if (Velocity *vel = data.template GetComponent<Velocity>(id))
				sum += entity.Get<Position>().x + vel->dx;
		});
		ankerl::nanobench::doNotOptimizeAway(sum);
	});
}

} // namespace

// Add further implementations under bench/implementations/, include their
// header above, and add a call for them in each block below to compare.
int main() {
	{
		ankerl::nanobench::Bench bench;
		bench.title("GetComponent across ComponentData implementations").relative(true).batch(kEntities * 3).minEpochIterations(200);

		RunGetComponent<bench_impl::current::ComponentData>(bench, "current (archetype/table storage)");
		RunGetComponent<bench_impl::dense_array::ComponentData>(bench, "dense_array (TypeId-indexed vector)");
		RunGetComponent<bench_impl::legacy::ComponentData>(bench, "legacy (type_index hash map)");
		RunGetComponentEntt(bench, "entt (sparse-set ECS, v4.0.0)");
		RunGetComponentFlecs(bench, "flecs (archetype/table ECS, v4.1.6)");
		RunGetComponentGaia(bench, "gaia-ecs (archetype/chunk ECS, v0.9.2)");
	}

	{
		// Needs a real registered Scene (unlike the bare-ComponentData block
		// above) since Archetype<T> always resolves through
		// EntityInstance::GetSceneData at construction - see
		// RunArchetypeGetComponent.
		tgl::Scene *scene = &tgl::SceneManager::Get().NewScene();
		SceneId scene_id = scene->GetId();
		auto &data = scene->GetComponentData();
		Populate(data, scene_id);

		ankerl::nanobench::Bench bench;
		bench.title("GetComponent<T> (checked) vs Archetype<T...>::Get<T> (cached table+row)").relative(true).batch(kEntities * 3);

		RunGetComponentChecked(bench, "ComponentData::GetComponent<T> (checked, nullable)", data, scene_id);
		RunArchetypeGetComponent(bench, "Archetype<T...>::Get<T> (table+row cached at construction)", scene_id);
	}

	{
		ankerl::nanobench::Bench bench;
		bench.title("GetComponent, heavy/realistic (many entities, many archetypes)").relative(true).batch(kHeavyEntities);

		RunGetComponentHeavy<bench_impl::current::ComponentData>(bench, "current (archetype/table storage)");
		RunGetComponentHeavy<bench_impl::dense_array::ComponentData>(bench, "dense_array (TypeId-indexed vector)");
		RunGetComponentHeavy<bench_impl::legacy::ComponentData>(bench, "legacy (type_index hash map)");
	}

	{
		ankerl::nanobench::Bench bench;
		bench.title("GetComponent<BigData> (40KB component - confirms cost is independent of sizeof(T))").relative(true).batch(kBigDataEntities);

		RunGetComponentBigData<bench_impl::current::ComponentData>(bench, "current (archetype/table storage)");
		RunGetComponentBigData<bench_impl::dense_array::ComponentData>(bench, "dense_array (TypeId-indexed vector)");
		RunGetComponentBigData<bench_impl::legacy::ComponentData>(bench, "legacy (type_index hash map)");
	}

	{
		ankerl::nanobench::Bench bench;
		bench.title("AddComponent across ComponentData implementations").relative(true).batch(kEntities * 3);

		RunAddComponent<bench_impl::current::ComponentData>(bench, "current (archetype/table storage)");
		RunAddComponent<bench_impl::dense_array::ComponentData>(bench, "dense_array (TypeId-indexed vector)");
		RunAddComponent<bench_impl::legacy::ComponentData>(bench, "legacy (type_index hash map)");
		RunAddComponentEntt(bench, "entt (sparse-set ECS, v4.0.0)");
		RunAddComponentFlecs(bench, "flecs (archetype/table ECS, v4.1.6, set<T> per component)");
		RunAddComponentFlecsBulk(bench, "flecs (archetype/table ECS, v4.1.6, ecs_bulk_init)");
		RunAddComponentGaia(bench, "gaia-ecs (archetype/chunk ECS, v0.9.2, add<T> per component)");
		RunAddComponentGaiaBulk(bench, "gaia-ecs (archetype/chunk ECS, v0.9.2, copy_n)");
	}

	{
		// current benchmarks directly against a real, registered Scene's
		// ComponentData. dense_array/legacy each get their own separate
		// storage populated with the same EntityIds/scene_id, purely so
		// Archetype<T>'s validation (which always resolves through the real
		// Scene above) succeeds for them too - see RunForEach's comment.
		tgl::Scene *scene = &tgl::SceneManager::Get().NewScene();
		SceneId scene_id = scene->GetId();

		auto &current_data = scene->GetComponentData();
		Populate(current_data, scene_id);

		bench_impl::dense_array::ComponentData dense_array_data;
		dense_array_data.SetSceneId(scene_id); // ForEach reconstructs EntityIds from this, unlike legacy which stores scene_id per-slot
		Populate(dense_array_data, scene_id);

		bench_impl::legacy::ComponentData legacy_data;
		Populate(legacy_data, scene_id);

		ankerl::nanobench::Bench bench;
		bench.title("ForEach across ComponentData implementations").relative(true).batch(kEntities);

		RunForEach(bench, "current (archetype/table storage)", current_data);
		RunForEach(bench, "dense_array (TypeId-indexed vector)", dense_array_data);
		RunForEach(bench, "legacy (type_index hash map)", legacy_data);
		RunForEachEntt(bench, "entt (sparse-set ECS, v4.0.0)");
		RunForEachFlecs(bench, "flecs (archetype/table ECS, v4.1.6)");
		RunForEachGaia(bench, "gaia-ecs (archetype/chunk ECS, v0.9.2)");
	}

	{
		// Same setup as the ForEach block above, but this one writes to each
		// entity's own Position in place instead of summing into a shared
		// accumulator - see RunForEachIndependent's comment for why that
		// distinction matters for whether the unrolled ForEach's lower
		// instruction count actually reduces wall-clock time.
		tgl::Scene *scene = &tgl::SceneManager::Get().NewScene();
		SceneId scene_id = scene->GetId();

		auto &current_data = scene->GetComponentData();
		Populate(current_data, scene_id);

		bench_impl::dense_array::ComponentData dense_array_data;
		dense_array_data.SetSceneId(scene_id);
		Populate(dense_array_data, scene_id);

		bench_impl::legacy::ComponentData legacy_data;
		Populate(legacy_data, scene_id);

		ankerl::nanobench::Bench bench;
		bench.title("ForEach, independent per-entity writes (no shared accumulator)").relative(true).batch(kEntities);

		RunForEachIndependent(bench, "current (archetype/table storage)", current_data);
		RunForEachIndependent(bench, "dense_array (TypeId-indexed vector)", dense_array_data);
		RunForEachIndependent(bench, "legacy (type_index hash map)", legacy_data);
		RunForEachIndependentEntt(bench, "entt (sparse-set ECS, v4.0.0)");
		RunForEachIndependentFlecs(bench, "flecs (archetype/table ECS, v4.1.6)");
		RunForEachIndependentGaia(bench, "gaia-ecs (archetype/chunk ECS, v0.9.2)");
	}

	{
		// Same trick as the single-component ForEach block above: dense_array/
		// legacy get their own storage populated with matching EntityIds/
		// scene_id purely so Archetype<T>::Get resolves - see RunForEach's
		// comment. Distribution: 50% of entities have both Position and
		// Velocity, 25% have only Position, 25% have only Velocity.
		tgl::Scene *scene = &tgl::SceneManager::Get().NewScene();
		SceneId scene_id = scene->GetId();

		auto &current_data = scene->GetComponentData();
		PopulateMixed(current_data, scene_id);

		bench_impl::dense_array::ComponentData dense_array_data;
		dense_array_data.SetSceneId(scene_id);
		PopulateMixed(dense_array_data, scene_id);

		bench_impl::legacy::ComponentData legacy_data;
		PopulateMixed(legacy_data, scene_id);

		ankerl::nanobench::Bench bench;
		bench.title("ForEach<Position, Velocity> across ComponentData implementations").relative(true).batch(kEntities / 2);

		RunForEachTwoNative(bench, "current (native 2-component archetype query)", current_data);
		RunForEachTwoManual(bench, "dense_array (ForEach<Position> + manual has-check)", dense_array_data);
		RunForEachTwoManual(bench, "legacy (ForEach<Position> + manual has-check)", legacy_data);
		RunForEachTwoEntt(bench, "entt (sparse-set ECS, v4.0.0)");
		RunForEachTwoFlecs(bench, "flecs (archetype/table ECS, v4.1.6)");
		RunForEachTwoGaia(bench, "gaia-ecs (archetype/chunk ECS, v0.9.2)");
	}
}
