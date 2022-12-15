#pragma once
#include <type_traits>
#include "ConstexprString.hpp"
#include "type_name.hpp"
#include "unsigned_to_string.hpp"

#define NL "\n"

namespace BasicLog
{
	namespace detail
	{
		constexpr std::string_view sl = "{";
		constexpr std::string_view sr = "}\n";
		constexpr std::string_view bl = "[\n";
		constexpr std::string_view br = "]";
		constexpr std::string_view c = ",";
		constexpr std::string_view q = "\"";
		constexpr std::string_view name = "\"name\":";
		constexpr std::string_view desc = "\"desc\":";
		constexpr std::string_view size = "\"size\":";
		constexpr std::string_view type = "\"type\":";

		template <std::string_view const &Name, std::string_view const &Description, std::string_view const &Before, std::string_view const &Type, std::string_view const &After, std::string_view const &Size>
		struct Header
		{
			constexpr static std::string_view value = join_v<sl, name, q, Name, q, c, desc, q, Description, q, c, size, Size, c, type, Before, Type, After, sr>;
		};

		template <std::string_view const &Before, std::string_view const &After>
		std::string Header_dynamic(std::string_view const Name, std::string_view const Description, std::string_view const Type, std::string_view const Size)
		{
			return std::string(join_v<sl, name, q>).append(Name).append(join_v<q, c, desc, q>).append(Description).append(join_v<q, c, size>).append(Size).append(join_v<c, type, Before>).append(Type).append(join_v<After, sr>);
		};
	}

	template <std::string_view const &Name, std::string_view const &Description, std::string_view const &Type, std::string_view const &Size>
	constexpr std::string_view Header_fundamental_v = detail::Header<Name, Description, detail::q, Type, detail::q, Size>::value;

	template <std::string_view const &Name, std::string_view const &Description, std::string_view const &Type, std::string_view const &Size>
	constexpr std::string_view Header_complex_v = detail::Header<Name, Description, detail::bl, Type, detail::br, Size>::value;

	std::string Header_fundamental(std::string_view const Name, std::string_view const Description, std::string_view const Type, std::string_view const Size)
	{
		return detail::Header_dynamic<detail::q, detail::q>(Name, Description, Type, Size);
	}

	std::string Header_complex(std::string_view const Name, std::string_view const Description, std::string_view const Type, std::string_view const Size)
	{
		return detail::Header_dynamic<detail::bl, detail::br>(Name, Description, Type, Size);
	}
}