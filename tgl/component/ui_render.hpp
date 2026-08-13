#pragma once

#include "core/gpu/mesh.h"
#include "component.hpp"
#include "register_component.hpp"
#include "tgl/archetype.hpp"

struct UIRenderComponent : public Component {
	Mesh *mesh;
	std::string shader = "VertexColor";

	UIRenderComponent() = default;
	UIRenderComponent(Mesh *&m) : mesh(m) {}
	UIRenderComponent(Mesh *m, std::string s) : mesh(m), shader(s) {}

	Mesh *GetMesh() { return mesh; }

	std::string GetShader() { return shader; }
};

register_component(UIRenderComponent);

template<typename Derived> class tgl::ArchetypeExtender<UIRenderComponent, Derived> {
	Using(Instance);
	Using(Self(UIRenderComponent));

  public:
	Using(Forward(GetMesh, GetShader));
};