#pragma once
#include "ConstexprString.hpp"
#include "type_name.hpp"
#include "unsigned_to_string.hpp"

#define NL "\n"

namespace BasicLog
{

	constexpr std::string_view name = "name";
	constexpr std::string_view desc = "desc";
	constexpr std::string_view size = "size";
	constexpr std::string_view type = "type";

	template <std::string_view const &Name, std::string_view const &Description, typename Type, size_t Size>
	constexpr std::string_view Header()
	{
		return join_v<name, Name, desc, Description, size, unsigned_to_string_v<Size>, type, type_name_v<Type>>;
	}

	//	class Header
	//	{
	//
	//		static std::string json_entry(const std::string_view name, const std::string_view description, size_t count, const std::string_view before_type, const std::string_view type, const std::string_view after_type)
	//		{
	//			std::string result("{");
	//			result += "\"name\":\"" + std::string(name) + "\",";
	//			result += "\"desc\":\"" + std::string(description) + "\",";
	//			result += "\"size\":" + std::to_string(count) + ",";
	//			result += "\"type\":" + std::string(before_type) + std::string(type) + std::string(after_type) + "}";
	//			return result;
	//		}
	//
	//		static std::string json_string_entry(const std::string_view name, const std::string_view description, size_t count, const std::string_view type)
	//		{
	//			return json_entry(name, description, count, "\"", type, "\"");
	//		}
	//
	//		static std::string json_array_entry(const std::string_view name, const std::string_view description, size_t count, const std::string_view type)
	//		{
	//			return json_entry(name, description, count, "[" NL, type, "]");
	//		}
	//	};
}