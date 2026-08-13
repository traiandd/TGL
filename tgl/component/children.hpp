#pragma once
#include "register_component.hpp"
#include "../entity.hpp"
#include "tgl/archetype.hpp"
#include "tgl/component/on_delete.hpp"

#include <set>

struct Children : Component {
	std::set<tgl::EntityInstance> m_child_entities;

	Children() = default;

	std::set<tgl::EntityInstance> &GetChildren() { return m_child_entities; }
};

register_component(Children);

template<typename Derived> class tgl::ArchetypeExtender<Children, Derived> {
	Using(Instance);
	Using(Self(Children));

  public:
	Using(Forward(GetChildren));

	void AddChild(EntityInstance c) {
		DeleteListener *del = c.Get<DeleteListener>();
		if (!del) {
			c.Add(DeleteListener());
			del = c.Get<DeleteListener>();
		}
		Children *self = Self();
		self->m_child_entities.insert(c);
		// TODO: this breaks if parent dies before child. Fix plz
		del->OnDelete().Subscribe([c, self](auto &e) { self->m_child_entities.erase(c); });
		c.Add(Parent(Instance()));
	}
};