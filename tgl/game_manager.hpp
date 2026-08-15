#pragma once

#include <string>
#include <unordered_map>

#include "core/world.h"

#include "core/gpu/shader.h"
#include "glm/fwd.hpp"

#include "tgl/scene_manager.hpp"

namespace tgl {

class Scene;

class GameManager : public World {
  public:
	GameManager();
	GameManager(GameManager &) = delete;
	GameManager(GameManager &&) = delete;
	~GameManager();

	void Init() override;
	static GameManager &Get();
	std::unordered_map<std::string, Shader *> shaders;

	// Thin forwarding to SceneManager, kept so existing call sites (and the
	// Init/frame-loop/input code below) don't need to know it moved.
	SceneId AddScene(Scene &&s) { return SceneManager::Get().AddScene(std::move(s)); }
	void SetScene(SceneId id, Scene &&s) { SceneManager::Get().SetScene(id, std::move(s)); }
	Scene *GetScene(SceneId id) { return SceneManager::Get().GetScene(id); }
	void SetActiveScene(SceneId id) { SceneManager::Get().SetActiveScene(id); }
	Scene *GetActiveScene() const { return SceneManager::Get().GetActiveScene(); }
	SceneId GetNewSceneId() { return SceneManager::Get().GetNewSceneId(); }

  private:
	void FrameStart() override;
	void Update(float deltaTimeSeconds) override;
	void FrameEnd() override;

	void DrawScene(glm::mat3 visMatrix);

	void OnInputUpdate(float deltaTime, int mods) override;
	void OnKeyPress(int key, int mods) override;
	void OnKeyRelease(int key, int mods) override;
	void OnMouseMove(int mouseX, int mouseY, int deltaX, int deltaY) override;
	void OnMouseBtnPress(int mouseX, int mouseY, int button, int mods) override;
	void OnMouseBtnRelease(int mouseX, int mouseY, int button, int mods) override;
	void OnMouseScroll(int mouseX, int mouseY, int offsetX, int offsetY) override;
};
} // namespace tgl