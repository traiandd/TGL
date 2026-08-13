#pragma once

#include "component_data.hpp"
#include "core/window/window_object.h"
#include "entity.hpp"
#include "system/system.hpp"
#include "tgl/component/register_component.hpp"
#include "tgl/entity_id.hpp"
#include "tgl/game_manager.hpp"
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
	Scene(SceneId id) : id_(id) {};
	Scene(const Scene &) = delete; // prevent copy
	Scene &operator=(const Scene &) = delete;
	Scene(Scene &&);
	Scene &operator=(Scene &&) = delete;
	ComponentData &GetComponentData();

	void Update(float dt);

	EntityInstance AddEntity(Entity &&entity);

	template<typename T, typename... Args> T AddEntity(Entity &&entity, Args... args) {
		EntityId id = {m_next_entity_id++, id_};
		auto &components = entity.GetComponents();
		for (auto &[type, component] : components) {
			auto AddComponent = component_registry[type];
			AddComponent(m_data, id, component.get());
		}
		return T(EntityInstance(id), args...);
	}

	template<typename T> T AddEntity(Entity &&entity) {
		EntityId id = {m_next_entity_id++, id_};
		auto &components = entity.GetComponents();
		for (auto &[type, component] : components) {
			auto AddComponent = component_registry[type];
			AddComponent(m_data, id, component.get());
		}
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