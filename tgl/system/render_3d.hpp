#pragma once

#include "tgl/archetype.hpp"
#include "tgl/entity.hpp"
#include "tgl/game_manager.hpp"
#include "../component/3d_transform.hpp"
#include "../component/3d_camera.hpp"
#include "../component/render.hpp"
#include "system.hpp"
#include "tgl/prefab/camera_3d.hpp"
#include "tgl/scene.hpp"

#include <iostream>
using namespace tgl;

void RenderMesh3D(Camera3d &camera, Mesh *mesh, Shader *shader, const glm::mat4 &modelMatrix) {
	if (!mesh || !shader || !shader->program)
		return;

	shader->Use();

	glm::mat4 v = camera.ViewMatrix();
	glm::mat4 p = camera.ProjectionMatrix();

	glUniformMatrix4fv(shader->loc_view_matrix, 1, GL_FALSE, glm::value_ptr(v));
	glUniformMatrix4fv(shader->loc_projection_matrix, 1, GL_FALSE, glm::value_ptr(p));
	glUniformMatrix4fv(shader->loc_model_matrix, 1, GL_FALSE, glm::value_ptr(modelMatrix));

	mesh->Render();
}

namespace tgl {
class Render3dSystem : public System {
  public:
	Render3dSystem(Scene *scene) : System(scene) {
		scene->OnUpdate().Subscribe([this](const float &e) { this->Update(e); });
	}

	void Update(float dt) {
		auto &data = m_scene->GetComponentData();
		auto cam_opt = m_scene->GetActiveCamera();
		if (!cam_opt)
			return;
		auto &cam = cam_opt.value();

		// World
		data.ForEach<RenderComponent>([&cam](Archetype<RenderComponent> entity) {
			if (!entity.GetMesh())
				return;
			auto transformComponent = entity.instance_.TryArche<Transform3dComponent>();
			if (!transformComponent)
				return;
			glm::mat4 model_matrix = transformComponent->GlobalTransform();

			GameManager &gm = GameManager::Get();
			RenderMesh3D(cam, entity.GetMesh(), gm.shaders[entity.GetShader()], model_matrix);
		});
	}
};
} // namespace tgl