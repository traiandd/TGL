#define ANKERL_NANOBENCH_IMPLEMENT
#include "nanobench/nanobench.h"

#include "implementations/current/component_data.hpp"
#include "implementations/legacy/component_data.hpp"

#include <cstdint>

// NOTE: same constraint as tests/component_data_tests.cpp - ForEach needs a
// live GameManager/window, so these benchmarks stick to Add/Get, which is
// also where the implementations under comparison actually differ. See
// bench/implementations/ for the implementations themselves.

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

constexpr int kEntities = 10000;

template<typename Data> void Populate(Data &data) {
	for (int i = 0; i < kEntities; i++) {
		EntityId e{static_cast<uint32_t>(i), 1};
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
}
