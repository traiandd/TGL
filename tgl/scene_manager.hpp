#pragma once

#include <memory>
#include <unordered_map>

#include "tgl/entity_id.hpp"

namespace tgl {

class Scene;

class SceneManager {
  public:
	SceneManager(SceneManager &) = delete;
	SceneManager(SceneManager &&) = delete;

	static SceneManager &Get();

	SceneId AddScene(Scene &&s);
	void SetScene(SceneId id, Scene &&s);
	Scene *GetScene(SceneId id);
	void SetActiveScene(SceneId id);
	Scene *GetActiveScene() const;
	SceneId GetNewSceneId();

  private:
	SceneManager() = default;

	SceneId current_scene_id_ = 1;
	std::unordered_map<SceneId, std::unique_ptr<Scene>> scenes_;
	Scene *active_scene_ = nullptr;
};

} // namespace tgl
