#include <string_view>
#include <string>
#include <vector>
#include <functional>
#include <unordered_set>
#include <optional>

#include <iostream>
#include <iomanip>

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
		struct DataChunk
		{
			const char *ptr;
			size_t count;
		};

		struct LogEntry;

		template <is_Class B>
		using StructMember = std::function<LogEntry(B const *const)>;

		struct LogEntry
		{
			LogEntry(const std::string_view Name, const std::string_view Description, std::convertible_to<const LogEntry> auto const... child_entries)
				: name(Name), description(Description), type(""), count(1), children({child_entries...})
			{
				if (auto err = check_name())
					throw err;
				for (size_t ind = 0; ind < sizeof...(child_entries); ind++)
				{
					children[ind].parent = name;
					children[ind].parent_index = ind;
				}
			}

			template <is_Fundamental A>
			LogEntry(const std::string_view Name, const std::string_view Description, A const *const Ptr, size_t Count)
				: name(Name), description(Description), type(type_name_v<A>), count(Count)
			{
				if (auto err = check_name())
					throw err;
				data.push_back(DataChunk{(char *)Ptr, sizeof(A) * Count});
			}

			template <is_Class B>
			LogEntry(const std::string_view Name, const std::string_view Description, B const *const Ptr, size_t Count, std::convertible_to<const StructMember<B>> auto const... child_entries)
				: name(Name), description(Description), type(""), count(Count)
			{
				// dump these into an array for easier use
				std::array<StructMember<B>, sizeof...(child_entries)> SM = {child_entries...};

				// for the first element...
				std::unordered_set<std::string_view> unique_child_names;
				for (size_t ind = 0; ind < sizeof...(child_entries); ind++)
				{
					auto c = SM[ind](Ptr);
					// check that the child names are unique
					if (!unique_child_names.insert(c.name).second)
						throw error("already contains a child named \"" + c.name + "\"");
					// update the child with the parents info
					c.parent = name;
					c.parent_index = ind; // order in which they were encoutered
					// add this child to the list
					children.push_back(c);
				}

				// sort the children
				std::sort(children.begin(), children.end(),
						  [](const auto &AA, const auto &BB) -> bool
						  { 
								if (AA.data.empty()) return true;
								if (BB.data.empty()) return false;
								return AA.data[0].ptr < BB.data[0].ptr; });

				// extract the sorted order
				std::vector<size_t> order;
				std::transform(children.begin(), children.end(), std::back_inserter(order),
							   [](const auto &c) -> size_t
							   { return c.parent_index; });

				// add the sorted child data to the parent data
				// add the sorted child headers to the parent sub_headers
				for (auto &c : children)
				{
					data.insert(data.end(), c.data.begin(), c.data.end());
					sub_headers.push_back(c.header());
				}
				children.clear(); // the parent has completely consumed the children at this point

				// for the remaning elements we just need to add their data to the list
				for (size_t i = 1; i < Count; i++)
				{
					for (size_t ind : order)
					{
						auto c = SM[ind](Ptr + i);
						data.insert(data.end(), c.data.begin(), c.data.end());
					}
				}

				// TODO 'condense' the list of data
				// implemented in the Log constructor currently
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
				//{"name":"{name}","desc":"{description}","type":"{type}","count":{count},"parent":"{parent_name}","ind":{parent_index}}
				auto h = std::string(l).append(name).append(q).append(this->name).append(qc).append(desc).append(q).append(this->description).append(qc).append(type).append(q).append(this->type).append(qc).append(count).append(std::to_string(this->count)).append(c).append(parent).append(q).append(this->parent).append(qc).append(ind).append(std::to_string(this->parent_index)).append(r);
				for (auto & h2 : sub_headers)
					h.append(",\n\t").append(h2);
				return h;
			}

			std::runtime_error error(const std::string_view msg) const
			{
				return std::runtime_error(std::string("\"").append(name).append("\", \"").append(description).append("\": ").append(msg));
			}

			std::optional<std::runtime_error> check_name() const
			{
				static const std::string ok1 = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
				static const std::string ok2 = "_0123456789";
				if (name.size() == 0)
					return error("name must be at least one character long");
				if (ok1.find(name[0]) == std::string::npos)
					return error("name must start with a letter");
				for (size_t i = 1; i < name.size(); i++)
				{
					if (ok2.find(name[i]) == std::string::npos && ok1.find(name[i]) == std::string::npos)
						return error(std::string("name must not contain '").append(name.substr(i, 1)).append("' only letters, numbers, and _"));
				}
				return {};
			}

			// for the header
			std::string name;
			std::string description;
			std::string_view type;
			size_t count;
			std::string parent;
			size_t parent_index;
			std::vector<std::string> sub_headers;

			// for the data
			std::vector<DataChunk> data;
			std::vector<LogEntry> children;
		};

		Log(const std::string_view Name, const std::string_view Description, std::convertible_to<const LogEntry> auto const... child_entries)
			: MainEntry(Entry(Name, Description, child_entries...))
		{
			MainEntry.parent = "";
			MainEntry.parent_index = 0;
			std::vector<LogEntry> AllItems;
			std::function<void(LogEntry)> flatten;
			flatten = [&](LogEntry L)
			{
				AllItems.push_back(L);
				for (auto &c : L.children)
				{
					flatten(c);
				}
			};
			flatten(MainEntry);

			for (auto &p : AllItems)
			{
				std::cout << p.header() << '\n';
				for (auto &d : p.data)
				{
					std::cout << (void *)d.ptr << "    " << d.count << "    " << (void *)(d.ptr + d.count) << '\n';
				}
			}
			std::cout << '\n';

			//// sort the list by address
			// std::sort(AllItems.begin(), AllItems.end(), [](const auto &A, const auto &B) -> bool
			//		  { return A.ptr < B.ptr; });

			// std::vector<std::string_view> headers;
			// std::vector<LoggedItem> sorted_data;
			// for (auto &p : AllItems)
			//{
			//	headers.push_back(p.header());
			//	sorted_data.insert(sorted_data.end(), p.data.begin(), p.data.end());
			//	std::cout << p.header() << '\n';
			// }
			// std::cout << '\n';

			//// look for contiguous chunks of memory
			// std::vector<LoggedItem> FinalItems;
			//{
			//	LoggedItem L{AllItems[0].name, AllItems[0].ptr, AllItems[0].count, AllItems[0].size, AllItems[0].stride}; // may modify this one so make a copy
			//	LoggedItem const *Li = &L;
			//	size_t i = 1;
			//	while (i < AllItems.size())
			//	{
			//		auto tmp = LoggedItem{AllItems[i].name, AllItems[i].ptr, AllItems[i].count, AllItems[i].size, AllItems[i].stride};
			//		Li = &tmp;
			//		if (L.ptr + L.size == Li->ptr)
			//		{
			//			L.name += "," + Li->name;
			//			L.size += Li->size;
			//		}
			//		else
			//		{
			//			FinalItems.push_back(L);
			//			L = *Li;
			//		}
			//		i++;
			//	}
			//	FinalItems.push_back(L);
			// }

			// const auto w = std::setw(7);
			// std::cout << std::setw(15) << "name" << std::setw(20) << "address" << w << "count" << w << "size" << w << "stride" << '\n';
			// std::cout << "=======================in order (only not)=======================\n";
			// auto first = sorted_data[0].ptr;
			// for (auto &i : sorted_data)
			//{
			//	std::cout << std::setw(15) << i.name << std::setw(20) << i.ptr - first << w << i.count << w << i.size << w << i.stride << '\n';
			// }
			// std::cout << "=======================junk?=====================================\n";
			// first = AllItems[5].ptr;
			// for (auto &i : AllItems)
			//{
			//	std::cout << std::setw(15) << i.name << std::setw(20) << i.ptr - first << w << i.count << w << i.size << w << i.stride << '\n';
			// }
			// std::cout << "=======================final=====================================\n";
			// first = FinalItems[0].ptr;
			// for (auto &i : FinalItems)
			//{
			//	std::cout << std::setw(15) << i.name << std::setw(20) << i.ptr - first << w << i.count << w << i.size << w << i.stride << '\n';
			// }
		}

		// Append(LogEntry)?

		// a zero size container. cannot represent an array, just a container of things
		static const LogEntry Entry(const std::string_view Name, const std::string_view Description, std::convertible_to<const LogEntry> auto const... child_entries)
		{
			return LogEntry(Name, Description, child_entries...);
		}

		// a fundamental
		static const LogEntry Entry(const std::string_view Name, const std::string_view Description, is_Fundamental auto const *const data, size_t Count, size_t Stride)
		{
			return LogEntry(Name, Description, data, Count);
		}

		static const LogEntry Entry(const std::string_view Name, const std::string_view Description, is_Fundamental auto const *const data, size_t Count = 1)
		{
			return LogEntry(Name, Description, data, Count);
		}

		// a fundamental array
		template <size_t Count>
		static const LogEntry Entry(const std::string_view Name, const std::string_view Description, is_Fundamental auto const (&arr)[Count])
		{
			return Entry(Name, Description, &arr[0], Count);
		}

		// a struct (user supplied spec)
		template <is_Class B>
		static const LogEntry Entry(const std::string_view Name, const std::string_view Description, B const *const data, size_t Count, std::convertible_to<const StructMember<B>> auto const... child_entries)
		{
			return LogEntry(Name, Description, data, Count, child_entries...);
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
			return [=](B const *const Data)
			{
				return LogEntry(Name, Description, &(Data->*Member), Count);
			};
		}

		template <is_Fundamental A, is_Class B, size_t Count>
		static const StructMember<B> Entry(const std::string_view Name, const std::string_view Description, A (B::*Member)[Count])
		{
			return [=](B const *const Data)
			{
				return LogEntry(Name, Description, &((Data->*Member)[0]), Count);
			};
		}

		//	private:
		LogEntry MainEntry;
	};
}