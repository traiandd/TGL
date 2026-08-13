#pragma once

namespace tgl {
class Scene;

class System {
  public:
	System(Scene *scene) : m_scene(scene) {}
	virtual ~System() = default;
	void SetScene(Scene *scene) { m_scene = scene; }

  protected:
	Scene *m_scene;
};
} // namespace tgl