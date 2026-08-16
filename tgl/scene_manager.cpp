#include "tgl/scene_manager.hpp"

#include "scene.hpp"
#include <cassert>
#include <iostream>

using namespace std;
using namespace tgl;

SceneManager &tgl::SceneManager::Get() {
	static SceneManager instance;
	return instance;
}

Scene &tgl::SceneManager::NewScene() {
	SceneId id = current_scene_id_++;
	assert(id <= kMaxScenes && "SceneManager: exceeded fixed scene capacity (kMaxScenes)");
	scenes_[id] = make_unique<Scene>(id);
	return *scenes_[id];
}

void tgl::SceneManager::SetScene(SceneId id, Scene &&s) {
	assert(id <= kMaxScenes && "SceneManager: exceeded fixed scene capacity (kMaxScenes)");
	scenes_[id] = make_unique<Scene>(std::move(s));
}

Scene *tgl::SceneManager::GetScene(SceneId id) {
	assert(id < scenes_.size() && "SceneManager: scene id out of range");
	return scenes_[id].get();
}

Scene *tgl::SceneManager::GetActiveScene() const { return active_scene_; }

void tgl::SceneManager::SetActiveScene(SceneId id) {
	if (id < scenes_.size() && scenes_[id]) {
		active_scene_ = scenes_[id].get();
	} else {
		// cout << std::format("WARN: no scene with id {}. Silently
		// failing...", id);
	}
}
