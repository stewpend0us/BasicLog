#include <string_view>
#include <string>
#include <vector>
#include <functional>

#include "Header.hpp"
#include "type_name.hpp"

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
				: name(Name), description(Description), type(Type), count(Count), size(Size), stride(Stride), ptr(Ptr), header(Header(name, description, type, count * size))
			{
			}

			LogEntry(const std::string_view Name, const std::string_view Description, const std::string_view Type, char const *const Ptr, size_t Count, size_t Size, size_t Stride, std::convertible_to<LogEntry> auto const... child_entries)
				: name(Name), description(Description), type(Type), count(Count), size(Size), stride(Stride), ptr(Ptr), header(Header(name, description, type, count * size)), children({child_entries...})
			{
			}

			//			std::string header() const
			//			{
			//				// TODO determine "type"
			//				//{
			//				//	LogEntry child[] = {child_entries...};
			//				//	size_t size = child[0].count * child[0].size;
			//				//	std::string type(child[0].header());
			//				//	for (size_t i = 1; i < sizeof...(child_entries); i++)
			//				//	{
			//				//		size += child[i].count * child[i].size;
			//				//		type += ",\n" + child[i].header();
			//				//	}
			//				return Header(name, description, type, count * size);
			//			}

			const std::string_view name;
			const std::string_view description;
			const std::string_view type;
			const size_t count;
			const size_t size;
			const size_t stride;
			const char *const ptr;
			const std::string header;
			std::vector<LogEntry> children;
		};

		Log(const std::string_view Name, const std::string_view Description, std::convertible_to<const LogEntry> auto const... child_entries)
			: MainEntry(Entry(Name, Description, child_entries...))
		{
			// TODO reduce the child_entries down to a flat list of things we can loop over and log
		}

		// Append(LogEntry)?

		// a zero size container. cannot represent an array, just a container of things
		static const LogEntry Entry(const std::string_view Name, const std::string_view Description, std::convertible_to<const LogEntry> auto const... child_entries)
		{
			std::string type;
			size_t size;
			// TODO figureout size and type
			return LogEntry(Name, Description, type, (char *)nullptr, 1, size, size, child_entries...);
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
			std::string type;
			size_t size;
			// TODO
			return LogEntry(Name, Description, type, (char *)data, Count, size, sizeof(B), (std::invoke(child_entries, data), ...));
		}

		template <is_Fundamental A, is_Class B>
		static const StructMember<B> Entry(const std::string_view Name, const std::string_view Description, A B::*Member, size_t Count = 1)
		{
			std::string type;
			// TODO
			return [=](B const *const Data)
			{
				return LogEntry(Name, Description, type, (char *)&(Data->*Member), Count, sizeof(A), sizeof(B));
			};
		}

		template <is_Fundamental A, is_Class B, size_t Count>
		static const StructMember<B> Entry(const std::string_view Name, const std::string_view Description, A (B::*Member)[Count])
		{
			std::string type;
			// TODO
			return [=](B const *const Data)
			{
				return LogEntry(Name, Description, type, (char *)&(Data->*Member), Count, sizeof(A), sizeof(B));
			};
		}

		//	private:
		LogEntry MainEntry;

		//		static auto error(const std::string_view name, const std::string_view description, const std::string_view msg)
		//		{
		//			return std::runtime_error(std::string("(name:").append(name).append(", description:").append(description).append("): ").append(msg));
		//		}
	};
}