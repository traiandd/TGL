#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "tgl/archetype.hpp"
#include "tgl/component_data.hpp"
#include "tgl/scene.hpp"
#include "tgl/scene_manager.hpp"

#include <vector>

// ForEach builds a tgl::Archetype<Components...>, whose constructor
// validates through EntityInstance::GetScene() -> SceneManager::Get(), so
// the ForEach tests below register a real Scene through SceneManager first
// (see tests/scene_entity_tests.cpp for the same pattern at the
// Scene/entity level). Add/Remove/Get/size don't need that - they work on
// a bare ComponentData with fabricated EntityIds.
//
// ComponentData's storage is archetype/table-based (see
// tgl/component_data.hpp): entities are grouped by their exact component
// set, and adding/removing a component moves the entity's row to a
// different table. These tests exercise that through the public API only -
// ArchetypeTable/Column/IColumn are implementation details, not tested
// directly.

using tgl::EntityId;

namespace {
struct Position {
	float x = 0, y = 0;
};

struct Velocity {
	float dx = 0, dy = 0;
};

struct Health {
	int hp = 0;
};
} // namespace

// Archetype<T...> requires an ArchetypeExtender<T, Derived> specialization
// for every component type it's instantiated with (see tgl/archetype.hpp) -
// real components provide one to forward behavior (SetPosition etc.); these
// test-only types just need an empty one to be usable in Archetype<T...>.
namespace tgl {
template<typename Derived> class ArchetypeExtender<Position, Derived> {};
template<typename Derived> class ArchetypeExtender<Velocity, Derived> {};
template<typename Derived> class ArchetypeExtender<Health, Derived> {};
} // namespace tgl

TEST_CASE("ComponentData::AddComponent / GetComponent round-trip") {
	ComponentData data;
	EntityId e{0, 1};

	CHECK(data.GetComponent<Position>(e) == nullptr);

	data.AddComponent(e, Position{1, 2});
	REQUIRE(data.GetComponent<Position>(e) != nullptr);
	CHECK(data.GetComponent<Position>(e)->x == 1);
}

TEST_CASE("ComponentData::AddComponent overwrites an existing component in place") {
	ComponentData data;
	EntityId e{0, 1};

	data.AddComponent(e, Position{1, 2});
	data.AddComponent(e, Velocity{3, 4});
	data.AddComponent(e, Position{9, 9});

	REQUIRE(data.GetComponent<Position>(e) != nullptr);
	CHECK(data.GetComponent<Position>(e)->x == 9);
	REQUIRE(data.GetComponent<Velocity>(e) != nullptr);
	CHECK(data.GetComponent<Velocity>(e)->dx == 3);
}

TEST_CASE("ComponentData keeps distinct component types independent across archetype-table transitions") {
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
	REQUIRE(data.GetComponent<Velocity>(e) != nullptr);
	CHECK(data.GetComponent<Velocity>(e)->dx == 3);
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

TEST_CASE("ComponentData::RemoveComponent on a type never added is a no-op") {
	ComponentData data;
	EntityId e{0, 1};
	data.RemoveComponent<Position>(e);
	CHECK(data.GetComponent<Position>(e) == nullptr);
}

TEST_CASE("Removing a component relocates the swapped sibling entity's row correctly") {
	// e0, e1, e2 all land in the same archetype table (Position+Velocity).
	// Removing e1's Velocity moves it to a different table and swap-removes
	// its old row - the table's last entity (e2) should get relocated into
	// the vacated slot, and still resolve correctly afterwards.
	ComponentData data;
	EntityId e0{0, 1}, e1{1, 1}, e2{2, 1};

	for (EntityId e : {e0, e1, e2}) {
		data.AddComponent(e, Position{float(e.id), 0});
		data.AddComponent(e, Velocity{float(e.id) * 10, 0});
	}

	data.RemoveComponent<Velocity>(e1);

	CHECK(data.GetComponent<Velocity>(e1) == nullptr);
	REQUIRE(data.GetComponent<Position>(e1) != nullptr);
	CHECK(data.GetComponent<Position>(e1)->x == 1);

	REQUIRE(data.GetComponent<Position>(e0) != nullptr);
	CHECK(data.GetComponent<Position>(e0)->x == 0);
	REQUIRE(data.GetComponent<Velocity>(e0) != nullptr);
	CHECK(data.GetComponent<Velocity>(e0)->dx == 0);

	REQUIRE(data.GetComponent<Position>(e2) != nullptr);
	CHECK(data.GetComponent<Position>(e2)->x == 2);
	REQUIRE(data.GetComponent<Velocity>(e2) != nullptr);
	CHECK(data.GetComponent<Velocity>(e2)->dx == 20);
}

TEST_CASE("ComponentData::size reflects the number of distinct archetype tables in use") {
	ComponentData data;
	EntityId e0{0, 1}, e1{1, 1};
	CHECK(data.size() == 0);

	data.AddComponent(e0, Position{1, 2}); // table {Position}
	CHECK(data.size() == 1);

	data.AddComponent(e0, Velocity{3, 4}); // moves e0 to table {Position, Velocity}
	CHECK(data.size() == 2);

	data.AddComponent(e1, Position{5, 6}); // reuses table {Position}
	CHECK(data.size() == 2);
}

TEST_CASE("ComponentData::ForEach<T> visits every entity with the component, and no others") {
	tgl::Scene *scene = &tgl::SceneManager::Get().NewScene();
	SceneId scene_id = scene->GetId();
	auto &data = scene->GetComponentData();

	EntityId with_position{0, scene_id};
	EntityId with_velocity{1, scene_id};
	data.AddComponent(with_position, Position{1, 2});
	data.AddComponent(with_velocity, Velocity{3, 4});

	std::vector<tgl::LocalEntityId> visited;
	data.ForEach<Position>([&](tgl::Archetype<Position> entity) { visited.push_back(entity.instance_.GetId().id); });

	REQUIRE(visited.size() == 1);
	CHECK(visited[0] == with_position.id);
}

TEST_CASE("ComponentData::ForEach<T> skips entities whose component was removed") {
	tgl::Scene *scene = &tgl::SceneManager::Get().NewScene();
	SceneId scene_id = scene->GetId();
	auto &data = scene->GetComponentData();

	EntityId e0{0, scene_id};
	EntityId e1{1, scene_id};
	data.AddComponent(e0, Position{1, 2});
	data.AddComponent(e1, Position{3, 4});
	data.RemoveComponent<Position>(e0);

	int count = 0;
	data.ForEach<Position>([&](tgl::Archetype<Position> entity) {
		count++;
		CHECK(entity.instance_.GetId().id == e1.id);
	});
	CHECK(count == 1);
}

TEST_CASE("ComponentData::ForEach<T>'s Archetype exposes the actual component value") {
	tgl::Scene *scene = &tgl::SceneManager::Get().NewScene();
	SceneId scene_id = scene->GetId();
	auto &data = scene->GetComponentData();

	EntityId e{0, scene_id};
	data.AddComponent(e, Position{7, 8});

	int visits = 0;
	data.ForEach<Position>([&](tgl::Archetype<Position> entity) {
		visits++;
		REQUIRE(entity.Get<Position>() != nullptr);
		CHECK(entity.Get<Position>()->x == 7);
		CHECK(entity.Get<Position>()->y == 8);
	});
	CHECK(visits == 1);
}

TEST_CASE("ComponentData::ForEach<A, B> only visits entities that have both, including ones with a superset") {
	tgl::Scene *scene = &tgl::SceneManager::Get().NewScene();
	SceneId scene_id = scene->GetId();
	auto &data = scene->GetComponentData();

	EntityId both{0, scene_id};
	EntityId all_three{1, scene_id};
	EntityId only_position{2, scene_id};

	data.AddComponent(both, Position{1, 0});
	data.AddComponent(both, Velocity{1, 0});

	data.AddComponent(all_three, Position{2, 0});
	data.AddComponent(all_three, Velocity{2, 0});
	data.AddComponent(all_three, Health{50});

	data.AddComponent(only_position, Position{3, 0});

	std::vector<tgl::LocalEntityId> visited;
	data.ForEach<Position, Velocity>([&](tgl::Archetype<Position, Velocity> entity) { visited.push_back(entity.instance_.GetId().id); });

	REQUIRE(visited.size() == 2);
	CHECK((visited[0] == both.id || visited[0] == all_three.id));
	CHECK((visited[1] == both.id || visited[1] == all_three.id));
	CHECK(visited[0] != visited[1]);
}
