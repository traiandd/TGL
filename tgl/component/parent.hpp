#pragma once
#include "tgl/archetype.hpp"
#include "tgl/entity.hpp"

struct Parent {
	tgl::EntityInstance m_parent;
	Parent() {}

	Parent(tgl::EntityInstance parent) : m_parent(parent) {}

	operator tgl::EntityInstance() const { return m_parent; }
};

template<typename Derived> class tgl::ArchetypeExtender<Parent, Derived> {
	Using(Self(Parent));

  public:
	EntityInstance GetParent() { return Self()->m_parent; }
};