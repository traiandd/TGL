#pragma once

#include "core/gpu/mesh.h"
#include "tgl/archetype.hpp"
#include <string>

struct RenderComponent {
	Mesh *mesh;
	std::string shader = "VertexColor";

	RenderComponent() = default;
	RenderComponent(Mesh *m) : mesh(m) {}
	RenderComponent(Mesh *m, std::string s) : mesh(m), shader(s) {}

	Mesh *GetMesh() { return mesh; }

	void SetMesh(Mesh *m) { mesh = m; }

	std::string GetShader() { return shader; }
};

template<typename Derived> class tgl::ArchetypeExtender<RenderComponent, Derived> {
	Using(Instance);
	Using(Self(RenderComponent));

  public:
	Using(Forward(GetMesh, GetShader, SetMesh));
};
