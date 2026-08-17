#pragma once

#include "core/gpu/mesh.h"
#include "tgl/archetype.hpp"

struct UIRenderComponent {
	Mesh *mesh;
	std::string shader = "VertexColor";

	UIRenderComponent() = default;
	UIRenderComponent(Mesh *&m) : mesh(m) {}
	UIRenderComponent(Mesh *m, std::string s) : mesh(m), shader(s) {}

	Mesh *GetMesh() { return mesh; }

	std::string GetShader() { return shader; }
};

template<typename Derived> class tgl::ArchetypeExtender<UIRenderComponent, Derived> {
	Using(Instance);
	Using(Self(UIRenderComponent));

  public:
	Using(Forward(GetMesh, GetShader));
};