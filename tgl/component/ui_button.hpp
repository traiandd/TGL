#pragma once
#include "glm/fwd.hpp"
#include "glm/glm.hpp"
#include "register_component.hpp"
#include "../entity.hpp"
#include "../observer.hpp"
#include "../scene.hpp"

namespace tgl {
class Scene; // forward declaration
}

struct UIMouseClickEvent : MouseClickEvent {
	tgl::Scene *scene;
	tgl::EntityId target;

	UIMouseClickEvent() : scene(nullptr) {}

	UIMouseClickEvent(tgl::Scene *scene, tgl::EntityId target, MouseClickEvent e) : MouseClickEvent(e), scene(scene), target(target) {}
};

struct UIButton : Component {
	glm::vec2 m_dim = glm::vec2(0.f);
	glm::vec2 m_offset = glm::vec2(0.f);

	Observer<UIMouseClickEvent> onClick;
	Observer<UIMouseClickEvent> onClickDown;
	Observer<UIMouseClickEvent> onClickUp;

	bool pressed = false;
	UIButton() {}
	UIButton(glm::vec2 dim) : m_dim(dim) {}
	UIButton(glm::vec2 dim, glm::vec2 offset) : m_dim(dim), m_offset(offset) {}

	Observer<UIMouseClickEvent> &OnClick() { return onClick; }
	Observer<UIMouseClickEvent> &OnClickDown() { return onClickDown; }
	Observer<UIMouseClickEvent> &OnClickUp() { return onClickUp; }

	bool IsPressed() { return pressed; }

	void SetPressed(bool is_pressed) { pressed = is_pressed; }

	glm::vec2 GetDimension() { return m_dim; }
	glm::vec2 GetOffset() { return m_dim; }
};

register_component(UIButton);

template<typename Derived> class tgl::ArchetypeExtender<UIButton, Derived> {
	Using(Self(UIButton));

  public:
	Using(Forward(IsPressed, SetPressed, GetDimension, GetOffset, OnClick, OnClickDown, OnClickUp));
};