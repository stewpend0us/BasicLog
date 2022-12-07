#include <string>
#include <string_view>
#include <vector>
#include <concepts>
#include <stdexcept>

#include <iostream>
#include "type_name.hpp"

#define NL "\n"

// TODO
/*
class Log
{

	struct Entry
	{
		//std::string_view name;
		//std::string_view description;
		std::string header;
		char * ptr;
		size_t size;
	}

	struct StructEntryChild
	{
		//std::string_view name;
		//std::string_view description;
		std::string header;
		size_t total_size;
	}

	Log(...Entry)
	Append(Entry)

	static constexpr Entry BasicEntry(...)
	static constexpr Entry ContainerEntry(...Entry)
	static constexpr Entry StructEntry(...StructEntryChild)

	template <typename T, size_t N = 1>
	requires(N>1)
	static constexpr StructEntryChild ChildEntry(...)

}
*/
namespace BasicLog
{
	// is this thing a real piece of basic data?
	template <class T>
	concept is_Fundamental = std::is_fundamental_v<T>;

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

	class Child
	{
	public:
		std::string_view name;
		std::string_view description;
		size_t size;
		std::string header;

		template <is_Fundamental T, size_t N = 1>
		requires(N > 0) static constexpr Child Entry(const std::string_view name, const std::string_view description)
		{
			return {name, description, N * sizeof(T), json_string_entry(name, description, N, type_name_v<T>)};
		}
	};

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
			header = json_array_entry(name, description, 1, type);
		}

		// Structure
		Entry(const std::string_view name, const std::string_view description, auto const *entry, std::convertible_to<const Child> auto const... child_entries)
			: Entry(name, description, entry, 1, child_entries...)
		{
		}

		// Structure Array
		Entry(const std::string_view name, const std::string_view description, auto const *entry, size_t count, std::convertible_to<const Child> auto const... child_entries)
			: Entry(name, description)
		{
			size_t total_size = 0;
			std::string type("");
			bool first = true;
			for (const auto &child : {child_entries...})
			{
				total_size += child.size;
				if (first)
				{
					type = child.header;
					first = false;
					continue;
				}
				type += "," NL + child.header;
			}
			header = json_array_entry(name, description, count, type);

			if (sizeof(*entry) != total_size)
				throw error("size mismatch. data size is " + std::to_string(sizeof(*entry)) + " total children size is " + std::to_string(total_size) +
							"\n           likely due to struct padding or the struct definition is out of sync with the log definition");

			data.ptr = (char *)entry;
			data.size = sizeof(*entry) * count;
		}
	};

}