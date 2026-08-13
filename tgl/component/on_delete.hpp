#pragma once
#include "register_component.hpp"
#include "tgl/observer.hpp"

struct DeleteEvent {};

struct DeleteListener : Component {
	Observer<DeleteEvent> onDelete;

	DeleteListener() = default;

	Observer<DeleteEvent> &OnDelete() { return onDelete; }
};

register_component(DeleteListener);