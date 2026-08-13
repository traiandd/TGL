#pragma once

#include "../game_manager.hpp"
#include "../component/2d_transform.hpp"
#include "../component/camera.hpp"
#include "../component/render.hpp"
#include "../component/ui_render.hpp"
#include "system.hpp"
#include "tgl/component/camera.hpp"
#include <iostream>
#include "tgl/scene.hpp"

using namespace tgl;

void RenderMesh2D(EntityInstance camera, Mesh *mesh, Shader *shader, const glm::mat3 &modelMatrix, const glm::mat4 &viewMatrix) {
	if (!mesh || !shader || !shader->program)
		return;

	shader->Use();
	auto cam = camera.Get<CameraComponent>();
	if (!cam) {
		std::cout << "waht\n";
		return;
	}
	glm::mat4 v = viewMatrix;
	glm::mat4 p = cam->ProjectionMatrix();

	glUniformMatrix4fv(shader->loc_view_matrix, 1, GL_FALSE, glm::value_ptr(v));
	glUniformMatrix4fv(shader->loc_projection_matrix, 1, GL_FALSE, glm::value_ptr(p));

	glm::mat3 mm = modelMatrix;
	glm::mat4 model = glm::mat4(mm[0][0], mm[0][1], mm[0][2], 0.f, mm[1][0], mm[1][1], mm[1][2], 0.f, 0.f, 0.f, mm[2][2], 0.f, mm[2][0], mm[2][1], 0.f, 1.f);

	glUniformMatrix4fv(shader->loc_model_matrix, 1, GL_FALSE, glm::value_ptr(model));

	mesh->Render();
}

void RenderWorldMesh2D(EntityInstance camera, Mesh *mesh, Shader *shader, const glm::mat3 &modelMatrix) {
	auto pos = camera.Get<Transform2dComponent>();
	if (!pos) {
		std::cout << "waht\n";
		return;
	}
	RenderMesh2D(camera, mesh, shader, modelMatrix, pos->TransformMatrix());
}

void RenderUiMesh2D(EntityInstance camera, Mesh *mesh, Shader *shader, const glm::mat3 &modelMatrix) {
	glm::mat4 view = glm::mat4(1);
	auto cam = camera.Get<CameraComponent>();
	if (!cam) {
		std::cout << "waht\n";
		return;
	}
	auto sizes = cam->GetSizes();
	// flipping y coord

	view[3][0] = -sizes.x / 2;
	view[3][1] = sizes.y / 2;

	glm::mat3 model = modelMatrix;
	model[2][1] *= -1;
	RenderMesh2D(camera, mesh, shader, model, view);
}

namespace tgl {
class RenderSystem : public System {
  public:
	RenderSystem(Scene *scene) : System(scene) {
		scene->OnUpdate().Subscribe([this](const float &e) { this->Update(e); });
	}

	void Update(float dt) {
		auto &data = m_scene->GetComponentData();
		auto cam_opt = m_scene->GetActiveCamera();
		if (!cam_opt)
			return;
		auto &cam = cam_opt.value();

		// World
		auto &renderComponents = data.GetAll<RenderComponent>();
		for (auto &[entity, renderComponent] : renderComponents) {
			if (!renderComponent.mesh)
				continue;
			auto transformComponent = EntityInstance(entity).TryArche<Transform2dComponent>();
			if (!transformComponent)
				continue;
			glm::mat3 model_matrix = transformComponent->GlobalTransform();

			GameManager &gm = GameManager::Get();

			RenderWorldMesh2D(cam, renderComponent.mesh, gm.shaders[renderComponent.shader], model_matrix);
		}
	}
};
} // namespace tgl