#pragma once

#include <functional>
#include "component.hpp"

#include "../component_data.hpp"
#include <unordered_map>
#include <typeindex>
#include <utility>

inline std::unordered_map<std::type_index, std::function<void(ComponentData &, tgl::EntityId, Component *)>> component_registry;

template<typename T> constexpr auto ComponentRegistration() {
	return [](ComponentData &data, tgl::EntityId e, Component *c) { data.AddComponent(e, std::move(*static_cast<T *>(c))); };
}

template<typename T> struct ComponentRegistrar {
	ComponentRegistrar() { component_registry[typeid(T)] = ComponentRegistration<T>(); }
};

#define register_component(name) static ComponentRegistrar<name> reg_##name