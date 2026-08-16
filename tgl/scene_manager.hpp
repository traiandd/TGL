#pragma once

#include <array>
#include <memory>

#include "tgl/entity_id.hpp"

namespace tgl {

class Scene;

// Fixed capacity rather than an unordered_map: EntityInstance::GetScene()
// resolves through GetScene() on every single component access across the
// engine, so this is a hot path - a direct array index is much cheaper than
// a hash lookup. Id 0 is reserved/always-empty (matches EntityId's
// {0,0}-is-invalid convention), so valid scene ids are 1..kMaxScenes.
constexpr size_t kMaxScenes = 16;

class SceneManager {
  public:
	SceneManager(SceneManager &) = delete;
	SceneManager(SceneManager &&) = delete;

	static SceneManager &Get();

	Scene &NewScene();
	void SetScene(SceneId id, Scene &&s);
	Scene *GetScene(SceneId id);
	void SetActiveScene(SceneId id);
	Scene *GetActiveScene() const;

  private:
	SceneManager() = default;

	SceneId current_scene_id_ = 1;
	std::array<std::unique_ptr<Scene>, kMaxScenes + 1> scenes_;
	Scene *active_scene_ = nullptr;
};

} // namespace tgl
