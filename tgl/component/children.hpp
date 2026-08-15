#pragma once
#include "register_component.hpp"
#include "../entity.hpp"
#include "tgl/archetype.hpp"
#include "tgl/component/on_delete.hpp"
#include "tgl/component/parent.hpp"

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
		Self()->m_child_entities.insert(c);
		// TODO: this breaks if parent dies before child. Fix plz
		EntityInstance parent = Instance();
		del->OnDelete().Subscribe([c, parent](auto &e) {
			if (Children *children = parent.Get<Children>())
				children->m_child_entities.erase(c);
		});
		c.Add(Parent(Instance()));
	}
};