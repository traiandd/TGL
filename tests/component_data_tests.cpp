#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "tgl/component_data.hpp"

// NOTE: ComponentData::ForEach (and ComponentStorage::ForEach) build a
// tgl::Archetype<T>, whose constructor validates through
// EntityInstance::GetScene() -> GameManager::Get() -> World -> a real
// window. That's not available in a plain test binary, so these tests only
// cover Add/Remove/Get/size, which don't touch that path.

using tgl::EntityId;

namespace {
struct Position {
	float x = 0, y = 0;
};

struct Velocity {
	float dx = 0, dy = 0;
};
} // namespace

TEST_CASE("ComponentStorage stores and retrieves by local id") {
	ComponentStorage<Position> storage;
	CHECK(storage.Get(0) == nullptr);

	storage.Add(0, Position{1, 2});
	REQUIRE(storage.Get(0) != nullptr);
	CHECK(storage.Get(0)->x == 1);
	CHECK(storage.Get(0)->y == 2);

	CHECK(storage.Get(1) == nullptr);
}

TEST_CASE("ComponentStorage::Remove clears only the given slot") {
	ComponentStorage<Position> storage;
	storage.Add(0, Position{1, 2});
	storage.Add(1, Position{3, 4});

	storage.Remove(0);
	CHECK(storage.Get(0) == nullptr);
	REQUIRE(storage.Get(1) != nullptr);
	CHECK(storage.Get(1)->x == 3);
}

TEST_CASE("ComponentStorage::Remove on an id never added is a no-op") {
	ComponentStorage<Position> storage;
	storage.Remove(5);
	CHECK(storage.Get(5) == nullptr);
}

TEST_CASE("ComponentStorage overwrites an existing slot on re-Add") {
	ComponentStorage<Position> storage;
	storage.Add(0, Position{1, 2});
	storage.Add(0, Position{9, 9});
	REQUIRE(storage.Get(0) != nullptr);
	CHECK(storage.Get(0)->x == 9);
}

TEST_CASE("ComponentData::AddComponent / GetComponent round-trip") {
	ComponentData data;
	EntityId e{0, 1};

	CHECK(data.GetComponent<Position>(e) == nullptr);

	data.AddComponent(e, Position{1, 2});
	REQUIRE(data.GetComponent<Position>(e) != nullptr);
	CHECK(data.GetComponent<Position>(e)->x == 1);
}

TEST_CASE("ComponentData keeps distinct component types independent") {
	ComponentData data;
	EntityId e{0, 1};

	data.AddComponent(e, Position{1, 2});
	data.AddComponent(e, Velocity{3, 4});

	REQUIRE(data.GetComponent<Position>(e) != nullptr);
	REQUIRE(data.GetComponent<Velocity>(e) != nullptr);
	CHECK(data.GetComponent<Position>(e)->x == 1);
	CHECK(data.GetComponent<Velocity>(e)->dx == 3);

	data.RemoveComponent<Position>(e);
	CHECK(data.GetComponent<Position>(e) == nullptr);
	CHECK(data.GetComponent<Velocity>(e) != nullptr);
}

TEST_CASE("ComponentData::RemoveEntityData clears every component type for that entity only") {
	ComponentData data;
	EntityId e0{0, 1};
	EntityId e1{1, 1};

	data.AddComponent(e0, Position{1, 2});
	data.AddComponent(e0, Velocity{3, 4});
	data.AddComponent(e1, Position{5, 6});

	data.RemoveEntityData(e0);

	CHECK(data.GetComponent<Position>(e0) == nullptr);
	CHECK(data.GetComponent<Velocity>(e0) == nullptr);
	REQUIRE(data.GetComponent<Position>(e1) != nullptr);
	CHECK(data.GetComponent<Position>(e1)->x == 5);
}

TEST_CASE("ComponentData::size reflects the number of distinct component types used") {
	ComponentData data;
	EntityId e{0, 1};
	CHECK(data.size() == 0);

	data.AddComponent(e, Position{1, 2});
	CHECK(data.size() == 1);

	data.AddComponent(e, Velocity{3, 4});
	CHECK(data.size() == 2);
}

TEST_CASE("ComponentData::RemoveComponent on a type never added is a no-op") {
	ComponentData data;
	EntityId e{0, 1};
	data.RemoveComponent<Position>(e);
	CHECK(data.GetComponent<Position>(e) == nullptr);
}
