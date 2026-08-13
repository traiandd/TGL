#pragma once

#include "../component/2d_transform.hpp"
#include "../component/ui_button.hpp"
#include "glm/fwd.hpp"
#include "system.hpp"
#include "tgl/archetype.hpp"
#include "tgl/component/2d_hitbox.hpp"
#include "tgl/entity.hpp"

using namespace tgl;

namespace tgl {
class HitboxCollideSystem : public System {
  public:
	HitboxCollideSystem(Scene *scene) : System(scene) {
		scene->OnUpdate().Subscribe([this](float dt) { this->OnUpdate(); });
	}

	void OnUpdate() {
		auto &data = m_scene->GetComponentData();

		auto &boxes = data.GetAll<Hitbox2d>();
		for (auto &[entitya, boxa] : boxes) {
			for (auto &[entityb, boxb] : boxes) {
				if (entitya.id >= entityb.id)
					continue;

				auto ta = EntityInstance(entitya).TryArche<Transform2dComponent>();
				auto tb = EntityInstance(entityb).TryArche<Transform2dComponent>();
				if (!ta || !tb)
					continue;
				glm::mat3 ga = ta->GlobalTransform();
				glm::mat3 gb = tb->GlobalTransform();

				glm::vec2 posa = glm::vec2(ga[2][0], ga[2][1]) + boxa.m_offset;
				glm::vec2 posb = glm::vec2(gb[2][0], gb[2][1]) + boxb.m_offset;

				glm::vec2 halfa = boxa.m_dim * 0.5f;
				glm::vec2 halfb = boxb.m_dim * 0.5f;
				bool ox = std::abs(posa.x - posb.x) < (halfa.x + halfb.x);
				bool oy = std::abs(posa.y - posb.y) < (halfa.y + halfb.y);

				if (ox && oy) {
					HitboxCollideEvent eventA(m_scene, entitya, entityb);
					HitboxCollideEvent eventB(m_scene, entityb, entitya);

					boxa.OnCollide().Update(eventA);
					boxb.OnCollide().Update(eventB);
				}
			}
		}
	}
};
} // namespace tgl