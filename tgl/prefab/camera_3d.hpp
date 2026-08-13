#pragma once

#include "glm/fwd.hpp"
#include "glm/gtx/norm.hpp"
#include "tgl/component/3d_camera.hpp"
#include "tgl/component/3d_transform.hpp"
#include "tgl/component_data.hpp"
#include "tgl/entity.hpp"
#include "tgl/archetype.hpp"
#include "utils/glm_utils.h"
#include <iostream>

class Camera3d : public Archetype<Transform3dComponent, Camera3dComponent> {
  public:
	Camera3d(EntityInstance instance) : Archetype<Transform3dComponent, Camera3dComponent>(instance) {}
	glm::vec3 Forward() {
		glm::vec3 rotation = GetRotation();
		glm::vec3 forward;
		forward.x = cos(rotation.y) * cos(rotation.x);
		forward.y = sin(rotation.x);
		forward.z = sin(rotation.y) * cos(rotation.x);
		forward = glm::normalize(forward);
		return forward;
	}
	glm::vec3 Right() {
		glm::vec3 globalUp = glm::vec3(0.0f, 1.0f, 0.0f);
		return glm::normalize(glm::cross(Forward(), globalUp));
	}

	glm::vec3 Up() { return glm::normalize(glm::cross(Right(), Forward())); }

	void RotateThirdPerson_OX(float angle, float distance) {
		SetPosition(GetPosition() + Forward() * distance);
		RotatePitch(angle);
		SetPosition(GetPosition() - Forward() * distance);
	}

	void RotateThirdPerson_OY(float angle, float distance) {
		SetPosition(GetPosition() + Forward() * distance);
		RotateYaw(angle);
		SetPosition(GetPosition() - Forward() * distance);
	}
	glm::mat4 ViewMatrix() { return glm::lookAt(GetPosition(), GetPosition() + Forward(), Up()); }

	static tgl::Entity New() {
		tgl::Entity c;
		c.AddComponent<Transform3dComponent>();
		c.AddComponent<Camera3dComponent>({glm::ivec2(1280, 720)});
		return c;
	}
};