#include <string>
#include <string_view>
#include <vector>
#include <concepts>
#include <stdexcept>

#include <iostream>
#include "type_name.hpp"

#define NL "\n"

namespace BasicLog
{
	// is this thing a real piece of basic data?
	template <class T>
	concept is_Fundamental = std::is_fundamental_v<T>;


	struct type_information
	{
		const std::string_view name;
		const size_t size;
		const size_t count;
		size_t total_size(void) const { return size * count; }
	};

	template <is_Fundamental T, size_t Count = 1>
	requires(Count > 0) constexpr static type_information Represents(void)
	{
		return {type_name_v<T>, sizeof(T), Count};
	}

	static std::string json_entry(const std::string_view name, const std::string_view description, size_t count, const std::string_view before_type, const std::string_view type, const std::string_view after_type)
	{
		std::string result("{");
		result += "\"name\":\"" + std::string(name) + "\",";
		result += "\"desc\":\"" + std::string(description) + "\",";
		result += "\"size\":" + std::to_string(count) + ",";
		result += "\"type\":" + std::string(before_type) + std::string(type) + std::string(after_type) + "}";
		return result;
	}

	static std::string json_string_entry(const std::string_view name, const std::string_view description, size_t count, const std::string_view type)
	{
		return json_entry(name, description, count, "\"", type, "\"");
	}

	static std::string json_array_entry(const std::string_view name, const std::string_view description, size_t count, const std::string_view type)
	{
		return json_entry(name, description, count, "[" NL, type, "]");
	}

	struct DataEntry
	{
		char *ptr = nullptr;
		size_t size = 0;
	};

	// Represents an entry in the log
	// lots of different constructures for a very simple type
	class Entry
	{
	private:
		const std::string_view name;
		const std::string_view description;

		std::string header;
		DataEntry data;

		std::runtime_error error(const std::string &info) const
		{
			return std::runtime_error("name:" + std::string(name) + ", description:" + std::string(description) + "..." + info);
		}

		Entry(const std::string_view name, const std::string_view description)
			: name(name), description(description)
		{
		}

		// General Container
		Entry(const std::string_view name, const std::string_view description, size_t count, std::convertible_to<const Entry> auto const... child_entries)
			: Entry(name, description)
		{
			std::string type("");
			bool first = true;
			for (const auto &child : {child_entries...})
			{
				if (first)
				{
					type = child.header;
					first = false;
					continue;
				}
				type += "," NL + child.header;
			}
			header = json_array_entry(name, description, count, type);
		}

	public:
		std::string get_header(void) const
		{
			return header;
		}

		DataEntry get_data(void) const
		{
			return data;
		}

		// Fundamental (and Fundamental array)
		Entry(const std::string_view name, const std::string_view description, is_Fundamental auto const *entry, size_t count = 1)
			: Entry(name, description)
		{
			// remove the * and const from the entry type
			header = json_string_entry(name, description, count, type_name_v<std::remove_const_t<std::remove_pointer_t<decltype(entry)>>>);
			data.ptr = (char *)entry;
			data.size = sizeof(*entry) * count;
		}

		// Container
		Entry(const std::string_view name, const std::string_view description, std::convertible_to<const Entry> auto const... child_entries)
			: Entry(name, description, 1, child_entries...)
		{
		}

		// Structure
		Entry(const std::string_view name, const std::string_view description, auto const *entry, std::convertible_to<const Entry> auto const... child_entries)
			: Entry(name, description, entry, 1, child_entries...)
		{
		}

		// Structure Array
		Entry(const std::string_view name, const std::string_view description, auto const *entry, size_t count, std::convertible_to<const Entry> auto const... child_entries)
			: Entry(name, description, count, child_entries...)
		{
			size_t total_size = 0;
			for (const auto &child : {child_entries...})
			{
				if (child.data.ptr != nullptr)
					throw child.error("contains data but shouldn't");
				if (child.data.size == 0)
					throw child.error("represents zero size");
				total_size += child.data.size;
			}
			if (sizeof(*entry) != total_size)
				throw error("size mismatch. data size is " + std::to_string(sizeof(*entry)) + " total children size is " + std::to_string(total_size) +
							"\n           likely due to struct padding or the struct definition is out of sync with the log definition");

			data.ptr = (char *)entry;
			data.size = sizeof(*entry) * count;
		}

		// Structure field (and Structure field array)
		Entry(const std::string_view name, const std::string_view description, const type_information info)
			: Entry(name, description)
		{
			header = json_string_entry(name, description, info.count, info.name);
			data.size = info.total_size();
		}
	};
}