#include <string_view>
#include <type_traits>
#include "ConstexprString.hpp"
#include "Header.hpp"

namespace BasicLog
{
	template <class T>
	concept is_Fundamental = std::is_fundamental_v<T>;

	class Log
	{
	public:
		struct EntryBase
		{
			// std::string_view name;
			// std::string_view description;
			std::string_view header;
		};

		struct FundamentalEntry : EntryBase
		{
			char *ptr;
			size_t size;
		};

		struct ContainerEntry : EntryBase
		{
		};

		struct StructEntry : FundamentalEntry
		{
		};

		struct StructChild : EntryBase
		{
			size_t size;
		};

		// Log(...Entry)
		// Append(Entry)

		template <std::string_view const &Name, std::string_view const &Description>
		static constexpr FundamentalEntry Entry(is_Fundamental auto const * const entry)
		{
			constexpr size_t size = sizeof(decltype(*entry));
			return {Header<Name, Description, std::remove_const_t<std::remove_pointer_t<decltype(entry)>>, size>(), (char *)entry, 1};
		}
	};
}