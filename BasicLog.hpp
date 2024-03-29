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
      static std::vector<DataChunk> condense(std::vector<DataChunk> const& chunk)
      {
        std::vector<DataChunk> result;
        if (!chunk.empty())
        {
          DataChunk L0{ chunk[0] }; // may modify this one so make a copy
          size_t i = 1;
          while (i < chunk.size())
          {
            DataChunk const* L1 = &chunk[i];
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
        }
        return result;
      }
    };

  public:
    // something to be logged
    struct Entry; // forward declare so we can define StructMemberEntry

    // the member of a struct to be logged
    template <is_Class B>
    using StructMemberEntry = std::function<Entry(B const* const)>;

    struct Entry
    {

      Entry() { }

      // simple container
      Entry(const std::string_view Name, const std::string_view Description, std::vector<Entry> child_entries)
        : name(Name), description(Description), type(""), type_size(0), count(1), children(child_entries), is_contiguous(false)
      {
        check_name();
        check_child_names();
        for (size_t ind = 0; ind < child_entries.size(); ind++)
        {
          children[ind].parent_index = ind;
          type_size += children[ind].type_size;
        }
      }

      // simple container
      Entry(const std::string_view Name, const std::string_view Description, std::convertible_to<Entry> auto const... child_entries)
        : Entry(Name, Description, std::vector<Entry>({ child_entries... }))
      { }

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
        static constexpr std::string_view ind = "\"ind\":";
        static constexpr std::string_view count = "\"count\":";
        //{"name":"{name}","desc":"{description}","type":"{type}","count":{count},"parent":"{parent_name}","ind":{parent_index}}
        auto h = std::string(l).append(name).append(q).append(this->name).append(qc).append(desc).append(q).append(this->description).append(qc).append(type).append(q).append(this->type).append(qc).append(count).append(std::to_string(this->count)).append(c).append(ind).append(std::to_string(this->parent_index)).append(r);
        if (is_contiguous)
        {
          for (auto& h2 : children)
            h.append(",\n").append(h2.header());
        }
        return h;
      }

      static void sort_entries(std::vector<Entry>& entries)
      {
        // sort the children
        std::sort(entries.begin(), entries.end(),
          [](const auto& AA, const auto& BB) -> bool {
            bool A = AA.data.empty();
            bool B = BB.data.empty();
            // sort containers by name length
            if (A && B) return AA.name.size() < BB.name.size();
            // containers above data
            if (A) return true;
            if (B) return false;
            // sort based on data location
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

      void check_name(void) const
      {
        static const std::string ok1 = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
        static const std::string ok2 = "_0123456789";
        if (name.size() == 0)
          throw error("name must be at least one character long");
        if (ok1.find(name[0]) == std::string::npos)
          throw error("name must start with a letter");
        for (size_t i = 1; i < name.size(); i++)
        {
          if (ok2.find(name[i]) == std::string::npos && ok1.find(name[i]) == std::string::npos)
            throw error(std::string("name must not contain '").append(name.substr(i, 1)).append("' only letters, numbers, and _"));
        }
      }

      void check_child_names(void) const
      {
        // check that the child names are unique
        std::unordered_set<std::string_view> unique_child_names;
        for (size_t ind = 0; ind < children.size(); ind++)
        {
          if (!unique_child_names.insert(children[ind].name).second)
            throw error("already contains a child named \"" + children[ind].name + "\"");
        }
      }

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

    Log() { };

    Log(const std::string_view Name, const std::string_view Description, CompressionMethod Compression, std::vector<Entry> child_entries)
      : MainEntry(Name, Description, child_entries), selected_recorder(CompressionMethodFunction[Compression]), current_recorder(&Log::record_NULL)
    {
      MainEntry.parent_index = 0;
      std::vector<Entry> AllEntries;
      std::function<void(Entry)> flatten;
      flatten = [&](Entry L) {

        //        auto children = L.children;
        for (auto& c : L.children)
        {
          c.name = L.name + "." + c.name;
        }
        //L.children.clear();
        AllEntries.push_back(L);
        if (!L.is_contiguous)
        {
          for (auto& c : L.children)
          {
            flatten(c);
          }
        }
      };
      flatten(MainEntry);
      Entry::sort_entries(AllEntries);
      std::vector<DataChunk> AllChunks;
      header.append("{\n");
      header.append("\"compression\":\"").append(CompressionMethodName[Compression]).append("\",\n");
      header.append("\"data_header\":[\n");
      bool first = true;
      for (auto& c : AllEntries)
      {
        AllChunks.insert(AllChunks.end(), c.data.begin(), c.data.end());
        if (first)
          first = false;
        else
          header.append(",\n");
        header.append(c.header());
      }
      data = DataChunk::condense(AllChunks);
      size_t total_size = std::accumulate(data.begin(), data.end(), 0, [](size_t sum, const DataChunk& E) { return sum + E.count; });

      previous_row = std::vector<char>(total_size, 0);

      header.append("\n],\n");
      header.append("\"row_size\":").append(std::to_string(total_size));
      header.append("\n}");

      // display stuff (for now)
      std::cout << header << '\n';
      for (auto& c : data)
      {
        std::cout << (void*)c.ptr << "     " << c.count << "     " << (void*)(c.ptr + c.count) << '\n';
      }
    }

    Log(const std::string_view Name, const std::string_view Description, CompressionMethod Compression, std::convertible_to<const Entry> auto const... child_entries)
      : Log(Name, Description, Compression, { child_entries... })
    { }

    std::string_view name() const
    {
      return MainEntry.name;
    }

    void start(std::filesystem::path directory)
    {
      if (directory.empty())
        throw MainEntry.error("cannot begin logging to an empty directory");
      // create the new file
      auto file_path = directory / (MainEntry.name + ".cap");
      log_file.open(file_path, std::ios_base::binary | std::ios_base::trunc);
      if (!log_file)
        throw MainEntry.error(std::string("failed to create log file: ").append(file_path));

      log_file << header << '\0';						// add the header followed by 0
      current_recorder = selected_recorder; // update the recorder function
      // init recorder states
      std::fill(previous_row.begin(), previous_row.end(), 0);
    }

    void stop(void)
    {
      current_recorder = &Log::record_NULL;
      log_file.flush();
      log_file.close();
    }

    void record(void)
    {
      (this->*current_recorder)();
    }

    // private:
    void record_NULL(void) { }

    void record_RAW(void)
    {
      for (auto& e : data)
      {
        log_file.write(e.ptr, e.count);
      }
    }

    std::vector<char> previous_row;
    void record_DIFF1(void)
    {
      const size_t num_bytes = previous_row.size();
      const size_t num_bits = num_bytes / 8 + (num_bytes % 8 > 0); // number of bytes we need to get at least one bit per byte
      std::vector<char> prefix(num_bits, 0);
      std::vector<char> delta(num_bytes, 0);
      std::vector<char> row(num_bytes, 0);


      // copy all the data to be logged into a buffer
      size_t pos = 0;
      for (auto& e : data)
      {
        for (size_t i = 0; i < e.count; i++)
        {
          row[pos++] = e.ptr[i];
        }
      }

      // compute the delta
      for (size_t i = 0; i < num_bytes; i++)
        delta[i] = row[i] - previous_row[i];

      // save the previous row
      previous_row = row;

      // compute the prefix and data to be logged
//      std::fill(prefix.begin(), prefix.end(), 0); // zero the prefix
      std::vector<char> bytes_to_log;
      for (size_t i = 0; i < num_bytes; i++)
      {
        if (delta[i] == 0)
          continue;
        size_t h_ind = i / 8;
        int b_ind = i % 8;
        prefix[h_ind] |= 1U << b_ind; // set the bit
        bytes_to_log.push_back(delta[i]); // insert the data
      }

      // finally write the prefix and data
      log_file.write(prefix.data(), prefix.size());
      log_file.write(bytes_to_log.data(), bytes_to_log.size());
    }

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

    static std::string unix_time_formatted(void)
    {
      time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()); // system clock is 'Unix time' in c++20
      struct tm tm;
      localtime_r(&now, &tm);
      std::stringstream ss;
      ss << std::put_time(&tm, "%Y%m%d_%H%M%S%z");
      return ss.str();
    }

    static std::chrono::system_clock::time_point now(void)
    {
      return std::chrono::system_clock::now();
    }

    static auto unix_time_now_ns(void)
    {
      return now().time_since_epoch().count();
    }

    class Manager
    {
      std::filesystem::path root_directory;
      std::vector<Log*> logs;
      bool logging = false;
      std::chrono::system_clock::time_point start_time;
      std::chrono::system_clock::duration max_log_duration = std::chrono::hours(1);
      std::mutex lock;

      void pcheck_child_names(void) const
      {
        // check that the child names are unique
        std::unordered_set<std::string_view> unique_names;
        std::for_each(logs.begin(), logs.end(), [&](const Log* L) {
          if (!unique_names.insert(L->name()).second)
            throw std::runtime_error(std::string("Log::Manager instance already contains a log named \"").append(L->name()).append("\"")); });
      }

      std::filesystem::path pstart(void)
      {
        std::filesystem::path dir = root_directory / unix_time_formatted();
        std::filesystem::create_directories(dir);
        std::for_each(logs.begin(), logs.end(), [&](Log* L) { L->start(dir); });
        start_time = Log::now();
        logging = true;
        return dir;
      }

      void pstop(void)
      {
        std::for_each(logs.begin(), logs.end(), [](Log* L) { L->stop(); });
        logging = false;
      }

    public:
      Manager() { }
      Manager(std::filesystem::path root)
      {
        set_root_directory(root);
      }
      Manager(std::filesystem::path root, std::convertible_to<const Log*> auto... Logs)
        : logs({ Logs... })
      {
        pcheck_child_names();
        set_root_directory(root);
      }

      void push_back(Log* L)
      {
        lock.lock();
        bool was_logging = logging;
        if (was_logging) pstop();
        logs.push_back(L);
        pcheck_child_names();
        if (was_logging) pstart();
        lock.unlock();
      }

      void set_root_directory(const std::filesystem::path path)
      {
        lock.lock();
        bool was_logging = logging;
        if (was_logging) pstop();
        std::filesystem::create_directories(path); // may fail
        root_directory = path;
        if (was_logging) pstart();
        lock.unlock();
      }

      void set_max_log_duration(const std::chrono::system_clock::duration& time)
      {
        lock.lock();
        max_log_duration = time;
        lock.unlock();
      }

      std::filesystem::path start(void)
      {
        lock.lock();
        auto dir = pstart();
        lock.unlock();
        return dir;
      }

      void stop(void)
      {
        lock.lock();
        std::for_each(logs.begin(), logs.end(), [](Log* L) { L->stop(); });
        logging = false;
        lock.unlock();
      }

      // NOTE: not going to expose a Manager level "record" method each log
      // should be recorded at an appropriate rate based on the application


      void restart_if_needed(void)
      {
        lock.lock();
        if (logging)
        {
          if ((now() - start_time) >= max_log_duration)
            pstart();
        }
        lock.unlock();
      }

      bool is_logging(void)
      {
        return logging;
      }
    };
  };
}