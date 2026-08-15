#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "entity_id.hpp"
#include "glm/fwd.hpp"
#include "tgl/game_manager.hpp"

namespace tgl {

class EntityInstance;
template<typename T> class ComponentHandle;
template<typename... Components> class Archetype;
}; // namespace tgl

struct IComponentStorage {
	virtual ~IComponentStorage() = default;
	virtual void Remove(tgl::LocalEntityId id) = 0;
};

template<typename T> class ComponentStorage : public IComponentStorage {
  public:
	void Add(tgl::LocalEntityId id, const T &component) {
		if (id >= components.size())
			components.resize(id + 1);
		components[id].emplace(component);
	}

	void Remove(tgl::LocalEntityId id) {
		if (id < components.size())
			components[id].reset();
	}

	T *Get(tgl::LocalEntityId id) { return (id < components.size() && components[id].has_value()) ? &*components[id] : nullptr; }

	template<typename Func> void ForEach(SceneId scene_id, Func f) {
		for (uint32_t e = 0; e < components.size(); e++)
			if (components[e])
				f(tgl::Archetype<T>(tgl::EntityId(e, scene_id)));
	}

  private:
	std::vector<std::optional<T>> components;
};

class ComponentData {
  public:
	ComponentData(SceneId id = 0) : scene_id(id) {}

	ComponentData(ComponentData &&) = default;

	void SetSceneId(SceneId id) { scene_id = id; }

	template<typename T> void AddComponent(tgl::EntityId e, const T &component) { GetOrCreateStorage<T>().Add(e.id, component); }

	template<typename T> void RemoveComponent(tgl::EntityId e) {
		if (auto *storage = GetStorage<T>()) {
			storage->Remove(e.id);
		}
	}

	void RemoveEntityData(tgl::EntityId e) {
		for (auto &storage : component_stores) {
			if (storage)
				storage->Remove(e.id);
		}
	}

	template<typename T> T *GetComponent(tgl::EntityId e) const {
		if (auto *storage = GetStorage<T>()) {
			return storage->Get(e.id);
		}
		return nullptr;
	}
	// would component handles be needed?
	template<typename T> std::optional<tgl::ComponentHandle<T>> GetComponentHandle(tgl::EntityId e) { return tgl::ComponentHandle<T>::TryCreate(e, this); }

	template<typename T, typename Func> void ForEach(Func f) {
		if (auto *storage = GetStorage<T>()) {
			storage->ForEach(scene_id, f);
		}
	}

	size_t size() {
		size_t count = 0;
		for (auto &storage : component_stores)
			count += storage != nullptr;
		return count;
	}

  private:
	SceneId scene_id = 0;
	std::vector<std::unique_ptr<IComponentStorage>> component_stores;

	// Assigns each component type a dense, program-wide index on first use
	template<typename T> static size_t TypeId() {
		static size_t id = nextTypeId++;
		return id;
	}
	static inline size_t nextTypeId = 0;

	template<typename T> ComponentStorage<T> &GetOrCreateStorage() {
		size_t id = TypeId<T>();
		if (id >= component_stores.size())
			component_stores.resize(id + 1);
		if (!component_stores[id])
			component_stores[id] = std::make_unique<ComponentStorage<T>>();
		return *static_cast<ComponentStorage<T> *>(component_stores[id].get());
	}

	template<typename T> ComponentStorage<T> *GetStorage() const {
		size_t id = TypeId<T>();
		if (id >= component_stores.size() || !component_stores[id])
			return nullptr;
		return static_cast<ComponentStorage<T> *>(component_stores[id].get());
	}
};