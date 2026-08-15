#define ANKERL_NANOBENCH_IMPLEMENT
#include "nanobench/nanobench.h"

#include "implementations/current/component_data.hpp"
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

} // namespace

// Add further implementations under bench/implementations/, include their
// header above, and add a call for them in each block below to compare.
int main() {
	{
		ankerl::nanobench::Bench bench;
		bench.title("GetComponent across ComponentData implementations").relative(true).batch(kEntities * 3);

		RunGetComponent<bench_impl::current::ComponentData>(bench, "current (dense vector index)");
		RunGetComponent<bench_impl::legacy::ComponentData>(bench, "legacy (type_index hash map)");
	}

	{
		ankerl::nanobench::Bench bench;
		bench.title("AddComponent across ComponentData implementations").relative(true).batch(kEntities * 3);

		RunAddComponent<bench_impl::current::ComponentData>(bench, "current (dense vector index)");
		RunAddComponent<bench_impl::legacy::ComponentData>(bench, "legacy (type_index hash map)");
	}

	{
		// current benchmarks directly against a real, registered Scene's
		// ComponentData. legacy gets its own separate storage populated
		// with the same EntityIds/scene_id, purely so Archetype<T>'s
		// validation (which always resolves through the real Scene above)
		// succeeds for it too - see RunForEach's comment.
		SceneId scene_id = tgl::SceneManager::Get().AddScene(tgl::Scene());
		tgl::Scene *scene = tgl::SceneManager::Get().GetScene(scene_id);

		auto &current_data = scene->GetComponentData();
		Populate(current_data, scene_id);

		bench_impl::legacy::ComponentData legacy_data;
		Populate(legacy_data, scene_id);

		ankerl::nanobench::Bench bench;
		bench.title("ForEach across ComponentData implementations").relative(true).batch(kEntities);

		RunForEach(bench, "current (dense vector index)", current_data);
		RunForEach(bench, "legacy (type_index hash map)", legacy_data);
	}
}
