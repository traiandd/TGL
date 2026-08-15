#pragma once

#include <algorithm>
#include <functional>
#include <vector>

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
		value = std::move(new_value);
		Update();
	}
	void Update(std::function<T(T &&)> map) {
		value = std::move(map(std::move(value)));
		Update();
	}
	ObserverHandler<T> Subscribe(std::function<void(const T &)> new_sub) {
		size_t id = next_id++;
		subscribers.emplace_back(id, std::move(new_sub));
		return ObserverHandler{this, id};
	}
	void Unsubscribe(ObserverHandler<T> &&handler) {
		// TODO: Warning here; Observer handler of another observer passed.
		if (handler.observer != this)
			return;

		auto it = std::find_if(subscribers.begin(), subscribers.end(), [&](const auto &entry) { return entry.first == handler.id; });
		if (it == subscribers.end())
			return;

		*it = std::move(subscribers.back());
		subscribers.pop_back();
	}
	const T &Get() const { return value; }

  private:
	T value;
	size_t next_id = 0;
	std::vector<std::pair<size_t, std::function<void(const T &)>>> subscribers;
};