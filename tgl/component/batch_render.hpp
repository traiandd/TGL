#pragma once

#include "register_component.hpp"
#include "core/gpu/mesh.h"
#include "component.hpp"
#include "tgl/archetype.hpp"
#include <string>

struct BatchRenderComponent : public Component {
	Mesh *mesh;
	std::string shader = "VertexColor";

	BatchRenderComponent() = default;
	BatchRenderComponent(Mesh *m) : mesh(m) {}
	BatchRenderComponent(Mesh *m, std::string s) : mesh(m), shader(s) {}

	Mesh *GetMesh() { return mesh; }

	std::string GetShader() { return shader; }
};

register_component(BatchRenderComponent);

template<typename Derived> class tgl::ArchetypeExtender<BatchRenderComponent, Derived> {
	Using(Instance);
	Using(Self(BatchRenderComponent));

  public:
	Using(Forward(GetMesh, GetShader));
};
