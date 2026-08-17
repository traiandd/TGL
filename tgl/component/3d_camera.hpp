#pragma once
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/glm.hpp"
#include "tgl/entity.hpp"
#include "utils/math_utils.h"
#include "tgl/archetype.hpp"

class Camera3dComponent {
  public:
	glm::ivec2 m_dim;
	Camera3dComponent() = default;
	Camera3dComponent(glm::ivec2 dim) : m_dim(dim) {}

	glm::mat4 ProjectionMatrix() { return glm::perspective(RADIANS(90.), (double)m_dim.x / m_dim.y, 0.001, 1000.); }

	glm::ivec2 GetSizes() { return m_dim; }
};

template<typename Derived> class tgl::ArchetypeExtender<Camera3dComponent, Derived> {
	Using(Self(Camera3dComponent));

  public:
	Using(Forward(ProjectionMatrix, GetSizes));
};