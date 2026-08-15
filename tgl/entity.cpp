#include "entity.hpp"

#include "scene.hpp"
#include "tgl/component_data.hpp"
#include "tgl/scene_manager.hpp"
#include <cassert>
#include <iostream>

tgl::EntityInstance::EntityInstance(tgl::EntityId id) : id_(id) {}

ComponentData &tgl::EntityInstance::GetSceneData() const { return GetScene()->GetComponentData(); }

tgl::Scene *tgl::EntityInstance::GetScene() const {
	tgl::Scene *s = tgl::SceneManager::Get().GetScene(id_.scene_id);
	assert(s && "Scene of EntityInstace is invalid!");
	return s;
}