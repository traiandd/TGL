#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "tgl/archetype.hpp"
#include "tgl/component_data.hpp"
#include "tgl/scene.hpp"
#include "tgl/scene_manager.hpp"

#include <vector>

// ForEach builds a tgl::Archetype<T>, whose constructor validates through
// EntityInstance::GetScene() -> SceneManager::Get(), so the ForEach tests
// below register a real Scene through SceneManager first (see
// tests/scene_entity_tests.cpp for the same pattern at the Scene/entity
// level). Add/Remove/Get/size don't need that - they work on a bare
// ComponentData with fabricated EntityIds.

using tgl::EntityId;

namespace {
struct Position {
	float x = 0, y = 0;
};

struct Velocity {
	float dx = 0, dy = 0;
};
} // namespace

// Archetype<T> requires an ArchetypeExtender<T, Derived> specialization for
// every component type it's instantiated with (see tgl/archetype.hpp) -
// real components provide one to forward behavior (SetPosition etc.); these
// test-only types just need an empty one to be usable in Archetype<T>.
namespace tgl {
template<typename Derived> class ArchetypeExtender<Position, Derived> {};
template<typename Derived> class ArchetypeExtender<Velocity, Derived> {};
} // namespace tgl

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

TEST_CASE("ComponentData::ForEach visits every entity with the component, and no others") {
	SceneId scene_id = tgl::SceneManager::Get().AddScene(tgl::Scene());
	tgl::Scene *scene = tgl::SceneManager::Get().GetScene(scene_id);
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

TEST_CASE("ComponentData::ForEach skips entities whose component was removed") {
	SceneId scene_id = tgl::SceneManager::Get().AddScene(tgl::Scene());
	tgl::Scene *scene = tgl::SceneManager::Get().GetScene(scene_id);
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

TEST_CASE("ComponentData::ForEach's Archetype exposes the actual component value") {
	SceneId scene_id = tgl::SceneManager::Get().AddScene(tgl::Scene());
	tgl::Scene *scene = tgl::SceneManager::Get().GetScene(scene_id);
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
