#include <string_view>
#include <string>
#include <vector>
#include <functional>
#include <unordered_set>

// #include "Header.hpp"
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
		struct LoggedItem
		{
			std::string name;
			const char *ptr;
			size_t size;
		};

		struct LogEntry
		{
			LogEntry(const std::string_view Name, const std::string_view Description, const std::string_view Type, char const *const Ptr, size_t Count, size_t Size, size_t Stride)
				: name(Name), description(Description), type(Type), parent_index(0), ptr(Ptr), count(Count), size(Size), stride(Stride)
			{
				static constexpr std::string_view ok1 = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
				static constexpr std::string_view ok2 = "_0123456789";
				if (name.size() == 0)
					throw error("name must be at least one character long");
				if (ok1.find(name[0]) == std::string_view::npos)
					throw error("name must start with a letter");
				for (size_t i = 1; i < name.size(); i++)
				{
					if (ok2.find(name[i]) == std::string_view::npos && ok1.find(name[i]) == std::string_view::npos)
						throw error("name must not contain '" + name.substr(i,1) + "' only letters, numbers, and _");
				}
			}

			LogEntry(const std::string_view Name, const std::string_view Description, size_t Count, std::convertible_to<LogEntry> auto const... child_entries)
				: name(Name), description(Description), type(""), parent_index(0), ptr(nullptr), count(Count), size(0), stride(0), children({child_entries...})
			{
				std::unordered_set<std::string_view> child_names;
				size_t i = 0;
				for (auto c : children)
				{
					if (!child_names.insert(c.name).second)
						throw error("already contains a child named \"" + c.name + "\"");
					c.parent = Name;
					c.parent_index = i++;
				}
			}

			std::string header() const
			{
				static constexpr std::string_view q = "\"";
				static constexpr std::string_view c = ",";
				static constexpr std::string_view qc = "\",";
				static constexpr std::string_view l = "{";
				static constexpr std::string_view r = "}";
				static constexpr std::string_view name = "\"name\":";
				static constexpr std::string_view desc = "\"desc\":";
				static constexpr std::string_view type = "\"type\":";
				static constexpr std::string_view parent = "\"parent\":";
				static constexpr std::string_view ind = "\"ind\":";
				static constexpr std::string_view count = "\"count\":";

				return std::string(l).append(name).append(q).append(this->name).append(qc).append(desc).append(q).append(this->description).append(qc).append(type).append(q).append(this->type).append(qc).append(count).append(std::to_string(this->count)).append(c).append(parent).append(q).append(this->parent).append(qc).append(ind).append(std::to_string(this->parent_index)).append(r);
			}
		std::runtime_error error(const std::string_view msg) const
		{
			return std::runtime_error(std::string("\"").append(name).append("\", \"").append(description).append("\": ").append(msg));
		}

			// header fields
			std::string name;
			std::string description;
			std::string type;
			std::string_view parent;
			size_t parent_index;

			// Item fields
			const char *const ptr; // start of the data
			size_t count;		   // number of elements
			size_t size;		   // number of (contiguous) bytes per element
			size_t stride;		   // number of (contiguous) bytes between elements

			std::vector<LogEntry> children;
		};

		template <is_Class B>
		using StructMember = std::function<LogEntry(B const *const, size_t)>;

		Log(const std::string_view Name, const std::string_view Description, std::convertible_to<const LogEntry> auto const... child_entries)
			: MainEntry(Entry(Name, Description, child_entries...))
		{
			std::vector<LoggedItem> AllItems;
			std::function<void(LogEntry)> extract;
			extract = [&](LogEntry L)
			{
				if (L.children.empty())
				{
					// reduce this to a list of address with size
					for (size_t i = 0, pos = 0; i < L.count; i++)
					{
						AllItems.push_back(LoggedItem{L.name, L.ptr + pos, L.size});
						pos += L.stride;
					}
					return;
				}
				for (auto c : L.children)
				{
					extract(c);
				}
			};
			extract(MainEntry);


			//TODO we need to keep track of this sort and re-order the list of headers to match

			// sort the list by address
			std::sort(AllItems.begin(), AllItems.end(), [](const LoggedItem &A, const LoggedItem &B) -> bool
					  { return A.ptr < B.ptr; });

			// look for contiguous chunks of memory
			std::vector<LoggedItem> FinalItems;
			{
				LoggedItem L = AllItems[0]; // may modify this one so make a copy
				LoggedItem const *Li = &L;
				size_t i = 1;
				while (i < AllItems.size())
				{
					Li = &AllItems[i];
					if (L.ptr + L.size == Li->ptr)
					{
						L.name += "," + Li->name;
						L.size += Li->size;
					}
					else
					{
						FinalItems.push_back(L);
						L = *Li;
					}
					i++;
				}
				FinalItems.push_back(L);
			}

			const auto w = std::setw(7);
			std::cout << std::setw(15) << "name" << std::setw(20) << "address" << w << "size" << '\n';
			for (auto i : AllItems)
			{
				std::cout << std::setw(15) << i.name << std::setw(20) << (void *)i.ptr << w << i.size << '\n';
			}
			std::cout << "=======================final=======================\n";
			for (auto i : FinalItems)
			{
				std::cout << std::setw(15) << i.name << std::setw(20) << (void *)i.ptr << w << i.size << '\n';
			}
		}

		// Append(LogEntry)?

		// a zero size container. cannot represent an array, just a container of things
		static const LogEntry Entry(const std::string_view Name, const std::string_view Description, std::convertible_to<const LogEntry> auto const... child_entries)
		{
			return LogEntry(Name, Description, 1, child_entries...);
		}

		// a fundamental
		static const LogEntry Entry(const std::string_view Name, const std::string_view Description, is_Fundamental auto const *const data, size_t Count, size_t Stride)
		{
			using Type = std::remove_cvref_t<decltype(*data)>;
			if (Stride == 1)
			{
				return LogEntry(Name, Description, type_name_v<Type>, (char *)data, 1, sizeof(Type) * Count, sizeof(Type) * Count);
			}
			return LogEntry(Name, Description, type_name_v<Type>, (char *)data, Count, sizeof(Type), sizeof(Type) * Stride);
		}

		static const LogEntry Entry(const std::string_view Name, const std::string_view Description, is_Fundamental auto const *const data, size_t Count = 1)
		{
			return Entry(Name, Description, data, Count, 1);
		}

		// a fundamental array
		template <size_t Count>
		static const LogEntry Entry(const std::string_view Name, const std::string_view Description, is_Fundamental auto const (&arr)[Count])
		{
			return Entry(Name, Description, &arr[0], Count, 1);
		}

		// a struct (user supplied spec)
		template <is_Class B>
		static const LogEntry Entry(const std::string_view Name, const std::string_view Description, B const *const data, size_t Count, std::convertible_to<const StructMember<B>> auto const... child_entries)
		{
			return LogEntry(Name, Description, Count, std::invoke(child_entries, data, Count)...);
		}

		// a struct array
		template <is_Class B, size_t Count>
		static const LogEntry Entry(const std::string_view Name, const std::string_view Description, B const (&arr)[Count], std::convertible_to<const StructMember<B>> auto const... child_entries)
		{
			return Entry(Name, Description, &arr[0], Count, child_entries...);
		}

		template <is_Fundamental A, is_Class B>
		static const StructMember<B> Entry(const std::string_view Name, const std::string_view Description, A B::*Member, size_t Count = 1)
		{
			return [=](B const *const Data, size_t DataCount)
			{
				return LogEntry(Name, Description, type_name_v<A>, (char *)&(Data->*Member), DataCount, sizeof(A) * Count, sizeof(B));
			};
		}

		template <is_Fundamental A, is_Class B, size_t Count>
		static const StructMember<B> Entry(const std::string_view Name, const std::string_view Description, A (B::*Member)[Count])
		{
			return [=](B const *const Data, size_t DataCount)
			{
				return LogEntry(Name, Description, type_name_v<A>, (char *)&(Data->*Member), DataCount, sizeof(A) * Count, sizeof(B));
			};
		}

		//	private:
		LogEntry MainEntry;

	};
}