#pragma once

#include "../entity.hpp"
#include "tgl/game_manager.hpp"
#include <optional>

namespace tgl {
class EntityInstance;
template<typename T, typename Derived> class BaseComponentHandle {
  public:
	static std::optional<Derived> TryCreate(tgl::EntityInstance instance) {
		auto &gm = tgl::GameManager::Get();
		if (instance.Get<T>())
			return Derived(instance);
		return std::nullopt;
	}

	static std::optional<Derived> TryCreate(EntityId id, ComponentData *data) {
		auto instance = EntityInstance(id);
		return TryCreate(instance);
	}

	T &Get() const { return m_instance.Get<T>(); }

  private:
	BaseComponentHandle(tgl::EntityInstance instance) : m_instance(instance) {}

  protected:
	tgl::EntityInstance m_instance;
};
} // namespace tgl