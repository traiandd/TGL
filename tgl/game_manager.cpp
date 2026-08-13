#include "game_manager.hpp"

#include "core/managers/resource_path.h"
#include "scene.hpp"

using namespace std;
using namespace tgl;

GameManager &tgl::GameManager::Get() {
	static GameManager instance;
	return instance;
}

GameManager::GameManager() {}

GameManager::~GameManager() {}

void GameManager::Init() {
	{
		Shader *shader = new Shader("VertexColor");
		shader->AddShader(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::SHADERS, "MVP.Texture.VS.glsl"), GL_VERTEX_SHADER);
		shader->AddShader(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::SHADERS, "VertexColor.FS.glsl"), GL_FRAGMENT_SHADER);
		shader->CreateAndLink();
		shaders[shader->GetName()] = shader;
	}
	{
		Shader *shader = new Shader("VertexColorInstanced");
		shader->AddShader(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::SHADERS, "MVP.TextureInstance.VS.glsl"), GL_VERTEX_SHADER);
		shader->AddShader(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::SHADERS, "VertexColor.FS.glsl"), GL_FRAGMENT_SHADER);
		shader->CreateAndLink();
		shaders[shader->GetName()] = shader;
	}
	{
		Shader *shader = new Shader("Texture");
		shader->AddShader(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::SHADERS, "MVP.Texture.VS.glsl"), GL_VERTEX_SHADER);
		shader->AddShader(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::SHADERS, "Default.FS.glsl"), GL_FRAGMENT_SHADER);
		shader->CreateAndLink();
		shaders[shader->GetName()] = shader;
	}
	{
		Shader *shader = new Shader("TextureInstance");
		shader->AddShader(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::SHADERS, "MVP.TextureInstance.VS.glsl"), GL_VERTEX_SHADER);
		shader->AddShader(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::SHADERS, "Default.FS.glsl"), GL_FRAGMENT_SHADER);
		shader->CreateAndLink();
		shaders[shader->GetName()] = shader;
	}
}

void GameManager::FrameStart() {
	// Clears the color buffer (using the previously set color) and depth buffer
	glClearColor(0.2, 0.1, 0.6, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GameManager::Update(float deltaTimeSeconds) {
	active_scene_->OnUpdate().Update(deltaTimeSeconds);

	active_scene_->FlushDeleteQueue();
}

void GameManager::FrameEnd() {}

void GameManager::DrawScene(glm::mat3 visMatrix) {}

void GameManager::OnInputUpdate(float deltaTime, int mods) { active_scene_->OnInputUpdate().Update({mods, deltaTime, window, active_scene_}); }

void GameManager::OnKeyPress(int key, int mods) {}

void GameManager::OnKeyRelease(int key, int mods) {}

void GameManager::OnMouseMove(int mouseX, int mouseY, int deltaX, int deltaY) { active_scene_->OnMouseMove().Update({mouseX, mouseY, deltaX, deltaY}); }

void GameManager::OnMouseBtnPress(int mouseX, int mouseY, int button, int mods) {
	MouseClickEvent event = {glm::vec2(mouseX, mouseY), button, mods};
	active_scene_->OnMouseDown().Update(event);
}

void GameManager::OnMouseBtnRelease(int mouseX, int mouseY, int button, int mods) {
	MouseClickEvent event = {glm::vec2(mouseX, mouseY), button, mods};
	active_scene_->OnMouseUp().Update(event);
}

void GameManager::OnMouseScroll(int mouseX, int mouseY, int offsetX, int offsetY) {}

SceneId tgl::GameManager::AddScene(Scene &&s) {
	cout << "Adding scene " << s.GetId() << "\n";
	if (s.GetId() == 0) {
		s.SetId(current_scene_id_);
		scenes_[current_scene_id_] = make_unique<Scene>(std::move(s));
		return current_scene_id_++;
	} else {
		scenes_[s.GetId()] = make_unique<Scene>(std::move(s));
		return s.GetId();
	}
}

void tgl::GameManager::SetScene(SceneId id, Scene &&s) { scenes_[id] = make_unique<Scene>(std::move(s)); }

Scene *tgl::GameManager::GetScene(SceneId id) { return scenes_[id].get(); }

Scene *tgl::GameManager::GetActiveScene() const { return active_scene_; }

SceneId tgl::GameManager::GetNewSceneId() { return current_scene_id_++; }

void tgl::GameManager::SetActiveScene(SceneId name) {
	if (scenes_.count(name)) {
		active_scene_ = scenes_[name].get();
	} else {
		// cout << std::format("WARN: no scene with name {}. Silently
		// failing...", name);
	}
}
