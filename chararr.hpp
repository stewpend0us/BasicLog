#include <array>
#include <numeric>
#include <tuple>

using C = char;

template <size_t N>
using chararr = std::array<C,N>;

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

// TODO can we simplify the following because we know T will always be char (or C)?
	template <typename T>
	struct type_t
	{
		using type = T;
	};
	template <typename T>
	inline constexpr type_t<T> type{};

	template <typename T, size_t N>
	constexpr size_t type_size(type_t<T[N]>)
	{
		return N - 1; // exclude the zero
	};

	template <typename T, size_t N>
	constexpr size_t type_size(type_t<std::array<T, N>>)
	{
		return N; // there is no zero
	}

	template <typename T>
	constexpr size_t type_size(T)
	{
		return 1; // char
	}
}

// TODO require args be a c array of char or a std::array of char
// no pointers no string_views no strings
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