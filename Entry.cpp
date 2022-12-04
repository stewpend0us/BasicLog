#include <string>
#include <string_view>
#include <vector>
#include <concepts>
#include <stdexcept>
#include <typeinfo>

namespace BasicLog
{
	// is this thing a real piece of basic data?
	template <class T>
	concept is_Fundamental = std::is_fundamental<T>::value;

	static std::string json_entry(const std::string_view name, const std::string_view description, size_t count, const std::string &type)
	{
		std::string result("{");
		result += "\"name\":\"" + std::string(name) + "\",";
		result += "\"desc\":\"" + std::string(description) + "\",";
		result += "\"size\":" + std::to_string(count) + ",";
		result += "\"type\":" + type + "}";
		return result;
	}

	static std::string json_string_entry(const std::string_view name, const std::string_view description, size_t count, const std::string &type)
	{
		return json_entry(name, description, count, "\"" + type + "\"");
	}

	static std::string json_array_entry(const std::string_view name, const std::string_view description, size_t count, const std::string &type)
	{
		return json_entry(name, description, count, "[" + type + "]");
	}

	class Entry
	{
	private:
		std::string header_entry;

	public:
		std::string get_header(void) const
		{
			return header_entry;
		}

		// Fundamental
		Entry(const std::string_view name, const std::string_view description, is_Fundamental auto const *entry)
		{
			std::string type;
			const std::type_info &id(typeid(*entry));
			if (id == typeid(double))
				type = "double";
			else if (id == typeid(int))
				type = "int";
			else if (id == typeid(bool))
				type = "bool";
			else if (id == typeid(int8_t))
				type = "int8";
			else
				throw std::runtime_error(std::string(name) + " (" + std::string(description) + "): " + "unexpected type id (" + id.name() + ")");

			header_entry = json_string_entry(name, description, 1, type);
		}

		// Container
		Entry(const std::string_view name, const std::string_view description, std::convertible_to<const Entry> auto const... child_entries)
		{
			std::string type("");
			bool first = true;
			for (const auto &child : {child_entries...})
			{
				if (first)
				{
					type = child.get_header();
					first = false;
					continue;
				}
				type += "," + child.get_header();
			}
			header_entry = json_array_entry(name, description, 1, type);
		}

	};
}