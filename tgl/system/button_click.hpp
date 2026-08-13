#pragma once

#include "../scene.hpp"
#include "../component/2d_transform.hpp"
#include "../component/ui_button.hpp"
#include "system.hpp"
#include "tgl/entity.hpp"
using namespace tgl;

namespace tgl {
class ButtonClickSystem : public System {
  public:
	ButtonClickSystem(Scene *scene) : System(scene) {
		scene->OnMouseDown().Subscribe([this](const MouseClickEvent &e) { this->OnMouseDown(e); });
		scene->OnMouseUp().Subscribe([this](const MouseClickEvent &e) { this->OnMouseUp(e); });
	}

	void OnMouseDown(MouseClickEvent e) {
		auto &data = m_scene->GetComponentData();

		data.ForEach<UIButton>([e, this](Archetype<UIButton> entity) {
			auto transformComponent = entity.instance_.TryArche<Transform2dComponent>();
			if (!transformComponent)
				return;
			auto pos = transformComponent->GlobalTransform();
			// TODO: transform rotation
			glm::vec2 button_pos = {pos[2][0] + entity.GetOffset().x, pos[2][1] + entity.GetOffset().y};
			// std::cout << button_pos << " " << e.screen_coords << "\n";
			if (button_pos.x - entity.GetDimension().x / 2 <= e.screen_coords.x && button_pos.x + entity.GetDimension().x / 2 >= e.screen_coords.x &&
				button_pos.y - entity.GetDimension().y / 2 <= e.screen_coords.y && button_pos.y + entity.GetDimension().y / 2 >= e.screen_coords.y) {
				entity.OnClickDown().Update(UIMouseClickEvent(this->m_scene, entity.instance_.GetId(), e));
				entity.SetPressed(true);
			}
		});
	}
	void OnMouseUp(MouseClickEvent e) {
		auto &data = m_scene->GetComponentData();
		data.ForEach<UIButton>([e, this](Archetype<UIButton> entity) {
			auto transformComponent = EntityInstance(entity).TryArche<Transform2dComponent>();
			if (!transformComponent)
				return;
			auto pos = transformComponent->GlobalTransform();
			// TODO: transform rotation
			glm::vec2 button_pos = {pos[2][0] + entity.GetOffset().x, pos[2][1] + entity.GetOffset().y};

			if (button_pos.x - entity.GetDimension().x / 2 <= e.screen_coords.x && button_pos.x + entity.GetDimension().x / 2 >= e.screen_coords.x &&
				button_pos.y - entity.GetDimension().y / 2 <= e.screen_coords.y && button_pos.y + entity.GetDimension().y / 2 >= e.screen_coords.y) {
				entity.OnClickUp().Update(UIMouseClickEvent(this->m_scene, entity.instance_.GetId(), e));
				if (entity.IsPressed()) {
					entity.OnClick().Update(UIMouseClickEvent(this->m_scene, entity.instance_.GetId(), e));
				}
			}
			entity.SetPressed(false);
		});
	}
};
} // namespace tgl