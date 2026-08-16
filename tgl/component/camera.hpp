#pragma once

#include "register_component.hpp"
#include <cstdint>
#include "glm/glm.hpp"
#include "tgl/archetype.hpp"
#include "tgl/component/2d_transform.hpp"

struct CameraComponent : Component {
	uint32_t width;
	uint32_t height;

	CameraComponent() = default;
	CameraComponent(uint32_t width, uint32_t height) : width(width), height(height) {}

	glm::mat4 ProjectionMatrix() {
		glm::mat4 proj(1.0f);
		proj[0][0] = 2.0f / width;
		proj[1][1] = 2.0f / height;
		proj[2][2] = -1.0f;
		proj[3][3] = 1.0f;
		return proj;
	}

	glm::vec2 GetSizes() { return {width, height}; }
};

register_component(CameraComponent);

template<typename Derived> class tgl::ArchetypeExtender<CameraComponent, Derived> {
	Using(Instance);
	Using(Self(CameraComponent));

  public:
	glm::vec2 ScreenToWorldCoords(glm::vec2 screen_coords) {
		auto cam_pos = Instance().template Get<Transform2dComponent>();
		auto cam_data = Self();

		glm::vec4 ndc((2.0f * screen_coords.x) / cam_data->GetSizes().x - 1.0f, 1.0f - (2.0f * screen_coords.y) / cam_data->GetSizes().y, 0.0f, 1.0f);

		glm::mat4 invVP = glm::inverse(cam_data->ProjectionMatrix() * cam_pos->TransformMatrix());
		glm::vec4 world = invVP * ndc;
		world /= world.w;
		return glm::vec2(world);
	}
};
