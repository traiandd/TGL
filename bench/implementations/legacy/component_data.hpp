#pragma once

// Frozen snapshot of ComponentData/ComponentStorage as they were before the
// type lookup was switched from std::unordered_map<std::type_index, ...> to
// a dense, per-type-index vector (see bench/implementations/current for the
// live version this is compared against). Kept only for benchmarking - not
// wired into the engine, and intentionally not updated to track further
// engine changes.
//
// ForEach/GetComponentHandle are dropped from this snapshot: both need
// tgl::Archetype<T>/tgl::ComponentHandle<T>, whose construction chains
// through EntityInstance::GetScene() -> GameManager::Get() -> a real
// window, which isn't available in the benchmark binary - same constraint
// the current implementation's benchmark is under.

#include "tgl/entity_id.hpp"

#include <memory>
#include <optional>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace bench_impl {
namespace legacy {

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

} // namespace legacy
} // namespace bench_impl
