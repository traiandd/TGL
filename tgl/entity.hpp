#pragma once

#include <optional>
#include <unordered_map>
#include <typeindex>
#include <memory>
#include "component/component.hpp"
#include "tgl/component_data.hpp"
#include "tgl/game_manager.hpp"
#include "entity_id.hpp"

namespace tgl {
class Scene;

template<typename T> class ComponentHandle;

template<typename... Components> class Archetype;

class EntityInstance {
  public:
	EntityInstance(EntityId id);
	EntityInstance() : id_({0, 0}) {};

	template<typename T> EntityInstance *Add(const T &component) {
		GetSceneData().AddComponent(id_, component);
		return this;
	}
	EntityId GetId() const { return id_; }

	ComponentData &GetSceneData() const;
	Scene *GetScene() const;

	bool operator==(const EntityInstance &other) const { return id_ == other.id_; }

	template<typename T> T *Get() const { return GetSceneData().GetComponent<T>(id_); }

	template<typename... T> Archetype<T...> GetArche() const { return Archetype<T...>(*this); }
	template<typename... T> std::optional<Archetype<T...>> TryArche() const { return Archetype<T...>::TryFrom(*this); }
	bool operator<(const EntityInstance &other) const { return id_.id < other.id_.id; }

  private:
	EntityId id_;
};

class Entity {
  public:
	template<typename T> T *AddComponent(T component) {
		std::type_index type = typeid(T);
		auto it = m_components.find(type);
		if (it == m_components.end()) {
			m_components[type] = std::make_unique<T>(std::move(component));
		}
		return static_cast<T *>(m_components[type].get());
	}

	template<typename T> T *AddComponent() { return AddComponent(T()); }
	template<typename T> T *Get() {
		std::type_index type = typeid(T);

		return static_cast<T *>(m_components[type].get());
	}

	std::unordered_map<std::type_index, std::unique_ptr<Component>> &GetComponents() { return m_components; }

  private:
	std::unordered_map<std::type_index, std::unique_ptr<Component>> m_components;
};
} // namespace tgl