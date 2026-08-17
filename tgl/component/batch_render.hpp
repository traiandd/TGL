#pragma once

#include "core/gpu/mesh.h"
#include "tgl/archetype.hpp"
#include <string>

struct BatchRenderComponent {
	Mesh *mesh;
	std::string shader = "VertexColor";

	BatchRenderComponent() = default;
	BatchRenderComponent(Mesh *m) : mesh(m) {}
	BatchRenderComponent(Mesh *m, std::string s) : mesh(m), shader(s) {}

	Mesh *GetMesh() { return mesh; }

	std::string GetShader() { return shader; }
};

template<typename Derived> class tgl::ArchetypeExtender<BatchRenderComponent, Derived> {
	Using(Instance);
	Using(Self(BatchRenderComponent));

  public:
	Using(Forward(GetMesh, GetShader));
};
