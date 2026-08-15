#pragma once

#include <cstdint>
#include <functional>

typedef uint16_t SceneId;

namespace tgl {

typedef uint32_t LocalEntityId;

struct EntityId {
	LocalEntityId id;
	SceneId scene_id;

	bool operator==(const EntityId &other) const { return id == other.id && scene_id == other.scene_id; }
};
} // namespace tgl

template<> struct std::hash<tgl::EntityId> {
	using is_transparent = void; // optional

	size_t operator()(const tgl::EntityId &e) const noexcept { return std::hash<uint32_t>{}(e.id); }
};