#pragma once

#include "../game_manager.hpp"
#include "../component/2d_transform.hpp"
#include "../component/camera.hpp"
#include "../component/render.hpp"
#include "../component/ui_render.hpp"
#include "components/text_renderer.h"
#include "core/engine.h"
#include "core/managers/resource_path.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/fwd.hpp"
#include "system.hpp"
#include "tgl/archetype.hpp"
#include "tgl/component/camera.hpp"
#include <iostream>
#include "tgl/component/ui_text.hpp"
#include "tgl/entity.hpp"
#include "tgl/scene.hpp"

using namespace tgl;

void RenderMesh2D(Mesh *mesh, Shader *shader, const glm::mat3 &modelMatrix, const glm::mat4 &viewMatrix) {
	if (!mesh || !shader || !shader->program)
		return;

	shader->Use();
	auto sizes = Engine::GetWindow()->props.resolution;

	glm::mat4 v = viewMatrix;
	// glm::mat4 p = glm::ortho(0, sizes.x, 0, sizes.y);
	// glm::mat4 p = glm::mat4(1);
	glm::mat4 p(1.0f);
	p[0][0] = 2.0f / sizes.x;
	p[1][1] = 2.0f / sizes.y;
	p[2][2] = -1.0f;
	p[3][3] = 1.0f;
	glUniformMatrix4fv(shader->loc_view_matrix, 1, GL_FALSE, glm::value_ptr(v));
	glUniformMatrix4fv(shader->loc_projection_matrix, 1, GL_FALSE, glm::value_ptr(p));

	glm::mat3 mm = modelMatrix;
	glm::mat4 model = glm::mat4(mm[0][0], mm[0][1], mm[0][2], 0.f, mm[1][0], mm[1][1], mm[1][2], 0.f, 0.f, 0.f, mm[2][2], 0.f, mm[2][0], mm[2][1], 0.f, 1.f);

	glUniformMatrix4fv(shader->loc_model_matrix, 1, GL_FALSE, glm::value_ptr(model));

	mesh->Render();
}

void RenderUiMesh2D(Mesh *mesh, Shader *shader, const glm::mat3 &modelMatrix) {
	glm::mat4 view = glm::mat4(1);
	auto sizes = Engine::GetWindow()->props.resolution;
	// flipping y coord

	view[3][0] = -sizes.x / 2.f;
	view[3][1] = sizes.y / 2.f;

	glm::mat3 model = modelMatrix;
	model[2][1] *= -1;
	RenderMesh2D(mesh, shader, model, view);
}

namespace tgl {
class RenderUiSystem : public System {
	gfxc::TextRenderer renderer =
		gfxc::TextRenderer(Engine::GetWindow()->props.selfDir, Engine::GetWindow()->props.resolution.x, Engine::GetWindow()->props.resolution.y);

  public:
	RenderUiSystem(Scene *scene) : System(scene) {
		scene->OnUpdate().Subscribe([this](const float &e) { this->Update(e); });
		auto props = Engine::GetWindow()->props;
		renderer.Load(PATH_JOIN(props.selfDir, RESOURCE_PATH::FONTS, "Hack-Bold.ttf"), 48);
	}

	void Update(float dt) {
		auto &data = m_scene->GetComponentData();

		data.ForEach<UIRenderComponent>([](Archetype<UIRenderComponent> entity) {
			if (!entity.GetMesh())
				return;

			auto transformComponent = EntityInstance(entity).TryArche<Transform2dComponent>();
			if (!transformComponent)
				return;

			glm::mat3 model_matrix = transformComponent->GlobalTransform();
			GameManager &gm = GameManager::Get();
			RenderUiMesh2D(entity.GetMesh(), gm.shaders[entity.GetShader()], model_matrix);
		});
		data.ForEach<UITextComponent>([this](Archetype<UITextComponent> entity) {
			auto transformComponent = EntityInstance(entity).TryArche<Transform2dComponent>();
			if (!transformComponent)
				return;
			auto pos = transformComponent->GlobalTransform();
			renderer.RenderText(entity.GetText(), pos[3][0], pos[3][1], transformComponent->GetScale().x, entity.GetTextColor());
		});
	}
};
} // namespace tgl