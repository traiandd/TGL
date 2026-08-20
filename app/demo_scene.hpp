#pragma once

#include "tgl/game_manager.hpp"

using namespace tgl;

// A syntax tour of the engine: EntityBuilder in its different forms,
// mutating a component through the builder before committing it, and a
// parent/child relationship. See empty_scene.hpp/.cpp for the minimal
// starting point this builds on.
SceneId DemoScene();
