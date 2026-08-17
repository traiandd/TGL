#pragma once

#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "glm/geometric.hpp"
#include "glm/glm.hpp"
#include "tgl/archetype.hpp"
#include <iostream>
#include <string>

struct UITextComponent {
  private:
	std::string text_ = "Hello world!";
	glm::vec3 color_ = {1., 1., 1.};

  public:
	UITextComponent() {};
	UITextComponent(const std::string text) : text_(text) {}
	UITextComponent(const std::string text, glm::vec3 color) : text_(text), color_(color) {}

	UITextComponent *SetText(std::string text) {
		text_ = text;
		return this;
	}
	std::string GetText() { return text_; }

	UITextComponent *SetTextColor(glm::vec3 color) {
		color_ = color;
		return this;
	}
	glm::vec3 GetTextColor() { return color_; }
};

template<typename Derived> class tgl::ArchetypeExtender<UITextComponent, Derived> {
	Using(Instance);
	Using(Self(UITextComponent));

  public:
	Using(Forward(SetText, GetText, SetTextColor, GetTextColor));
};