#pragma once

#include <functional>
#include <map>
#include <iostream>

template<typename T> class Observer;

template<typename T> struct ObserverHandler {
	Observer<T> *observer;
	size_t id;
};

template<typename T> class Observer {
  public:
	Observer() = default;
	Observer(T default_value) : value(default_value) {}
	void Update() {
		for (auto &[id, subscriber] : subscribers) {
			subscriber(value);
		}
	}
	void Update(T new_value) {
		value = new_value;
		Update();
	}
	void Update(std::function<T(T &&)> map) {
		value = map(std::move(value));
		Update();
	}
	ObserverHandler<T> Subscribe(std::function<void(const T &)> new_sub) {
		size_t id = next_id++;
		subscribers[id] = new_sub;
		return ObserverHandler{this, id};
	}
	void Unsubscribe(ObserverHandler<T> &&handler) {
		// TODO: Warning here; Observer handler of another observer passed.
		if (handler.observer != this)
			return;

		subscribers.erase(handler.id);
	}
	const T &Get() const { return value; }

  private:
	T value;
	size_t next_id = 0;
	std::map<size_t, std::function<void(const T &)>> subscribers;
};