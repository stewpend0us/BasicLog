#pragma once
#include <string_view>

namespace BasicLog
{
	namespace detail
	{

		static constexpr std::string_view q = "\"";
		static constexpr std::string_view c = ",";
		static constexpr std::string_view qc = "\",";
		static constexpr std::string_view br = "]";
		static constexpr std::string_view bl = "[\n";
		static constexpr std::string_view name = "\"name\":";
		static constexpr std::string_view desc = "\"desc\":";
		static constexpr std::string_view size = "\"size\":";
		static constexpr std::string_view type = "\"type\":";

		auto Header_general(const std::string_view Name, const std::string_view Description, const std::string_view Before, const std::string_view Type, const std::string_view After, const std::string_view Size)
		{
			return std::string("{").append(name).append(q).append(Name).append(qc).append(desc).append(q).append(Description).append(qc).append(size).append(Size).append(c).append(type).append(Before).append(Type).append(After).append("}\n");
		}
	}

	auto Header(const std::string_view Name, const std::string_view Description, const std::string_view Type, const auto Size)
	{
		return detail::Header_general(Name, Description, detail::q, Type, detail::q, Size);
	}

	auto Header_nested(const std::string_view Name, const std::string_view Description, const std::string_view Type, const auto Size)
	{
		return detail::Header_general(Name, Description, detail::bl, Type, detail::br, Size);
	}
}