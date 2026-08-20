#pragma once

#include <tuple>
#include <type_traits>
#include <utility>

#include "component_data.hpp"

namespace entity_builder_detail {
template<typename T, typename... Pack> constexpr bool kIsOneOf = (std::is_same_v<T, Pack> || ...);
}

namespace tgl {

// Compile-time-typed replacement for the old type-erased Entity: each Add
// accumulates one more component into the type, so the full component set
// is known statically by the time Scene::AddEntity commits it - letting
// that commit insert directly into the right ArchetypeTable in one shot
// instead of transitioning through N intermediate tables.
template<typename... Components> class EntityBuilder {
  public:
	EntityBuilder() = default;
	explicit EntityBuilder(std::tuple<Components...> data) : data_(std::move(data)) {}

	template<typename T, typename... Args>
		requires(!entity_builder_detail::kIsOneOf<T, Components...>)
	EntityBuilder<Components..., T> Add(Args &&...args) && {
		return EntityBuilder<Components..., T>(std::tuple_cat(std::move(data_), std::make_tuple(T(std::forward<Args>(args)...))));
	}

	template<typename T>
		requires(!entity_builder_detail::kIsOneOf<T, Components...>)
	EntityBuilder<Components..., T> Add(T value) && {
		return EntityBuilder<Components..., T>(std::tuple_cat(std::move(data_), std::make_tuple(std::move(value))));
	}

	// Only meaningful on a builder you still hold - & qualified so it can't
	// be called on a temporary and hand back a reference into storage that's
	// about to be destroyed.
	template<typename T>
		requires entity_builder_detail::kIsOneOf<T, Components...>
	T &Get() & {
		return std::get<T>(data_);
	}

	std::tuple<Components...> &Data() { return data_; }

  private:
	std::tuple<Components...> data_;
};

} // namespace tgl
