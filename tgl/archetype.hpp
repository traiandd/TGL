#pragma once

#include "tgl/entity.hpp"

using namespace tgl;
namespace tgl {

#define __MethodInstance                                                                                                                                                 \
	EntityInstance Instance() { return static_cast<Derived *>(this)->instance_; }

#define __MethodSelf(type)                                                                                                                                               \
	type *Self() { return static_cast<Derived *>(this)->instance_.template Get<type>(); }

#define __MethodGetScene                                                                                                                                                 \
	Scene *GetScene() { return static_cast<Derived *>(this)->instance_.GetScene(); }

#define FORWARD_METHOD(method)                                                                                                                                           \
	template<typename... Args> auto method(Args &&...args) { return Self()->method(std::forward<Args>(args)...); }

/* Foreach macro hack */
#define FM_0(X)
#define FM_1(X, ...) FORWARD_METHOD(X) FM_0(__VA_ARGS__)
#define FM_2(X, ...) FORWARD_METHOD(X) FM_1(__VA_ARGS__)
#define FM_3(X, ...) FORWARD_METHOD(X) FM_2(__VA_ARGS__)
#define FM_4(X, ...) FORWARD_METHOD(X) FM_3(__VA_ARGS__)
#define FM_5(X, ...) FORWARD_METHOD(X) FM_4(__VA_ARGS__)
#define FM_6(X, ...) FORWARD_METHOD(X) FM_5(__VA_ARGS__)
#define FM_7(X, ...) FORWARD_METHOD(X) FM_6(__VA_ARGS__)
#define FM_8(X, ...) FORWARD_METHOD(X) FM_7(__VA_ARGS__)
#define FM_9(X, ...) FORWARD_METHOD(X) FM_8(__VA_ARGS__)
#define FM_10(X, ...) FORWARD_METHOD(X) FM_9(__VA_ARGS__)
#define FM_11(X, ...) FORWARD_METHOD(X) FM_10(__VA_ARGS__)
#define FM_12(X, ...) FORWARD_METHOD(X) FM_11(__VA_ARGS__)
#define FM_13(X, ...) FORWARD_METHOD(X) FM_12(__VA_ARGS__)
#define FM_14(X, ...) FORWARD_METHOD(X) FM_13(__VA_ARGS__)
#define FM_15(X, ...) FORWARD_METHOD(X) FM_14(__VA_ARGS__)
#define FM_16(X, ...) FORWARD_METHOD(X) FM_15(__VA_ARGS__)
#define FM_17(X, ...) FORWARD_METHOD(X) FM_16(__VA_ARGS__)
#define FM_18(X, ...) FORWARD_METHOD(X) FM_17(__VA_ARGS__)
#define FM_19(X, ...) FORWARD_METHOD(X) FM_18(__VA_ARGS__)
#define FM_20(X, ...) FORWARD_METHOD(X) FM_19(__VA_ARGS__)
#define GET_MACRO_20(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, NAME, ...) NAME
#define __MethodForward(...)                                                                                                                                             \
	GET_MACRO_20(_0, __VA_ARGS__, FM_19, FM_18, FM_17, FM_16, FM_15, FM_14, FM_13, FM_12, FM_11, FM_10, FM_9, FM_8, FM_7, FM_6, FM_5, FM_4, FM_3, FM_2, FM_1, FM_0)(     \
		__VA_ARGS__)
#define Using(X) __Method##X

class ArchetypeData {
  public:
	ArchetypeData(EntityInstance instance) : instance_(instance) {}
	EntityInstance instance_;
};

template<typename T, typename Derived> class ArchetypeExtender;

template<typename... Components> class Archetype : public ArchetypeData, public ArchetypeExtender<Components, Archetype<Components...>>... {
  public:
	explicit Archetype(const EntityInstance instance) : ArchetypeData(instance) {
		// compile-time list, runtime assertion
		(..., assert(instance_.Get<Components>() != nullptr));
	}

	static std::optional<Archetype<Components...>> TryFrom(const EntityInstance instance) {
		bool hasAll = (... && (instance.Get<Components>() != nullptr));
		if (!hasAll)
			return std::nullopt;
		return Archetype<Components...>(instance);
	}

	template<typename T> T *Get() const {
		static_assert((std::is_same_v<T, Components> || ...), "This entity is not guaranteed to have this component");
		return instance_.Get<T>();
	}
	operator EntityInstance() const { return instance_; }
};
} // namespace tgl
