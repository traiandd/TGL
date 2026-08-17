#pragma once
#include "glm/fwd.hpp"
#include "glm/glm.hpp"
#include "../entity.hpp"
#include "../observer.hpp"
#include "../game_manager.hpp"
#include "tgl/component_data.hpp"
#include <set>

namespace tgl {
class Scene; // forward declaration
}

struct HitboxCollideEvent {
	tgl::Scene *scene;
	tgl::EntityInstance target;
	tgl::EntityInstance other;

	HitboxCollideEvent() : scene(nullptr) {}

	HitboxCollideEvent(tgl::Scene *scene, tgl::EntityInstance target, tgl::EntityInstance other) : scene(scene), target(target), other(other) {}
};

struct Hitbox2d {
	glm::vec2 m_dim = glm::vec2(0.f);
	glm::vec2 m_offset = glm::vec2(0.f);

	Observer<HitboxCollideEvent> onCollide;

	Hitbox2d(glm::vec2 dim) : m_dim(dim) {}
	Hitbox2d(glm::vec2 dim, glm::vec2 offset) : m_dim(dim), m_offset(offset) {}

	Observer<HitboxCollideEvent> &OnCollide() { return onCollide; }

	glm::vec2 GetDimension() { return m_dim; }
};
