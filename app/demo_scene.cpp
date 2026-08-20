#include "demo_scene.hpp"

#include "core/engine.h"
#include "core/managers/resource_path.h"
#include "glm/trigonometric.hpp"
#include "tgl/component/3d_transform.hpp"
#include "tgl/component/children.hpp"
#include "tgl/component/render.hpp"
#include "tgl/entity.hpp"
#include "tgl/entity_builder.hpp"
#include "tgl/prefab/camera_3d.hpp"
#include "tgl/scene.hpp"
#include "tgl/system/render_3d.hpp"

namespace {
Mesh *LoadPrimitive(const char *name, const char *file) {
	Mesh *mesh = new Mesh(name);
	mesh->LoadMesh(PATH_JOIN(Engine::GetWindow()->props.selfDir, RESOURCE_PATH::MODELS, "primitives"), file);
	return mesh;
}
} // namespace

SceneId DemoScene() {
	GameManager &gm = GameManager::Get();
	Scene *scene = &gm.NewScene();

	// Prefab entities (Camera3d) go through Scene::AddEntity<T>, which
	// commits the builder and hands back the concrete wrapper type - as
	// opposed to plain Scene::AddEntity(builder), which returns a bare
	// Archetype<Components...> (used below for everything else).
	Camera3d camera = scene->AddEntity<Camera3d>(Camera3d::New());
	camera.SetPosition(glm::vec3({0.f, 4.f, 12.f}));
	camera.RotatePitch(glm::radians(-15.f));
	scene->SetActiveCamera(camera);

	Mesh *cube_mesh = LoadPrimitive("box", "box.obj");
	Mesh *sphere_mesh = LoadPrimitive("sphere", "sphere.obj");

	// EntityBuilder<>().Add<T>() default-constructs T and returns a builder
	// typed one component wider - EntityBuilder<Transform3dComponent>, then
	// EntityBuilder<Transform3dComponent, RenderComponent> after the second
	// Add. The compiler tracks the exact component set through the chain.
	auto ground = tgl::EntityBuilder<>().Add<Transform3dComponent>().Add<RenderComponent>(cube_mesh, "Texture");
	// Get<T>() hands back a live reference into the not-yet-committed
	// builder, so a component can be finished configuring before AddEntity
	// ever touches ComponentData.
	Transform3dComponent &ground_transform = ground.Get<Transform3dComponent>();
	ground_transform.SetScale({10.f, 0.2f, 10.f});
	ground_transform.SetPosition({0.f, -1.f, 0.f});
	scene->AddEntity(std::move(ground));

	// A row of cubes - same builder shape, built inside ordinary control
	// flow, one positioned differently each time.
	for (int i = -2; i <= 2; i++) {
		// .Add(value) takes an already-constructed component instead of
		// forwarding constructor args - equivalent to .Add<RenderComponent>
		// above, just handed a value that already exists.
		auto cube = tgl::EntityBuilder<>().Add<Transform3dComponent>().Add(RenderComponent(cube_mesh, "Texture"));
		cube.Get<Transform3dComponent>().SetPosition({i * 2.5f, 0.f, 0.f});
		scene->AddEntity(std::move(cube));
	}

	// Parent/child: Children::AddChild (component/children.hpp) wires up
	// DeleteListener + Parent bookkeeping on the child; Transform3dComponent
	// ::GlobalTransform() then walks the Parent chain automatically, so the
	// satellite follows wherever the hub goes.
	auto hub_builder = tgl::EntityBuilder<>().Add<Transform3dComponent>().Add<Children>().Add(RenderComponent(sphere_mesh, "Texture"));
	hub_builder.Get<Transform3dComponent>().SetPosition({0.f, 3.f, 0.f});
	auto hub = scene->AddEntity(std::move(hub_builder));

	auto satellite_builder = tgl::EntityBuilder<>().Add<Transform3dComponent>().Add(RenderComponent(cube_mesh, "Texture"));
	satellite_builder.Get<Transform3dComponent>().SetScale({0.4f, 0.4f, 0.4f});
	satellite_builder.Get<Transform3dComponent>().SetPosition({1.5f, 0.f, 0.f});
	auto satellite = scene->AddEntity(std::move(satellite_builder));
	hub.AddChild(satellite); // Archetype<...> -> EntityInstance conversion happens implicitly here.

	scene->AddSystem<Render3dSystem>();

	return scene->GetId();
}
