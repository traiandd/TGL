#pragma once

#include "system.hpp"
#include "../entity.hpp"
#include "../component/2d_transform.hpp"
#include "tgl/scene.hpp"

using namespace tgl;

class ItemHold : public System {
  public:
	ItemHold(Scene *scene, EntityId id) : System(scene), m_ghost_id(id) {
		scene->OnMouseMove().Subscribe([this](const MouseMoveEvent &e) { this->onMouseMove(e); });

		auto ghost = m_scene->GetEntity(m_ghost_id);
	}
	void onMouseMove(MouseMoveEvent e) {
		auto player = m_scene->GetEntity(m_ghost_id);
		player.Get<Transform2dComponent>()->SetPosition({e.mouseX, e.mouseY});
	}

  protected:
	EntityId m_ghost_id;
};