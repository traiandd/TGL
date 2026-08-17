#include "doctest/doctest.h"

#include "tgl/entity.hpp"
#include "tgl/entity_builder.hpp"
#include "tgl/scene.hpp"
#include "tgl/scene_manager.hpp"

// These exercise EntityInstance/Scene through tgl::SceneManager, which -
// unlike tgl::GameManager - has no World/window dependency. That's the
// point being tested here: entity/component access no longer needs a live
// window, only ComponentData did before.

namespace {
struct Position {
	float x = 0, y = 0;
};
} // namespace

template<typename Derived> class tgl::ArchetypeExtender<Position, Derived> {};

TEST_CASE("EntityInstance resolves its own scene through SceneManager without a window") {
	tgl::Scene *scene = &tgl::SceneManager::Get().NewScene();

	auto builder = tgl::EntityBuilder<>().Add(Position{1, 2});
	tgl::EntityInstance instance = scene->AddEntity(std::move(builder));

	Position *p = instance.Get<Position>();
	REQUIRE(p != nullptr);
	CHECK(p->x == 1);
	CHECK(p->y == 2);

	CHECK(instance.GetScene() == scene);
}

TEST_CASE("Entities in different scenes don't see each other's components") {
	tgl::Scene *scene_a = &tgl::SceneManager::Get().NewScene();
	tgl::Scene *scene_b = &tgl::SceneManager::Get().NewScene();

	auto builder = tgl::EntityBuilder<>().Add(Position{9, 9});
	tgl::EntityInstance instance = scene_a->AddEntity(std::move(builder));

	CHECK(instance.Get<Position>() != nullptr);
	CHECK(scene_b->GetComponentData().GetComponent<Position>(instance.GetId()) == nullptr);
}
