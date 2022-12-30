#include <string_view>
#include <string>
#include <vector>

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
		struct LogEntry
		{
			LogEntry(const std::string_view Name, const std::string_view Description, const std::string_view Type, char const *Ptr, size_t Count, size_t Size, size_t Stride)
				: name(Name), description(Description), type(Type), ptr(Ptr), count(Count), size(Size), stride(Stride)
			{
			}
			std::string header() const
			{
				return Header(name, description, type, count * size);
			}
			const std::string_view name;
			const std::string_view description;
			const std::string_view type;
			const char *const ptr;
			const size_t count;
			const size_t size;
			const size_t stride;
			std::vector<LogEntry> children;
		};

		struct StructMember
		{
			StructMember(const std::string_view Name, const std::string_view Description, const std::string_view Type, size_t Count, size_t Size, size_t Alignment)
				: name(Name), description(Description), type(Type), count(Count), size(Size), alignment(Alignment)
			{
			}
			std::string header() const
			{
				return Header(name, description, type, count * size);
			}
			const std::string_view name;
			const std::string_view description;
			const std::string_view type;
			const size_t count;
			const size_t size;
			const size_t alignment;
		};

		template <typename T>
		struct StructMember2
		{
			StructMember2(const std::string_view Name, const std::string_view Description, const std::string_view Type, size_t Count, size_t Size, size_t Alignment)
				: name(Name), description(Description), type(Type), count(Count), size(Size), alignment(Alignment)
			{
			}
			std::string header() const
			{
				return Header(name, description, type, count * size);
			}
			const std::string_view name;
			const std::string_view description;
			const std::string_view type;
			const size_t count;
			const size_t size;
			const size_t alignment;
		};

		Log(const std::string_view Name, const std::string_view Description, std::convertible_to<const LogEntry> auto const... child_entries)
			: MainEntry(Entry(Name, Description, child_entries...))
		{
		}
		// Append(Entry)

		// a zero size container. cannot represent an array, just a container of things
		static const LogEntry Entry(const std::string_view Name, const std::string_view Description, std::convertible_to<const LogEntry> auto const... child_entries)
		{
			LogEntry child[] = {child_entries...};
			size_t size = child[0].count * child[0].size;
			std::string type(child[0].header());
			for (size_t i = 1; i < sizeof...(child_entries); i++)
			{
				size += child[i].count * child[i].size;
				type += ",\n" + child[i].header();
			}
			return LogEntry(Name, Description, type, nullptr, 1, size, size);
		}

		// a fundamental
		static const LogEntry Entry(const std::string_view Name, const std::string_view Description, is_Fundamental auto const *const data, size_t Count = 1)
		{
			using Type = std::remove_cvref_t<decltype(*data)>;
			const auto Size = sizeof(Type);
			return LogEntry(Name, Description, type_name_v<Type>, (char *)data, Count, Size, Size);
		}

		// a fundamental array
		template <size_t Count>
		static const LogEntry Entry(const std::string_view Name, const std::string_view Description, is_Fundamental auto const (&arr)[Count])
		{
			return Entry(Name, Description, &arr[0], Count); // force the array to a pointer
		}

		// a struct (user supplied spec)
		template <is_Class T>
		static const LogEntry Entry(const std::string_view Name, const std::string_view Description, T const *const data, size_t Count, std::convertible_to<const StructMember2<T>> auto const... child_entries)
		{
			constexpr size_t Size = sizeof(*data);
			constexpr size_t Width = alignof(*data);
			constexpr size_t Height = Size / Width;

			const StructMember2<T> child[] = {child_entries...};
			size_t total_size = 0;
			size_t ipos = 0;
			size_t jpos = 0;
			std::string type(child[0].header());
			for (size_t i = 1; i < sizeof...(child_entries); i++)
			{
				type.append(",").append(child[i].header());
				for (size_t j = 0; j < child[i].count; j++)
				{
					jpos += child[i].size;
					if (jpos == (Width - 1))
					{
						jpos = 0;
						ipos++;
					}
					else if (jpos > (Width - 1))
					{
						jpos = Width;
						ipos++;
					}
				}
			}
			if (jpos == Height && ipos > 0)
			{
				return LogEntry(Name, Description, type, (char *)data, Count, Size, Size);
			}

			std::string data_rep("");
			for (size_t i = 0; i < Height; i++)
			{
				data_rep += '\t';
				for (size_t j = 0; j < Width; j++)
				{
					const std::string n("????");
					data_rep += "|" + n;
				}
				data_rep += "|\n";
			}

			std::string child_rep("");
			size_t ind = 0; // child index
			size_t pos = 0; // position within the child
			// while there are still children
			while (ind < sizeof...(child_entries))
			{
				auto s = child[ind].size;
				auto c = child[ind].count;
				auto a = child[ind].alignment;
				size_t j = 0;

				// print out this row
				child_rep += '\t';
				while (j < Width)
				{
					if (j % a == 0 && pos < c) // child in byte
					{
						// print name in byte
						child_rep += "|" + str(child[ind].name, pos * 4, s);
						pos += 1;
						j += s;
						if (pos >= c) // child consumed
						{
							// next child
							ind++;
							if (ind >= sizeof...(child_entries))
								break;
							s = child[ind].size;
							c = child[ind].count;
							a = child[ind].alignment;
							pos = 0;
							continue;
						}
					}
					else
					{
					const std::string n("----");
						child_rep += "|" + n;
						j += 1;
					}
				}
				child_rep += "|\n";
			}
			throw error(Name, Description, "size mismatch. data size is " + std::to_string(Size) + " total children size is " + std::to_string(total_size) + ". likely due to struct padding or the struct definition is out of sync with the log definition.\nstruct layout:\n" + data_rep + "provided layout:\n" + child_rep);
		}

		static std::string str(const std::string_view name, size_t pos, size_t size)
		{
			const size_t s = 4 * size;
			std::string result(s + size - 1, ' ');
			if (pos > name.size())
				return result;
			const auto delta = name.size() - pos;
			const auto len = delta < s ? delta : s;
			return result.replace(0, len, name, pos, s);
		}

		// single struct
		static const LogEntry Entry(const std::string_view Name, const std::string_view Description, is_Class auto const *const data, std::convertible_to<const StructMember> auto const... child_entries)
		{
			return Entry(Name, Description, data, 1, child_entries...);
		}

		// a struct array
		template <size_t Count>
		static const LogEntry Entry(const std::string_view Name, const std::string_view Description, is_Class auto const (&arr)[Count], std::convertible_to<const StructMember> auto const... child_entries)
		{
			return Entry(Name, Description, &arr[0], Count, child_entries...);
		}

		// a struct member description
		template <is_Fundamental Type, size_t Count = 1>
		static const StructMember Entry(const std::string_view Name, const std::string_view Description)
		{
			return StructMember(Name, Description, type_name_v<Type>, Count, sizeof(Type), alignof(Type));
		}

		template <is_Fundamental A, class B>
		static const StructMember2<B> Entry(const std::string_view Name, const std::string_view Description, A B::*mem)
		{
			return StructMember2<B>(Name, Description, type_name_v<A>, 1, sizeof(A), alignof(A));
		}

		template <is_Fundamental A, class B, size_t Count>
		static const StructMember2<B> Entry(const std::string_view Name, const std::string_view Description, A (B::*mem)[Count])
		{
			return StructMember2<B>(Name, Description, type_name_v<A>, Count, sizeof(A), alignof(A));
		}
		//	private:
		LogEntry MainEntry;

		static auto error(const std::string_view name, const std::string_view description, const std::string_view msg)
		{
			return std::runtime_error(std::string("(name:").append(name).append(", description:").append(description).append("): ").append(msg));
		}
	};
}