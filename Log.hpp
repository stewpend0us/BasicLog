#include <string_view>
#include <string>

#include "ConstexprString.hpp"
#include "Header.hpp"

namespace BasicLog
{
	template <class T>
	concept is_Fundamental = std::is_fundamental_v<T>;

	template <class T>
	concept is_Class = std::is_class_v<T>;

	class Log
	{
	public:
		struct EntryBase
		{
			// std::string_view name;
			// std::string_view description;
			std::string const header;
		};

		struct FundamentalEntry : EntryBase
		{
			char const *const ptr;
			size_t const size;
		};

		struct ContainerEntry : EntryBase
		{
		};

		struct StructEntry : FundamentalEntry
		{
		};

		struct StructMember: EntryBase
		{
			size_t const size;
		};

		// Log(...Entry)
		// Append(Entry)

		static auto error(std::string_view const name, std::string_view const description, std::string_view const msg)
		{
			return std::runtime_error(std::string("name:").append(name).append(", description:").append(description).append("...").append(msg));
		}

		template <std::string_view const &Name, std::string_view const &Description, size_t const Count = 1>
		static FundamentalEntry Entry(is_Fundamental auto const *const data) requires(Count > 0)
		{
			using Type = std::remove_const_t<std::remove_reference_t<decltype(*data)>>;
			constexpr size_t Size = sizeof(Type);
			constexpr std::string_view const header = Header_fundamental_v<Name, Description, type_name_v<Type>, unsigned_to_string_v<Count>>;
			return {std::string(header), (char const *const)data, Count * Size};
		}

		template <std::string_view const &Name, std::string_view const &Description, size_t const Count>
		static FundamentalEntry Entry(is_Fundamental auto const (&arr)[Count])
		{
			return Entry<Name, Description, Count>(&arr[0]); // force the array to a pointer
		}

		static FundamentalEntry Entry(std::string_view const Name, std::string_view const Description, is_Fundamental auto const *const data, size_t const Count = 1)
		{
			using Type = std::remove_const_t<std::remove_reference_t<decltype(*data)>>;
			constexpr size_t Size = sizeof(Type);
			std::string header = Header_fundamental(Name, Description, type_name_v<Type>, std::to_string(Count));
			return {header, (char const *const)data, Count * Size};
		}

		static StructEntry Entry(const std::string_view Name, const std::string_view Description, is_Class auto const * const data, size_t Count, std::convertible_to<const StructMember> auto const... child_entries)
		{
			constexpr size_t Size = sizeof(*data);
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
				type += ",\n" + child.header;
			}
			std::string header = Header_complex(Name, Description, type, std::to_string(Count));

			if (Size != total_size)
				throw error(Name, Description, "size mismatch. data size is " + std::to_string(Size) + " total children size is " + std::to_string(total_size) +
						"\n           likely due to struct padding or the struct definition is out of sync with the log definition");

			return { header, (char const *const)data, Count *Size };
		}

//		template <is_Fundamental T>
//		static constexpr StructMember Entry()
//		{
//			constexpr size_t Size = sizeof(T);
//			constexpr std::string_view header = Header<>
//
//			return {header, Size};
//		}
	};
}