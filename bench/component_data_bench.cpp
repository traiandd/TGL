#define ANKERL_NANOBENCH_IMPLEMENT
#include "nanobench/nanobench.h"

#include "implementations/current/component_data.hpp"
#include "implementations/dense_array/component_data.hpp"
#include "implementations/legacy/component_data.hpp"

#include "tgl/scene.hpp"
#include "tgl/scene_manager.hpp"

#include <cstdint>

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

template<typename Data> void RunAddComponent(ankerl::nanobench::Bench &bench, const char *name) {
	bench.run(name, [&] {
		Data data;
		Populate(data);
		ankerl::nanobench::doNotOptimizeAway(data);
	});
}

// Data must already be populated with the same EntityIds/scene_id as a real
// Scene registered through SceneManager - Archetype<T>'s constructor always
// validates through that Scene, regardless of which ComponentData
// implementation's storage ForEach is walking. See main()'s ForEach block.
template<typename Data> void RunForEach(ankerl::nanobench::Bench &bench, const char *name, Data &data) {
	bench.run(name, [&] {
		float sum = 0;
		data.template ForEach<Position>([&](tgl::Archetype<Position> entity) { sum += entity.Get<Position>()->x; });
		ankerl::nanobench::doNotOptimizeAway(sum);
	});
}

// current has real multi-component archetype queries: this visits only the
// table(s) whose signature contains both Position and Velocity.
template<typename Data> void RunForEachTwoNative(ankerl::nanobench::Bench &bench, const char *name, Data &data) {
	bench.run(name, [&] {
		float sum = 0;
		data.template ForEach<Position, Velocity>([&](tgl::Archetype<Position, Velocity> entity) { sum += entity.Get<Position>()->x + entity.Get<Velocity>()->dx; });
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
				sum += entity.Get<Position>()->x + vel->dx;
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
		bench.title("GetComponent across ComponentData implementations").relative(true).batch(kEntities * 3);

		RunGetComponent<bench_impl::current::ComponentData>(bench, "current (archetype/table storage)");
		RunGetComponent<bench_impl::dense_array::ComponentData>(bench, "dense_array (TypeId-indexed vector)");
		RunGetComponent<bench_impl::legacy::ComponentData>(bench, "legacy (type_index hash map)");
	}

	{
		ankerl::nanobench::Bench bench;
		bench.title("AddComponent across ComponentData implementations").relative(true).batch(kEntities * 3);

		RunAddComponent<bench_impl::current::ComponentData>(bench, "current (archetype/table storage)");
		RunAddComponent<bench_impl::dense_array::ComponentData>(bench, "dense_array (TypeId-indexed vector)");
		RunAddComponent<bench_impl::legacy::ComponentData>(bench, "legacy (type_index hash map)");
	}

	{
		// current benchmarks directly against a real, registered Scene's
		// ComponentData. dense_array/legacy each get their own separate
		// storage populated with the same EntityIds/scene_id, purely so
		// Archetype<T>'s validation (which always resolves through the real
		// Scene above) succeeds for them too - see RunForEach's comment.
		SceneId scene_id = tgl::SceneManager::Get().AddScene(tgl::Scene());
		tgl::Scene *scene = tgl::SceneManager::Get().GetScene(scene_id);

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
	}

	{
		// Same trick as the single-component ForEach block above: dense_array/
		// legacy get their own storage populated with matching EntityIds/
		// scene_id purely so Archetype<T>::Get resolves - see RunForEach's
		// comment. Distribution: 50% of entities have both Position and
		// Velocity, 25% have only Position, 25% have only Velocity.
		SceneId scene_id = tgl::SceneManager::Get().AddScene(tgl::Scene());
		tgl::Scene *scene = tgl::SceneManager::Get().GetScene(scene_id);

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
	}
}
