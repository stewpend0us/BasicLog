#include <string_view>
#include <string>
#include <cstring>
#include <vector>
#include <functional>
#include <unordered_set>
#include <optional>
#include <fstream>

#include <iostream>
#include <iomanip>

#include "type_name.hpp"

namespace BasicLog
{
	//	constexpr std::string_view Version = "0";

	template <class T>
	concept is_Fundamental = std::is_fundamental_v<T>;

	template <class T>
	concept is_Class = std::is_class_v<T>;

	class Log
	{

	private:
		// a location/size of memory (to be logged)
		struct DataChunk
		{
			const char *ptr;
			size_t count;

			// look for contiguous chunks of memory
			static std::vector<DataChunk> condense(std::vector<DataChunk> const &chunk)
			{
				std::vector<DataChunk> result;
				DataChunk L0{chunk[0]}; // may modify this one so make a copy
				size_t i = 1;
				while (i < chunk.size())
				{
					DataChunk const *L1 = &chunk[i];
					if (L0.ptr + L0.count == L1->ptr)
					{
						L0.count += L1->count;
					}
					else
					{
						result.push_back(L0);
						L0 = *L1;
					}
					i++;
				}
				result.push_back(L0);
				return result;
			}
		};

	public:
		// something to be logged
		struct LogEntry; // forward declare so we can define StructMember

		// the member of a struct to be logged
		template <is_Class B>
		using StructMember = std::function<LogEntry(B const *const)>;

		struct LogEntry
		{
			LogEntry(const std::string_view Name, const std::string_view Description, std::vector<LogEntry> child_entries)
					: name(Name), description(Description), type(""), count(1), children(child_entries)
			{
				if (auto err = check_name())
					throw err;
				for (size_t ind = 0; ind < child_entries.size(); ind++)
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

				sort_children();

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

				data = DataChunk::condense(data);
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
				for (auto &h2 : sub_headers)
					h.append(",\n\t").append(h2);
				return h;
			}

			static void sort_entries(std::vector<LogEntry> &entries)
			{
				// sort the children
				std::sort(entries.begin(), entries.end(),
									[](const auto &AA, const auto &BB) -> bool
									{ 
								if (AA.data.empty()) return true;
								if (BB.data.empty()) return false;
								return AA.data[0].ptr < BB.data[0].ptr; });
			}

			void sort_children(void)
			{
				sort_entries(children);
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

//			void append_child(LogEntry & child)
//			{
//				children.push_back(child);
//			}

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

		// how the logged data is written to the file
		enum CompressionMethod
		{
			NONE,
			CompressionMethodCount
		};

		Log(const std::string_view Name, const std::string_view Description, CompressionMethod Compression, std::vector<LogEntry> child_entries)
				: MainEntry(Name, Description, child_entries)
		{

			MainEntry.parent = "";
			MainEntry.parent_index = 0;
			std::vector<LogEntry> AllEntries;
			std::function<void(LogEntry)> flatten;
			flatten = [&](LogEntry L)
			{
				auto children = L.children;
				L.children.clear();
				AllEntries.push_back(L);
				for (auto &c : children)
				{
					flatten(c);
				}
			};
			flatten(MainEntry);
			LogEntry::sort_entries(AllEntries);
			std::vector<DataChunk> AllChunks;
			header.append("{\n");
			// header.append("\"version\":\"").append(Version).append("\",\n");
			header.append("\"compression\":\"").append(CompressionMethodName[Compression]).append("\",\n");
			header.append("\"data_header\":[\n");
			bool first = true;
			for (auto &c : AllEntries)
			{
				AllChunks.insert(AllChunks.end(), c.data.begin(), c.data.end());
				data_size += c.count;
				if (first)
				{
					first = false;
				}
				else
				{
					header.append(",\n");
				}
				header.append(c.header());
			}
			header.append("\n]\n}");

			data = DataChunk::condense(AllChunks);

			// display stuff (for now)
			std::cout << header << '\n';
			for (auto &c : data)
			{
				std::cout << (void *)c.ptr << "     " << c.count << "     " << (void *)(c.ptr + c.count) << '\n';
			}
		}

		Log(const std::string_view Name, const std::string_view Description, CompressionMethod Compression, std::convertible_to<const LogEntry> auto const... child_entries)
		: Log(Name,Description,Compression,{child_entries...})
		{}

		size_t record(char *const dest, size_t size)
		{
			if (size != data_size)
				return 0;

			size_t ind = 0;
			for (auto &e : data)
			{
				memcpy(&dest[ind], e.ptr, e.count);
				ind += e.count;
			}
			return size;
		}

		template <size_t Count>
		size_t record(char const (&arr)[Count])
		{
			return record(&arr[0], Count);
		}

		size_t record(std::fstream &file)
		{
			size_t ind = 0;
			for (auto &e : data)
			{
				file.write(e.ptr, e.count);
				ind += e.count;
			}
			return ind;
		}

//		void append_child(LogEntry & child)
//		{
//			MainEntry.append_child(child);
//		}

	//private:
		LogEntry MainEntry;
		std::string header;
		std::vector<DataChunk> data;
		size_t data_size;
		static constexpr std::string_view CompressionMethodName[CompressionMethodCount] = {"NONE"};

		// static methods
	public:
		static const LogEntry Entry(const std::string_view Name, const std::string_view Description, std::convertible_to<const LogEntry> auto const... child_entries)
		{
			return LogEntry(Name, Description, {child_entries...});
		}

		// a fundamental
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

		// a struct
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

		// a struct member
		template <is_Fundamental A, is_Class B>
		static const StructMember<B> Entry(const std::string_view Name, const std::string_view Description, A B::*Member, size_t Count = 1)
		{
			return [=](B const *const Data)
			{
				return LogEntry(Name, Description, &(Data->*Member), Count);
			};
		}

		// a struct member array
		template <is_Fundamental A, is_Class B, size_t Count>
		static const StructMember<B> Entry(const std::string_view Name, const std::string_view Description, A (B::*Member)[Count])
		{
			return [=](B const *const Data)
			{
				return LogEntry(Name, Description, &((Data->*Member)[0]), Count);
			};
		}
	};
}