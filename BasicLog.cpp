#include "BasicLog.hpp"
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
  // look for contiguous chunks of memory
  std::vector<Log::DataChunk> Log::DataChunk::condense(std::vector<DataChunk> const &chunk)
  {
    std::vector<DataChunk> result;
    if (!chunk.empty())
    {
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
    }
    return result;
  }

  Log::Entry::Entry() {}

  // simple container
  Log::Entry::Entry(const std::string_view Name, const std::string_view Description, std::vector<Entry> child_entries)
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

  std::string Log::Entry::header() const
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
      for (auto &h2 : children)
        h.append(",\n").append(h2.header());
    }
    return h;
  }

  void Log::Entry::sort_entries(std::vector<Entry> &entries)
  {
    // sort the children
    std::sort(entries.begin(), entries.end(),
              [](const auto &AA, const auto &BB) -> bool
              {
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

  void Log::Entry::sort_children(void)
  {
    sort_entries(children);
  }

  std::runtime_error Log::Entry::error(const std::string_view msg) const
  {
    return std::runtime_error(std::string("\"").append(name).append("\", \"").append(description).append("\": ").append(msg));
  }

  void Log::Entry::check_name(void) const
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

  void Log::Entry::check_child_names(void) const
  {
    // check that the child names are unique
    std::unordered_set<std::string_view> unique_child_names;
    for (size_t ind = 0; ind < children.size(); ind++)
    {
      if (!unique_child_names.insert(children[ind].name).second)
        throw error("already contains a child named \"" + children[ind].name + "\"");
    }
  }

  Log::Log(){};

  Log::Log(const std::string_view Name, const std::string_view Description, CompressionMethod Compression, std::vector<Entry> child_entries)
      : MainEntry(Name, Description, child_entries), selected_recorder(CompressionMethodFunction[Compression]), current_recorder(&Log::record_NULL)
  {
    MainEntry.parent_index = 0;
    std::vector<Entry> AllEntries;
    std::function<void(Entry)> flatten;
    flatten = [&](Entry L)
    {
      //        auto children = L.children;
      for (auto &c : L.children)
      {
        c.name = L.name + "." + c.name;
      }
      // L.children.clear();
      AllEntries.push_back(L);
      if (!L.is_contiguous)
      {
        for (auto &c : L.children)
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
    for (auto &c : AllEntries)
    {
      AllChunks.insert(AllChunks.end(), c.data.begin(), c.data.end());
      if (first)
        first = false;
      else
        header.append(",\n");
      header.append(c.header());
    }
    data = DataChunk::condense(AllChunks);
    size_t total_size = std::accumulate(data.begin(), data.end(), 0, [](size_t sum, const DataChunk &E)
                                        { return sum + E.count; });

    previous_row = std::vector<char>(total_size, 0);

    header.append("\n],\n");
    header.append("\"row_size\":").append(std::to_string(total_size));
    header.append("\n}");

    // display stuff (for now)
    std::cout << header << '\n';
    for (auto &c : data)
    {
      std::cout << (void *)c.ptr << "     " << c.count << "     " << (void *)(c.ptr + c.count) << '\n';
    }
  }

  std::string_view Log::name() const
  {
    return MainEntry.name;
  }

  void Log::start(std::filesystem::path directory)
  {
    if (directory.empty())
      throw MainEntry.error("cannot begin logging to an empty directory");
    // create the new file
    auto file_path = directory / (MainEntry.name + ".cap");
    log_file.open(file_path, std::ios_base::binary | std::ios_base::trunc);
    if (!log_file)
      throw MainEntry.error(std::string("failed to create log file: ").append(file_path));

    log_file << header << '\0';           // add the header followed by 0
    current_recorder = selected_recorder; // update the recorder function
    // init recorder states
    std::fill(previous_row.begin(), previous_row.end(), 0);
  }

  void Log::stop(void)
  {
    current_recorder = &Log::record_NULL;
    log_file.flush();
    log_file.close();
  }

  void Log::record(void)
  {
    (this->*current_recorder)();
  }

  void Log::record_NULL(void) {}

  void Log::record_RAW(void)
  {
    for (auto &e : data)
    {
      log_file.write(e.ptr, e.count);
    }
  }

  void Log::record_DIFF1(void)
  {
    const size_t num_bytes = previous_row.size();
    const size_t num_bits = num_bytes / 8 + (num_bytes % 8 > 0); // number of bytes we need to get at least one bit per byte
    std::vector<char> prefix(num_bits, 0);
    std::vector<char> delta(num_bytes, 0);
    std::vector<char> row(num_bytes, 0);

    // copy all the data to be logged into a buffer
    size_t pos = 0;
    for (auto &e : data)
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
      prefix[h_ind] |= 1U << b_ind;     // set the bit
      bytes_to_log.push_back(delta[i]); // insert the data
    }

    // finally write the prefix and data
    log_file.write(prefix.data(), prefix.size());
    log_file.write(bytes_to_log.data(), bytes_to_log.size());
  }

  std::string Log::unix_time_formatted(void)
  {
    time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()); // system clock is 'Unix time' in c++20
    struct tm tm;
    localtime_r(&now, &tm);
    std::stringstream ss;
    ss << std::put_time(&tm, "%Y%m%d_%H%M%S%z");
    return ss.str();
  }

  std::chrono::system_clock::time_point Log::now(void)
  {
    return std::chrono::system_clock::now();
  }

  auto Log::unix_time_now_ns(void)
  {
    return now().time_since_epoch().count();
  }

  void Log::Manager::pcheck_child_names(void) const
  {
    // check that the child names are unique
    std::unordered_set<std::string_view> unique_names;
    std::for_each(logs.begin(), logs.end(), [&](const Log *L)
                  {
          if (!unique_names.insert(L->name()).second)
            throw std::runtime_error(std::string("Log::Manager instance already contains a log named \"").append(L->name()).append("\"")); });
  }

  std::filesystem::path Log::Manager::pstart(void)
  {
    std::filesystem::path dir = root_directory / unix_time_formatted();
    std::filesystem::create_directories(dir);
    std::for_each(logs.begin(), logs.end(), [&](Log *L)
                  { L->start(dir); });
    start_time = Log::now();
    logging = true;
    return dir;
  }

  void Log::Manager::pstop(void)
  {
    std::for_each(logs.begin(), logs.end(), [](Log *L)
                  { L->stop(); });
    logging = false;
  }

  Log::Manager::Manager() {}
  Log::Manager::Manager(std::filesystem::path root)
  {
    set_root_directory(root);
  }

  void Log::Manager::push_back(Log *L)
  {
    lock.lock();
    bool was_logging = logging;
    if (was_logging)
      pstop();
    logs.push_back(L);
    pcheck_child_names();
    if (was_logging)
      pstart();
    lock.unlock();
  }

  void Log::Manager::set_root_directory(const std::filesystem::path path)
  {
    lock.lock();
    bool was_logging = logging;
    if (was_logging)
      pstop();
    std::filesystem::create_directories(path); // may fail
    root_directory = path;
    if (was_logging)
      pstart();
    lock.unlock();
  }

  void Log::Manager::set_max_log_duration(const std::chrono::system_clock::duration &time)
  {
    lock.lock();
    max_log_duration = time;
    lock.unlock();
  }

  std::filesystem::path Log::Manager::start(void)
  {
    lock.lock();
    auto dir = pstart();
    lock.unlock();
    return dir;
  }

  void Log::Manager::stop(void)
  {
    lock.lock();
    std::for_each(logs.begin(), logs.end(), [](Log *L)
                  { L->stop(); });
    logging = false;
    lock.unlock();
  }

  // NOTE: not going to expose a Manager level "record" method each log
  // should be recorded at an appropriate rate based on the application

  void Log::Manager::restart_if_needed(void)
  {
    lock.lock();
    if (logging)
    {
      if ((now() - start_time) >= max_log_duration)
        pstart();
    }
    lock.unlock();
  }

  bool Log::Manager::is_logging(void)
  {
    return logging;
  }
}