#pragma once

#include <optional>
#include <utility>
#include "tgl/component_data.hpp"
#include "entity_id.hpp"

namespace tgl {
class Scene;

template<typename... Components> class Archetype;

class EntityInstance {
  public:
	EntityInstance(EntityId id);
	EntityInstance() : id_({0, 0}) {};

	template<typename T> EntityInstance *Add(T component) {
		GetSceneData().AddComponent(id_, std::move(component));
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
} // namespace tgl