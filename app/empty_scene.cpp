#include "empty_scene.hpp"

#include "core/engine.h"
#include "core/managers/resource_path.h"
#include "glm/trigonometric.hpp"
#include "tgl/component/3d_transform.hpp"
#include "tgl/component/render.hpp"
#include "tgl/entity.hpp"
#include "tgl/prefab/camera_3d.hpp"
#include "tgl/scene.hpp"
#include "tgl/system/batch_render_3d.hpp"
#include "tgl/system/render_3d.hpp"

SceneId EmptyScene() {
	GameManager &gm = GameManager::Get();
	SceneId scene_id = gm.AddScene(Scene());
	Scene *scene = gm.GetScene(scene_id);

	Camera3d camera = scene->AddEntity(Camera3d::New());
	camera.SetPosition(glm::vec3({0.f, 1.f, 0.f}));
	camera.RotatePitch(glm::radians(-22.5f));
	scene->SetActiveCamera(camera);

	Mesh *mesh = new Mesh("box");
	mesh->LoadMesh(PATH_JOIN(Engine::GetWindow()->props.selfDir, RESOURCE_PATH::MODELS, "primitives"), "box.obj");

	Entity cube;
	cube.AddComponent<Transform3dComponent>()->SetPosition({2, 0, 0})->RotateYaw(glm::radians(45.f));
	cube.AddComponent(RenderComponent(mesh, "Texture"));
	scene->AddEntity(std::move(cube));

	scene->AddSystem<Render3dSystem>();
	scene->AddSystem<BatchRender3dSystem>();

	return scene_id;
}
