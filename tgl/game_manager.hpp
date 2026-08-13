#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "core/world.h"

#include "core/gpu/shader.h"
#include "glm/fwd.hpp"

typedef uint16_t SceneId;

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

	SceneId AddScene(Scene &&s);
	void SetScene(SceneId id, Scene &&s);
	Scene *GetScene(SceneId id);
	void SetActiveScene(SceneId name);
	Scene *GetActiveScene() const;
	SceneId GetNewSceneId();

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

	SceneId current_scene_id_ = 1;
	std::unordered_map<SceneId, std::unique_ptr<Scene>> scenes_;
	Scene *active_scene_ = nullptr;
};
} // namespace tgl