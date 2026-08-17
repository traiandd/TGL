#pragma once

#include "component_data.hpp"
#include "core/window/window_object.h"
#include "entity.hpp"
#include "system/system.hpp"
#include "tgl/entity_builder.hpp"
#include "tgl/entity_id.hpp"
#include "tgl/observer.hpp"
#include "tgl/prefab/camera_3d.hpp"

#include <cstdint>
#include <optional>
#include <queue>
#include <vector>

struct MouseMoveEvent {
	int mouseX;
	int mouseY;
	int deltaX;
	int deltaY;
};

struct MouseClickEvent {
	glm::vec2 screen_coords;
	int button;
	int mods;
};
struct KeyUpdateEvent {
	int mods;
	float dt;
	WindowObject *window;
	tgl::Scene *scene;
};
// TODO: optimize all entity ids in scene to local ids and add global id
namespace tgl {
class Scene {
  public:
	Scene();
	Scene(SceneId id) : id_(id), m_data(id) {};
	Scene(const Scene &) = delete; // prevent copy
	Scene &operator=(const Scene &) = delete;
	Scene(Scene &&);
	Scene &operator=(Scene &&) = delete;
	ComponentData &GetComponentData();

	void Update(float dt);

	// Commits a builder's accumulated components straight into their final
	// ArchetypeTable (see ComponentData::CreateEntity) and returns a view
	// over the exact component set the builder was typed with.
	template<typename... Components> Archetype<Components...> AddEntity(EntityBuilder<Components...> &&builder) {
		EntityId id = {m_next_entity_id++, id_};
		std::apply([&](Components &...comps) { m_data.CreateEntity(id, std::move(comps)...); }, builder.Data());
		return Archetype<Components...>(EntityInstance(id));
	}

	// Same commit, but for prefab wrapper types (e.g. Camera3d) that extend
	// an Archetype<...> with extra behavior - T must be given explicitly
	// since it can't be deduced from the builder alone.
	template<typename T, typename... Components> T AddEntity(EntityBuilder<Components...> &&builder) {
		EntityId id = {m_next_entity_id++, id_};
		std::apply([&](Components &...comps) { m_data.CreateEntity(id, std::move(comps)...); }, builder.Data());
		return T(EntityInstance(id));
	}

	EntityInstance GetEntity(EntityId i);

	void DeleteEntity(EntityInstance i);

	void FlushDeleteQueue();

	void SetActiveCamera(Camera3d camera) { m_active_camera.emplace(camera); }

	std::optional<Camera3d> GetActiveCamera() {
		if (m_active_camera) {
			return *m_active_camera;
		}
		return std::nullopt;
	}
	template<typename T, typename... Args> void AddSystem(Args &&...args) { m_systems.push_back(std::make_unique<T>(this, std::forward<Args>(args)...)); }

	Observer<MouseMoveEvent> &OnMouseMove() { return mouseMove; }

	Observer<float> &OnUpdate() { return update; }
	Observer<MouseClickEvent> &OnMouseDown() { return mouseDown; }
	Observer<MouseClickEvent> &OnMouseUp() { return mouseUp; }
	Observer<KeyUpdateEvent> &OnInputUpdate() { return inputUpdate; }
	SceneId GetId() const { return id_; }
	void SetId(SceneId id) {
		id_ = id;
		m_data.SetSceneId(id);
		std::cout << "set id to " << id << "\n";
	}

  private:
	SceneId id_ = 0;
	Observer<KeyUpdateEvent> inputUpdate;
	Observer<MouseMoveEvent> mouseMove;
	Observer<MouseClickEvent> mouseDown;
	Observer<MouseClickEvent> mouseUp;
	Observer<float> update;

	ComponentData m_data;
	std::vector<std::unique_ptr<tgl::System>> m_systems;
	std::optional<Camera3d> m_active_camera = std::optional<Camera3d>();
	std::queue<EntityInstance> m_delete_queue;
	uint32_t m_next_entity_id = 0;
};
} // namespace tgl