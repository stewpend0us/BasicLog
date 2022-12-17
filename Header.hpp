#pragma once
#include "chararr.hpp"

namespace BasicLog
{
	namespace detail
	{

		static constexpr char q = '"';
		static constexpr char c = ',';
		static constexpr char br = ']';
		static constexpr char bl[] = "[\n";
		static constexpr char name[] = "\"name\":";
		static constexpr char desc[] = "\"desc\":";
		static constexpr char size[] = "\"size\":";
		static constexpr char type[] = "\"type\":";

		constexpr auto Header_general(const auto &Name, const auto &Description, const auto &Before, const auto &Type, const auto &After, const auto &Size)
		{
			return concat("{", name, q, Name, q, c, desc, q, Description, q, c, size, Size, c, type, Before, Type, After, "}\n");
		}

	}

	constexpr auto Header(const auto &Name, const auto &Description, const auto &Type, const auto &Size)
	{
		return detail::Header_general(Name, Description, detail::q, Type, detail::q, Size);
	}

	constexpr auto Header_nested(const auto &Name, const auto &Description, const auto &Type, const auto &Size)
	{
		return detail::Header_general(Name, Description, detail::bl, Type, detail::br, Size);
	}
}