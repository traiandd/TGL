#pragma once

#include <memory>
#include <typeindex>
#include <unordered_map>
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
	virtual void Remove(tgl::EntityId e) = 0;
};

template<typename T> class ComponentStorage : public IComponentStorage {
  public:
	void Add(tgl::EntityId e, const T &component) {
		if (e.id >= components.size()) {
			components.resize(e.id + 1);
			hasComponent.resize(e.id + 1);
		}
		components[e.id].emplace(component);
		hasComponent[e.id] = e.scene_id;
	}

	void Remove(tgl::EntityId e) {
		if (e.id < hasComponent.size()) {
			components[e.id].reset();
			hasComponent[e.id] = 0;
		}
	}

	T *Get(tgl::EntityId e) { return (e.id < hasComponent.size() && hasComponent[e.id] && components[e.id].has_value()) ? &*components[e.id] : nullptr; }

	template<typename Func> void ForEach(Func f) {
		for (uint32_t e = 0; e < components.size(); e++)
			if (hasComponent[e] && components[e])
				f(tgl::Archetype<T>(tgl::EntityId(e, hasComponent[e])));
	}

  private:
	std::vector<std::optional<T>> components;
	std::vector<SceneId> hasComponent;
};

class ComponentData {
  public:
	ComponentData() {}

	ComponentData(ComponentData &&) = default;

	template<typename T> void AddComponent(tgl::EntityId e, const T &component) { GetOrCreateStorage<T>().Add(e, component); }

	template<typename T> void RemoveComponent(tgl::EntityId e) {
		if (auto *storage = GetStorage<T>()) {
			storage->Remove(e);
		}
	}

	void RemoveEntityData(tgl::EntityId e) {
		for (auto [type, storage] : componentStores) {
			storage->Remove(e);
		}
	}

	template<typename T> T *GetComponent(tgl::EntityId e) const {
		if (auto *storage = GetStorage<T>()) {
			return storage->Get(e);
		}
		return nullptr;
	}

	template<typename T> std::optional<tgl::ComponentHandle<T>> GetComponentHandle(tgl::EntityId e) { return tgl::ComponentHandle<T>::TryCreate(e, this); }

	template<typename T, typename Func> void ForEach(Func f) { return GetOrCreateStorage<T>().ForEach(f); }

	size_t size() { return componentStores.size(); }

  private:
	typedef std::unordered_map<std::type_index, std::shared_ptr<IComponentStorage>> te;
	te componentStores;

	template<typename T> ComponentStorage<T> &GetOrCreateStorage() {
		std::type_index type = typeid(T);
		auto it = componentStores.find(type);
		if (it == componentStores.end()) {
			auto e = std::make_shared<ComponentStorage<T>>();
			componentStores.insert<te::value_type>(te::value_type(type, std::move(e)));
		}
		return *static_cast<ComponentStorage<T> *>(componentStores[type].get());
	}

	template<typename T> ComponentStorage<T> *GetStorage() const {
		std::type_index type = typeid(T);
		auto it = componentStores.find(type);
		if (it == componentStores.end())
			return nullptr;
		return static_cast<ComponentStorage<T> *>(it->second.get());
	}
};