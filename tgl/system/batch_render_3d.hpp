#pragma once

#include "tgl/archetype.hpp"
#include "tgl/component/batch_render.hpp"
#include "tgl/entity.hpp"
#include "tgl/game_manager.hpp"
#include "../component/3d_transform.hpp"
#include "../component/3d_camera.hpp"
#include "../component/render.hpp"
#include "system.hpp"
#include "tgl/prefab/camera_3d.hpp"
#include "tgl/scene.hpp"
#include "tgl/core/batch.hpp"

#include <iostream>
#include <unordered_map>
#include <vector>
using namespace tgl;

namespace tgl {
class BatchRender3dSystem : public System {
	std::unordered_map<BatchIdentifier, StaticBatch> batches;

  public:
	BatchRender3dSystem(Scene *scene) : System(scene) {
		std::unordered_map<BatchIdentifier, std::vector<Archetype<Transform3dComponent>>> entities;

		auto &data = m_scene->GetComponentData();
		GameManager &gm = GameManager::Get();
		// TODO: Get all the entity with same mesh and shader and batch them together
		data.ForEach<BatchRenderComponent>([&entities, &gm](Archetype<BatchRenderComponent> entity) {
			if (!entity.GetMesh())
				return;
			auto transformComponent = entity.instance_.TryArche<Transform3dComponent>();
			if (!transformComponent)
				return;
			entities[{entity.GetMesh(), gm.shaders[entity.GetShader()]}].push_back(*transformComponent);
		});
		for (auto &[id, transforms] : entities) {
			batches.emplace(id, StaticBatch(id.mesh, id.shader, transforms));
		}
		scene->OnUpdate().Subscribe([this](const float &e) { this->Update(e); });
	}

	void Update(float dt) {
		auto &data = m_scene->GetComponentData();
		auto cam_opt = m_scene->GetActiveCamera();
		if (!cam_opt)
			return;

		auto &cam = cam_opt.value();

		for (auto &[id, batch] : batches) {
			auto shader = batch.shader;
			shader->Use();

			glm::mat4 v = cam.ViewMatrix();
			glm::mat4 p = cam.ProjectionMatrix();

			glUniformMatrix4fv(shader->loc_view_matrix, 1, GL_FALSE, glm::value_ptr(v));
			glUniformMatrix4fv(shader->loc_projection_matrix, 1, GL_FALSE, glm::value_ptr(p));

			batch.Render();
		}
	}
};
} // namespace tgl