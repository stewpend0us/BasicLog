#include <string_view>
#include <string>
#include <vector>
#include <functional>

#include "Header.hpp"
#include "type_name.hpp"

#include <iostream>
#include <iomanip>

namespace BasicLog
{
	template <class T>
	concept is_Fundamental = std::is_fundamental_v<T>;

	template <class T>
	concept is_Class = std::is_class_v<T>;

	class Log
	{
	public:
		//		template <is_Class B>
		//		struct StructMember;

		struct LogEntry;
		template <is_Class B>
		using StructMember = std::function<LogEntry(B const *const Data)>;

		struct LogEntry
		{
			LogEntry(const std::string_view Name, const std::string_view Description, const std::string_view Type, char const *const Ptr, size_t Count, size_t Size, size_t Stride)
				: name(Name), description(Description), count(Count), ptr(Ptr), size(Size), stride(Stride), type(Type), header(Header(name, description, type, count))
			{
			}

			LogEntry(const std::string_view Name, const std::string_view Description, char const *const Ptr, size_t Count, std::convertible_to<LogEntry> auto const... child_entries)
				: name(Name), description(Description), count(Count), ptr(Ptr), children({child_entries...})
			{
				static_assert(sizeof...(child_entries) > 0 && "must have at least one child");
				size = children[0].count * children[0].size;
				type = children[0].header;
				for (size_t i = 1; i < children.size(); i++)
				{
					size += children[i].count * children[i].size;
					type += ",\n" + children[i].header;
				}
				header = Header_nested(name, description, type, count);
			}

			const std::string_view name;
			const std::string_view description;
			const size_t count;
			const char *const ptr;
			size_t size;
			size_t stride;
			std::string type;
			std::string header;
			std::vector<LogEntry> children;
		};

		struct LoggedItem
		{
			const std::string_view name;
			const char *const ptr;
			const size_t count;
			const size_t size;
			const size_t stride;
		};

		Log(const std::string_view Name, const std::string_view Description, std::convertible_to<const LogEntry> auto const... child_entries)
			: MainEntry(Entry(Name, Description, child_entries...))
		{
			std::function<void(LogEntry)> extract;
			extract = [&](LogEntry L)
			{
				if (L.children.empty())
				{
					Items.push_back({L.name, L.ptr, L.count, L.size, L.stride});
					return;
				}
				for (auto c : L.children)
				{
					extract(c);
				}
			};
			extract(MainEntry);

			const auto w = std::setw(7);
			std::cout << std::setw(15) << "name" << std::setw(20) << "address" << w << "count" << w << "size" << w << "stride" << '\n';
			for (auto i : Items)
			{
				std::cout << std::setw(15) << i.name << std::setw(20) << (void*)i.ptr << w << i.count << w << i.size << w << i.stride << '\n';
			}
		}

		// Append(LogEntry)?

		// a zero size container. cannot represent an array, just a container of things
		static const LogEntry Entry(const std::string_view Name, const std::string_view Description, std::convertible_to<const LogEntry> auto const... child_entries)
		{
			return LogEntry(Name, Description, (char *)nullptr, 1, child_entries...);
		}

		// a fundamental
		static const LogEntry Entry(const std::string_view Name, const std::string_view Description, is_Fundamental auto const *const data, size_t Count = 1)
		{
			using Type = std::remove_cvref_t<decltype(*data)>;
			return LogEntry(Name, Description, type_name_v<Type>, (char *)data, Count, sizeof(Type), alignof(Type));
		}

		// a fundamental array
		template <size_t Count>
		static const LogEntry Entry(const std::string_view Name, const std::string_view Description, is_Fundamental auto const (&arr)[Count])
		{
			auto ptr = &arr[0];
			using Type = std::remove_cvref_t<decltype(*ptr)>;
			return LogEntry(Name, Description, type_name_v<Type>, (char *)ptr, Count, sizeof(Type), alignof(Type));
		}

		// a struct (user supplied spec)
		template <is_Class B>
		static const LogEntry Entry(const std::string_view Name, const std::string_view Description, B const *const data, size_t Count, std::convertible_to<const StructMember<B>> auto const... child_entries)
		{
			return LogEntry(Name, Description, (char *)data, Count, std::invoke(child_entries, data)...);
		}

		template <is_Fundamental A, is_Class B>
		static const StructMember<B> Entry(const std::string_view Name, const std::string_view Description, A B::*Member, size_t Count = 1)
		{
			return [=](B const *const Data)
			{
				return LogEntry(Name, Description, type_name_v<A>, (char *)&(Data->*Member), Count, sizeof(A), sizeof(B));
			};
		}

		template <is_Fundamental A, is_Class B, size_t Count>
		static const StructMember<B> Entry(const std::string_view Name, const std::string_view Description, A (B::*Member)[Count])
		{
			return [=](B const *const Data)
			{
				return LogEntry(Name, Description, type_name_v<A>, (char *)&(Data->*Member), Count, sizeof(A), sizeof(B));
			};
		}

		//	private:
		LogEntry MainEntry;
		std::vector<LoggedItem> Items;

		//		static auto error(const std::string_view name, const std::string_view description, const std::string_view msg)
		//		{
		//			return std::runtime_error(std::string("(name:").append(name).append(", description:").append(description).append("): ").append(msg));
		//		}
	};
}