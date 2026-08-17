#pragma once
#include "tgl/observer.hpp"

struct DeleteEvent {};

struct DeleteListener {
	Observer<DeleteEvent> onDelete;

	DeleteListener() = default;

	Observer<DeleteEvent> &OnDelete() { return onDelete; }
};
