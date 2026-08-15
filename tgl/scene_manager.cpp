#include "tgl/scene_manager.hpp"

#include "scene.hpp"
#include <iostream>

using namespace std;
using namespace tgl;

SceneManager &tgl::SceneManager::Get() {
	static SceneManager instance;
	return instance;
}

SceneId tgl::SceneManager::AddScene(Scene &&s) {
	if (s.GetId() == 0) {
		s.SetId(current_scene_id_);
		scenes_[current_scene_id_] = make_unique<Scene>(std::move(s));
		return current_scene_id_++;
	} else {
		scenes_[s.GetId()] = make_unique<Scene>(std::move(s));
		return s.GetId();
	}
}

void tgl::SceneManager::SetScene(SceneId id, Scene &&s) { scenes_[id] = make_unique<Scene>(std::move(s)); }

Scene *tgl::SceneManager::GetScene(SceneId id) { return scenes_[id].get(); }

Scene *tgl::SceneManager::GetActiveScene() const { return active_scene_; }

SceneId tgl::SceneManager::GetNewSceneId() { return current_scene_id_++; }

void tgl::SceneManager::SetActiveScene(SceneId id) {
	if (scenes_.count(id)) {
		active_scene_ = scenes_[id].get();
	} else {
		// cout << std::format("WARN: no scene with id {}. Silently
		// failing...", id);
	}
}
