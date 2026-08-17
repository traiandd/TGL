#pragma once

#include "glm/fwd.hpp"
#include "glm/glm.hpp"
#include "tgl/archetype.hpp"
#include "tgl/component/parent.hpp"
#include "tgl/entity.hpp"
#include "parent.hpp"

struct Transform2dComponent {
	glm::mat3 transform;
	float z = 0;
	Transform2dComponent() : transform(glm::mat3(1)) {};
	Transform2dComponent(const glm::mat3 &t) : transform(t) {}

	glm::mat4 TransformMatrix() {
		glm::mat4 proj(1.0f);
		proj[0][0] = transform[0][0];
		proj[0][1] = transform[1][0];
		proj[1][0] = transform[0][1];
		proj[1][1] = transform[1][1];

		proj[0][3] = transform[2][0];
		proj[1][3] = transform[2][1];
		proj[2][3] = z;
		return proj;
	}

	Transform2dComponent *SetPosition(glm::vec2 new_pos) {
		transform[2][0] = new_pos[0];
		transform[2][1] = new_pos[1];
		return this;
	}

	glm::vec2 GetScale() { return glm::vec2(transform[0][0], transform[1][1]); }

	Transform2dComponent *SetScale(float scale) {
		transform[0][0] = scale;
		transform[1][1] = scale;
		return this;
	}

	Transform2dComponent *SetXScale(float scale) {
		transform[0][0] = scale;
		return this;
	}

	Transform2dComponent *SetYScale(float scale) {
		transform[1][1] = scale;
		return this;
	}

	Transform2dComponent *SetZIndex(float zindex) {
		z = zindex;
		return this;
	}

	glm::vec2 GetPosition() { return {transform[2][0], transform[2][1]}; }
};

template<typename Derived> class tgl::ArchetypeExtender<Transform2dComponent, Derived> {
	Using(Instance);
	Using(Self(Transform2dComponent));

  public:
	Using(Forward(SetPosition, GetPosition, SetScale, SetXScale, SetYScale, SetZIndex, TransformMatrix, GetScale));

	glm::mat4 GlobalTransform() {
		auto inv = Self();
		if (auto parent = Instance().template Get<Parent>()) {
			EntityInstance parent_instance = *parent;

			auto handle = parent_instance.TryArche<Transform2dComponent>();
			if (!handle) {
				return inv->TransformMatrix();
			}

			return handle->GlobalTransform() * inv->TransformMatrix();
		} else {
			return inv->TransformMatrix();
		}
	}
};