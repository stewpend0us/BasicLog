#pragma once
#include <string_view>
#include <cstdint>
#include <type_traits>

// falsy placeholder so we can static_assert in the fallback for type_name
template <typename T>
struct assert_false : std::false_type
{
	// might as well use this scope for some static asserts while we're at it
	static_assert(sizeof(long double) == 128 / 8, "assumed long double size");
	static_assert(sizeof(double) == 64 / 8, "assumed double size");
	static_assert(sizeof(float) == 32 / 8, "assumed float size");
	static_assert(sizeof(bool) == 8 / 8, "assumed bool size");
	static_assert(sizeof(char) == 8 / 8, "assumed char size");
};

// fallback to a compiler error if there is no specialization for T
template <typename T>
struct type_name
{
	static_assert(assert_false<T>::value, "type not supported");
};

// list of c++ name, my name
#define X_LIST_TYPES         \
	X(long double, float128) \
	X(double, float64)       \
	X(float, float32)        \
	X(bool, bool)            \
	X(char, char)            \
	X(int8_t, int8)          \
	X(int16_t, int16)        \
	X(int32_t, int32)        \
	X(int64_t, int64)        \
	X(uint8_t, uint8)        \
	X(uint16_t, uint16)      \
	X(uint32_t, uint32)      \
	X(uint64_t, uint64)

// for each type,name in the list define a specialization that looks like:
#define X(type, name)                                    \
	template <>                                          \
	struct type_name<type>                               \
	{                                                    \
		static constexpr std::string_view value = #name; \
	};
X_LIST_TYPES
#undef X

#undef X_LIST_TYPES

// a helper so we don't have to "call" ::value all the time
template<typename T>
constexpr std::string_view type_name_v = type_name<T>::value;