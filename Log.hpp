#include <vector>
#include <string>
#include <string_view>
#include <type_traits>
#include <concepts>
#include <typeinfo>
#include <memory>
#include <variant>

#include <iostream>

namespace BasicLog
{

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

	// overload pattern
	template <class... Ts>
	struct overload : Ts...
	{
		using Ts::operator()...;
	};
	template <class... Ts>
	overload(Ts...) -> overload<Ts...>;

	// class prorotypes
	class DataEntry;
	class ContainerEntry;
	class DataStructure;

	// variant type for all Entry types
	using Entries = std::variant<DataEntry, ContainerEntry, DataStructure>;

	// is this thing a real piece of basic data?
	template <class T>
	concept is_Fundamental = std::is_fundamental<T>::value;

	class DataStructure
	{
		std::string name;
		std::string description;
		std::string type;

	public:
		size_t ptr_size;
		char *ptr;
		DataStructure(const std::string_view name, const std::string_view description, auto *item, std::is_convertible<std::type_info> auto... TI)
			: name(name), description(description), ptr_size(sizeof(*item)), ptr((char *)item)
		{
			type(get_type_name(typeid(*item)));
		}
	};

	// represents a single p
	class DataEntry
	{
		static std::string_view get_type_name(const std::type_info &id)
		{
			if (id == typeid(double))
				return "double";
			if (id == typeid(int))
				return "int";
			if (id == typeid(bool))
				return "bool";
			if (id == typeid(int8_t))
				return "int8";
			throw std::runtime_error(std::string("unexpected type id: ") + id.name());
		}

		std::string name;
		std::string description;
		std::string type;

	public:
		size_t ptr_size;
		char *ptr;

		DataEntry(const std::string_view name, const std::string_view description, is_Fundamental auto *item)
			: name(name), description(description), type(get_type_name(typeid(*item))), ptr_size(sizeof(*item)), ptr((char *)item)
		{
		}
		std::string get_header_entry(void)
		{
			return json_string_entry(name, description, 1, type);
		}
	};

	class ContainerEntry
	{
		std::string name;
		std::string description;

	public:
		std::vector<Entries> children;

		ContainerEntry(const std::string_view name, const std::string_view description, std::convertible_to<Entries> auto... E)
			: name(name), description(description), children({E...})
		{
		}

		std::string get_header_entry(void)
		{
			auto ov = overload{
				[](DataEntry &E)
				{
					return E.get_header_entry();
				},
				[](ContainerEntry &E)
				{
					return E.get_header_entry();
				},
			};
			std::string type(std::visit(ov, children.front()));
			for (size_t i = 1, s = children.size(); i < s; i++)
			{
				type += "," + std::visit(ov, children[i]);
			}
			return json_array_entry(name, description, 1, type);
		}
	};

	class Log
	{
	private:
		struct data_entry
		{
			char *ptr;
			size_t size;
		};

		std::string header;
		std::vector<data_entry> entries;

	public:
		Log(const std::string_view name, const std::string_view description, std::convertible_to<Entries> auto... E)
		{
			ContainerEntry C(name, description, E...);
			header = C.get_header_entry();
			auto ov = overload{
				[this](DataEntry &E)
				{
					entries.push_back({E.ptr, E.ptr_size});
				},
				[](ContainerEntry &E) {
				},
			};
			for (auto &E : C.children)
			{
				std::visit(ov, E);
			}
			std::cout << header;
		}
	};

}