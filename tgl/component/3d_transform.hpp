#pragma once

#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "glm/geometric.hpp"
#include "glm/glm.hpp"
#include "glm/gtx/euler_angles.hpp"
#include "tgl/entity.hpp"
#include "parent.hpp"
#include "tgl/archetype.hpp"
#include <iostream>

struct Transform3dComponent {
  private:
	glm::vec3 position = glm::vec3(0.f);
	glm::vec3 rotation = glm::vec3(0.f);
	glm::vec3 scale;

  public:
	Transform3dComponent() : scale(glm::vec3(1)) {};

	glm::mat4 TransformMatrix() {
		glm::mat4 translation = glm::translate(glm::mat4(1.0f), position);
		glm::mat4 rotation_mat = glm::yawPitchRoll(rotation.y, rotation.x, rotation.z);
		glm::mat4 scale_mat = glm::scale(glm::mat4(1.0f), scale);
		return translation * rotation_mat * scale_mat;
	}

	Transform3dComponent *SetPosition(glm::vec3 new_pos) {
		position = new_pos;
		return this;
	}

	Transform3dComponent *SetScale(glm::vec3 new_scale) {
		scale = new_scale;
		return this;
	}

	glm::vec3 GetScale() { return scale; }

	Transform3dComponent *SetXScale(float new_scale) {
		scale[0] = new_scale;

		return this;
	}

	Transform3dComponent *SetYScale(float new_scale) {
		scale[1] = new_scale;

		return this;
	}

	Transform3dComponent *SetZScale(float new_scale) {
		scale[2] = new_scale;

		return this;
	}

	Transform3dComponent *RotatePitch(float angleRadians) {
		rotation.x += angleRadians;

		if (rotation.x > 1.55f)
			rotation.x = 1.55f;
		if (rotation.x < -1.55f)
			rotation.x = -1.55f;
		return this;
	}

	Transform3dComponent *RotateYaw(float angleRadians) {
		rotation.y += angleRadians;
		return this;
	}

	Transform3dComponent *RotateRoll(float angleRadians) {
		rotation.z += angleRadians;
		return this;
	}

	glm::vec3 GetRotation() { return rotation; }

	void SetRotation(glm::vec3 rot) { rotation = rot; }

	glm::vec3 GetPosition() { return position; }
};

impl(Transform3dComponent) {
	Using(Instance);
	Using(Self(Transform3dComponent));

  public:
	Using(Forward(RotatePitch,
				  RotateYaw,
				  RotateRoll,
				  GetRotation,
				  SetRotation,
				  SetPosition,
				  GetPosition,
				  SetScale,
				  SetXScale,
				  SetYScale,
				  SetZScale,
				  GetScale,
				  TransformMatrix));

	glm::mat4 GlobalTransform() {
		auto inv = Self();
		if (auto parent = Instance().template Get<Parent>()) {
			EntityInstance parent_instance = *parent;

			auto handle = parent_instance.TryArche<Transform3dComponent>();
			if (!handle) {
				return inv->TransformMatrix();
			}

			return handle->GlobalTransform() * inv->TransformMatrix();
		} else {
			return inv->TransformMatrix();
		}
	}

	glm::vec3 GlobalPosition() {
		glm::mat4 global_transform = GlobalTransform();
		return glm::vec3(global_transform[3]);
	}
};