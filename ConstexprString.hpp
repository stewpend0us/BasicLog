#pragma once
//#include <algorithm>
#include <array>

namespace detail
{
	// https://ctrpeach.io/posts/cpp20-string-literal-template-parameters/

	template <std::string_view const &...Strs>
	struct join
	{
		// Join all strings into a single std::array of chars
		static constexpr auto impl() noexcept
		{
			constexpr std::size_t len = (Strs.size() + ... + 0);
			std::array<char, len + 1> arr{};
			auto append = [i = 0, &arr](auto const &s) mutable
			{
				for (auto c : s)
					arr[i++] = c;
			};
			(append(Strs), ...);
			arr[len] = 0;
			return arr;
		}
		// Give the joined string static storage
		static constexpr auto arr = impl();
		// View as a std::string_view
		static constexpr std::string_view value{arr.data(), arr.size() - 1};
	};
}

template <std::string_view const &...Strs>
static constexpr auto join_v = detail::join<Strs...>::value;

template <size_t N>
struct ConstexprString
{
	constexpr ConstexprString(const char (&str)[N])
	{
		std::copy_n(str, N, value);
	}
	char value[N];
};


//template<std::string_view const & Str>
//struct constexpr_string
//{
//	static constexpr auto impl() noexcept
//	{
//
//	}
//	static constexpr auto arr = impl();
//	static constexpr std::string_view value{arr.data(),arr.size()-1};
//}