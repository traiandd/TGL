#pragma once

// Alias over the real, current tgl::ComponentData/ComponentStorage, so the
// benchmark always measures the actual production implementation instead of
// a frozen copy that could drift out of sync with it. See
// bench/implementations/legacy for the pre-refactor baseline this gets
// compared against.

#include "tgl/component_data.hpp"

namespace bench_impl {
namespace current {

using ComponentData = ::ComponentData;
template<typename T> using ComponentStorage = ::ComponentStorage<T>;

} // namespace current
} // namespace bench_impl
