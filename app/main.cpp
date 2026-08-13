#include <ctime>
#include <string>

#include "app/empty_scene.hpp"
#include "core/engine.h"
#include "tgl/game_manager.hpp"
#include "utils/gl_utils.h"

#ifdef _WIN32
PREFER_DISCRETE_GPU_NVIDIA;
PREFER_DISCRETE_GPU_AMD;
#endif

std::string GetParentDir(const std::string &filePath) {
	size_t pos = filePath.find_last_of("\\/");
	return (std::string::npos == pos) ? "." : filePath.substr(0, pos);
}

int main(int argc, char **argv) {
	srand((unsigned int)time(NULL));

	WindowProperties wp;
	wp.resolution = glm::ivec2(1280, 720);
	wp.vSync = true;
	wp.selfDir = GetParentDir(std::string(argv[0]));

	(void)Engine::Init(wp);
	Engine::GetWindow()->DisablePointer();

	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);

	glEnable(GL_FRAMEBUFFER_SRGB);

	tgl::GameManager &gm = tgl::GameManager::Get();
	gm.Init();

	SceneId scene = EmptyScene();
	gm.SetActiveScene(scene);
	gm.Run();

	Engine::Exit();

	return 0;
}
