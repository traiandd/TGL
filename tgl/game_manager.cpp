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
	Scene *active_scene = SceneManager::Get().GetActiveScene();
	active_scene->OnUpdate().Update(deltaTimeSeconds);

	active_scene->FlushDeleteQueue();
}

void GameManager::FrameEnd() {}

void GameManager::DrawScene(glm::mat3 visMatrix) {}

void GameManager::OnInputUpdate(float deltaTime, int mods) {
	Scene *active_scene = SceneManager::Get().GetActiveScene();
	active_scene->OnInputUpdate().Update({mods, deltaTime, window, active_scene});
}

void GameManager::OnKeyPress(int key, int mods) {}

void GameManager::OnKeyRelease(int key, int mods) {}

void GameManager::OnMouseMove(int mouseX, int mouseY, int deltaX, int deltaY) {
	SceneManager::Get().GetActiveScene()->OnMouseMove().Update({mouseX, mouseY, deltaX, deltaY});
}

void GameManager::OnMouseBtnPress(int mouseX, int mouseY, int button, int mods) {
	MouseClickEvent event = {glm::vec2(mouseX, mouseY), button, mods};
	SceneManager::Get().GetActiveScene()->OnMouseDown().Update(event);
}

void GameManager::OnMouseBtnRelease(int mouseX, int mouseY, int button, int mods) {
	MouseClickEvent event = {glm::vec2(mouseX, mouseY), button, mods};
	SceneManager::Get().GetActiveScene()->OnMouseUp().Update(event);
}

void GameManager::OnMouseScroll(int mouseX, int mouseY, int offsetX, int offsetY) {}
