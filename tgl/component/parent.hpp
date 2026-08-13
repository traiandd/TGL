#pragma once
#include "register_component.hpp"
#include "tgl/archetype.hpp"
#include "tgl/entity.hpp"

struct Parent : Component {
	tgl::EntityInstance m_parent;
	Parent() {}

	Parent(tgl::EntityInstance parent) : m_parent(parent) {}

	operator tgl::EntityInstance() const { return m_parent; }
};

register_component(Parent);

template<typename Derived> class tgl::ArchetypeExtender<Parent, Derived> {
	Using(Self(Parent));

  public:
	EntityInstance GetParent() { return Self()->m_parent; }
};