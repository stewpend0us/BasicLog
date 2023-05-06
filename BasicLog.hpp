#pragma once
#include <string_view>
#include <string>
#include <cstring>
#include <vector>
#include <numeric>
#include <functional>
#include <unordered_set>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <concepts>
#include <mutex>

#include <iostream>
#include <iomanip>

#include "type_name.hpp"

namespace BasicLog
{
  template <class T>
  concept is_Fundamental = std::integral<T> || std::floating_point<T>;

  template <class T>
  concept is_Enum = std::is_enum_v<T>;

  template <class T>
  concept is_Class = std::is_class_v<T>;

  template <class T>
  concept has_data_size = requires (T t)
  {
    // we'll call these methods and pass their outputs to functions whos inputs are already constrained
    // so we don't need to worry too much about what type they return as long as they exist?
    { t.data() };
    { t.size() };
  };

  class Log
  {

  private:
    // a location/size of memory (to be logged)
    // TODO replace this with "span" ?
    struct DataChunk
    {
      const char* ptr;
      size_t count;

      // look for contiguous chunks of memory
      static std::vector<DataChunk> condense(std::vector<DataChunk> const& chunk);
    };

  public:
    // something to be logged
    struct Entry; // forward declare so we can define StructMemberEntry

    // the member of a struct to be logged
    template <is_Class B>
    using StructMemberEntry = std::function<Entry(B const* const)>;

    struct Entry
    {

      Entry();

      // simple container
      Entry(const std::string_view Name, const std::string_view Description, std::vector<Entry> child_entries);


  // simple container
  Entry(const std::string_view Name, const std::string_view Description, std::convertible_to<Entry> auto const... child_entries)
      : Entry(Name, Description, std::vector<Entry>({child_entries...}))
  {
  }

      // fundamental or array
      template <is_Fundamental A>
      Entry(const std::string_view Name, const std::string_view Description, A const* const Ptr, size_t Count = 1)
        : name(Name), description(Description), type(type_name_v<A>), type_size(sizeof(A)), count(Count), is_contiguous(true)
      {
        check_name();
        data.push_back(DataChunk{ (char*)Ptr, sizeof(A) * Count });
      }

      template <has_data_size T>
      Entry(const std::string_view Name, const std::string_view Description, T const& Obj)
        : Entry(Name, Description, Obj.data(), Obj.size())
      { }

      // allow Enum types
      template <is_Enum A>
      Entry(const std::string_view Name, const std::string_view Description, A const* const Ptr, size_t Count = 1)
        : Entry(Name, Description, (typename std::underlying_type<A>::type const* const)Ptr, Count)
      {
        // this cast is "not allowed" in c++. if you do a static_cast instead of a () cast the compiler barfs.
      }

      // fundamental array
      template <is_Fundamental A, size_t Count>
      Entry(const std::string_view Name, const std::string_view Description, A const (&arr)[Count])
        : Entry(Name, Description, &arr[0], Count)
      { }

      // Enum array
      template <is_Enum A, size_t Count>
      Entry(const std::string_view Name, const std::string_view Description, A const (&arr)[Count])
        : Entry(Name, Description, (typename std::underlying_type<A>::type const* const)& arr[0], Count)
      {
        // this cast is "not allowed" in c++. if you do a static_cast instead of a () cast the compiler barfs.
      }

      // struct or array
      template <is_Class B>
      Entry(const std::string_view Name, const std::string_view Description, B const* const Ptr, size_t Count, const std::vector<StructMemberEntry<B>> child_entries)
        : name(Name), description(Description), type_size(0), count(Count), is_contiguous(true)
      {
        // for the first element in the 'array' aka Ptr[0]...
        for (size_t ind = 0; ind < child_entries.size(); ind++)
        {
          auto c = child_entries[ind](Ptr);
          // update the child with the parents info
          c.parent_index = ind; // order in which they were encoutered
          // add this child to the list
          children.push_back(c);
          type_size += c.type_size;
        }
        type = std::to_string(type_size);

        check_child_names();
        sort_children();

        // extract the sorted order
        std::vector<size_t> order;
        std::transform(children.begin(), children.end(), std::back_inserter(order),
          [](const auto& c) -> size_t { return c.parent_index; });

        // add the sorted child data to the parent data
        // add the sorted child headers to the parent sub_headers
        for (auto& c : children)
        {
          data.insert(data.end(), c.data.begin(), c.data.end());
          //sub_headers.push_back(c.header());
        }
        //children.clear(); // the parent has completely consumed the children at this point

        // for the remaning elements we just need to add their data to the list
        for (size_t i = 1; i < Count; i++)
        {
          for (size_t ind : order)
          {
            auto c = child_entries[ind](Ptr + i);
            data.insert(data.end(), c.data.begin(), c.data.end());
          }
        }
        data = DataChunk::condense(data);
      }

      // struct
      template <is_Class B>
      Entry(const std::string_view Name, const std::string_view Description, B const* const Ptr, const std::vector<StructMemberEntry<B>> child_entries)
        : Entry(Name, Description, Ptr, 1, child_entries)
      { }

      // struct or array
      template <is_Class B>
      Entry(const std::string_view Name, const std::string_view Description, B const* const Ptr, size_t Count, std::convertible_to<const StructMemberEntry<B>> auto const... child_entries)
        : Entry(Name, Description, Ptr, Count, { child_entries... })
      { }

      // struct array
      template <is_Class B, size_t Count>
      Entry(const std::string_view Name, const std::string_view Description, B const (&arr)[Count], std::convertible_to<const StructMemberEntry<B>> auto const... child_entries)
        : Entry(Name, Description, &arr[0], Count, child_entries...)
      { }

      // struct
      template <is_Class B>
      Entry(const std::string_view Name, const std::string_view Description, B const* const Ptr, std::convertible_to<const StructMemberEntry<B>> auto const... child_entries)
        : Entry(Name, Description, Ptr, 1, child_entries...)
      { }

      std::string header() const;

      static void sort_entries(std::vector<Entry>& entries);

      void sort_children(void);

      std::runtime_error error(const std::string_view msg) const;

      void check_name(void) const;

      void check_child_names(void) const;

      // for the header
      std::string name;
      std::string description;
      std::string type;
      size_t type_size;
      size_t count;
      size_t parent_index;

      // for the data
      std::vector<DataChunk> data;
      std::vector<Entry> children;
      bool is_contiguous;
    };

    // how the logged data is written to the file
    enum CompressionMethod
    {
      // remmeber to update "CompressionMethodName" and "CompressionMethodFunction" also!
      RAW,
      DIFF1,
      CompressionMethodCount
    };

    Log();

    Log(const std::string_view Name, const std::string_view Description, CompressionMethod Compression, std::vector<Entry> child_entries);

  Log(const std::string_view Name, const std::string_view Description, CompressionMethod Compression, std::convertible_to<const Entry> auto const... child_entries)
      : Log(Name, Description, Compression, {child_entries...})
  {
  }


    std::string_view name() const;

    void start(std::filesystem::path directory);

    void stop(void);

    void record(void);

    // private:
    void record_NULL(void);

    void record_RAW(void);

    std::vector<char> previous_row;
    void record_DIFF1(void);

    // list of compression method names (to be included in the log header)
    static constexpr std::string_view CompressionMethodName[CompressionMethodCount] = { "RAW", "DIFF1" };

    // list of compression method member functions (to be used to actually record the data)
    typedef void (Log::* RecordFun)(void);
    static constexpr RecordFun CompressionMethodFunction[CompressionMethodCount] = { &Log::record_RAW, &Log::record_DIFF1 };

    Entry MainEntry;						 // each log has a top level entry. this is it.
    std::string header;					 // this logs header
    std::vector<DataChunk> data; // this logs data
    RecordFun selected_recorder; // the selected record method
    RecordFun current_recorder;	 // the recorder that's currently being used (either null or selected)

    std::ofstream log_file;

    // static methods
  public:
    // a struct member
    template <is_Fundamental A, is_Class B>
    static const StructMemberEntry<B> StructMember(const std::string_view Name, const std::string_view Description, A B::* Member, size_t Count = 1)
    {
      return [=](B const* const Data) {
        return Entry(Name, Description, &(Data->*Member), Count);
      };
    }

    // a struct member array
    template <is_Fundamental A, is_Class B, size_t Count>
    static const StructMemberEntry<B> StructMember(const std::string_view Name, const std::string_view Description, A(B::* Member)[Count])
    {
      return [=](B const* const Data) {
        return Entry(Name, Description, &((Data->*Member)[0]), Count);
      };
    }

    static std::string unix_time_formatted(void);

    static std::chrono::system_clock::time_point now(void);

    static auto unix_time_now_ns(void);

    class Manager
    {
      std::filesystem::path root_directory;
      std::vector<Log*> logs;
      bool logging = false;
      std::chrono::system_clock::time_point start_time;
      std::chrono::system_clock::duration max_log_duration = std::chrono::hours(1);
      std::mutex lock;

      void pcheck_child_names(void) const;

      std::filesystem::path pstart(void);

      void pstop(void);

    public:
      Manager();
      Manager(std::filesystem::path root);
  Manager(std::filesystem::path root, std::convertible_to<const Log *> auto... Logs)
      : logs({Logs...})
  {
    pcheck_child_names();
    set_root_directory(root);
  }


      void push_back(Log* L);

      void set_root_directory(const std::filesystem::path path);

      void set_max_log_duration(const std::chrono::system_clock::duration& time);

      std::filesystem::path start(void);

      void stop(void);

      // NOTE: not going to expose a Manager level "record" method each log
      // should be recorded at an appropriate rate based on the application


      void restart_if_needed(void);

      bool is_logging(void);
    };
  };
}