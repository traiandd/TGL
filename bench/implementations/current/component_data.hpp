#pragma once

// Alias over the real, current tgl::ComponentData, so the benchmark always
// measures the actual production implementation instead of a frozen copy
// that could drift out of sync with it. As of the archetype/table storage
// rewrite, this is the archetype implementation - see
// bench/implementations/legacy (original std::unordered_map<type_index,...>
// lookup) and bench/implementations/dense_array (the TypeId-indexed
// std::vector<std::optional<T>> version that preceded archetype storage)
// for the earlier baselines this gets compared against.

#include "tgl/component_data.hpp"

namespace bench_impl {
namespace current {

using ComponentData = ::ComponentData;

} // namespace current
} // namespace bench_impl
