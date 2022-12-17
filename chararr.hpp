#pragma once
#include <array>
#include <numeric>
#include <tuple>

using C = char;

template <size_t N>
using chararr = std::array<C, N>;

namespace detail
{
	static constexpr void move(C *dest, const C *source, size_t size)
	{
		std::char_traits<C>::move(dest, source, size);
	}

	static constexpr void move(C *dest, const C source, size_t size)
	{
		std::char_traits<C>::move(dest, &source, size);
	}

	template <size_t N>
	static constexpr void move(C *dest, const chararr<N> &source, size_t size)
	{
		std::char_traits<C>::move(dest, source.data(), size);
	}

	// note these could be more general but we don't need it
	template <typename T>
	struct type_t
	{
		using type = T;
	};

	template <typename T>
	inline constexpr type_t<T> type{};

	constexpr size_t type_size(type_t<C>)
	{
		return 1; // char
	}

	template <size_t N>
	constexpr size_t type_size(type_t<C[N]>)
	{
		return (N - 1); // exclude the zero
	}

	template <size_t N>
	constexpr size_t type_size(type_t<std::array<C, N>>)
	{
		return N; // there is no zero
	}

}

// TODO require args be a c array of char, std::array of char, or char
// no pointers no string_views no strings (at the moment)
static constexpr auto concat(auto &...args)
{
	constexpr size_t nargin = sizeof...(args);
	constexpr std::array<size_t, nargin> lens{detail::type_size(detail::type<std::remove_cvref_t<decltype(args)>>)...};
	constexpr size_t total_size = std::accumulate(lens.begin(), lens.end(), 0);

	chararr<total_size> buff;
	size_t pos = 0;
	size_t i = 0;
	((detail::move(&buff[pos], args, lens[i]), pos += lens[i], i++), ...);
	return buff;
}

//#include "unsigned_to_string.hpp"
//auto get_digit(auto left, auto right)
//{
//	if (left > 0)
//	{
//		const auto ar = get_digit(left / 10, left % 10);
//
//		std::array<decltype(left), ar.size() + 1> buff;
//		std::move(std::begin(ar), std::end(ar), std::begin(buff));
//		buff.back() = left;
//		return buff;
//	}
//	return std::array<decltype(left), 1>{right};
//}
//auto test_to_string(auto num)
//{
//	size_t i = 0;
//	while (num > 0)
//	{
//		decltype(num) r = num % 10;
//		num /= 10;
//		i++;
//	}
//
//	std::array<decltype(num),i> buff;
//	//const auto tc = get_digit(num / 10, num % 10);
//	// return tc;
//	return 1337;
//}