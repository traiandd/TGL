#pragma once

#include "register_component.hpp"
#include "core/gpu/mesh.h"
#include "component.hpp"
#include "tgl/archetype.hpp"
#include <string>

struct RenderComponent : public Component {
	Mesh *mesh;
	std::string shader = "VertexColor";

	RenderComponent() = default;
	RenderComponent(Mesh *m) : mesh(m) {}
	RenderComponent(Mesh *m, std::string s) : mesh(m), shader(s) {}

	Mesh *GetMesh() { return mesh; }

	void SetMesh(Mesh *m) { mesh = m; }

	std::string GetShader() { return shader; }
};

register_component(RenderComponent);

template<typename Derived> class tgl::ArchetypeExtender<RenderComponent, Derived> {
	Using(Instance);
	Using(Self(RenderComponent));

  public:
	Using(Forward(GetMesh, GetShader, SetMesh));
};
