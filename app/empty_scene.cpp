#include "empty_scene.hpp"

#include "tgl/prefab/camera_3d.hpp"
#include "tgl/scene.hpp"
#include "tgl/system/batch_render_3d.hpp"
#include "tgl/system/render_3d.hpp"

SceneId EmptyScene() {
	GameManager &gm = GameManager::Get();
	SceneId scene_id = gm.AddScene(Scene());
	Scene *scene = gm.GetScene(scene_id);

	Camera3d camera = scene->AddEntity(Camera3d::New());
	scene->SetActiveCamera(camera);

	scene->AddSystem<Render3dSystem>();
	scene->AddSystem<BatchRender3dSystem>();

	return scene_id;
}
