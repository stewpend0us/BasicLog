#pragma once
#include <cstdint>
#include <type_traits>

namespace detail
{
	static_assert(sizeof(long double) == 128 / 8 && "assumed long double size");
	static_assert(sizeof(double) == 64 / 8 && "assumed double size");
	static_assert(sizeof(float) == 32 / 8 && "assumed float size");
	static_assert(sizeof(bool) == 8 / 8 && "assumed bool size");
	static_assert(sizeof(char) == 8 / 8 && "assumed char size");

	// fallback to a compiler error if there is no specialization for T
	template <typename T>
	struct type_name
	{
		static_assert(std::is_same_v<void, T> && "type not supported");
	};

// list of c++ name, my name
#define X_LIST_BASICLOG_TYPES \
	X(long double, float128)  \
	X(double, float64)        \
	X(float, float32)         \
	X(bool, bool)             \
	X(char, char)             \
	X(int8_t, int8)           \
	X(int16_t, int16)         \
	X(int32_t, int32)         \
	X(int64_t, int64)         \
	X(uint8_t, uint8)         \
	X(uint16_t, uint16)       \
	X(uint32_t, uint32)       \
	X(uint64_t, uint64)

// for each type,name in the list define a specialization that looks like:
#define X(type, name)                          \
	template <>                                \
	struct type_name<type>                     \
	{                                          \
		static constexpr char value[] = #name; \
	};
	X_LIST_BASICLOG_TYPES
#undef X

#undef X_LIST_TYPES
}

template <typename T>
constexpr auto type_name_v = detail::type_name<T>::value;