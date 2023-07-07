#pragma once
#include <cstdint>
#include <cstring>
#include <functional>
#include <mutex>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace acl
{
namespace detail
{
struct dummy_debug_tracer
{
  struct backtrace
  {
    template <typename stream>
    friend stream& operator<<(stream& s, const backtrace& me)
    {
      s << "Unknown";
      return s;
    }
    friend bool operator==(const backtrace& f, const backtrace& s)
    {
      return true;
    }
    friend bool operator!=(const backtrace& f, const backtrace& s)
    {
      return false;
    }
  };

  struct hasher
  {
    inline std::size_t operator()(const backtrace& t) const
    {
      return 0;
    }
  };

  struct trace_output
  {
    inline void operator()(std::string_view s) const {}
  };
};

template <typename tag_arg, typename debug_tracer = dummy_debug_tracer, bool enabled = false>
struct memory_tracker
{
  inline static void* when_allocate(void* i_data, std::size_t i_size)
  {
    return i_data;
  }
  inline static void* when_deallocate(void* i_data, std::size_t i_size)
  {
    return i_data;
  }
};

template <typename tag_arg, typename debug_tracer>
struct ACL_API memory_tracker_impl
{
  using out_stream = typename debug_tracer::trace_output;
  using backtrace  = typename debug_tracer::backtrace;
  using hasher     = typename debug_tracer::hasher;
  ~memory_tracker_impl()
  {
    if (pointer_map.size() > 0)
    {
      out("\nPossible leaks\n");
      std::ostringstream stream;
      for (auto& e : pointer_map)
      {
        stream << "\n[" << e.first << "] for " << e.second.first << " bytes from\n" << e.second.second.get();
      }
      out(stream.str());
    }
  }

  template <typename Arg>
  void set_out_stream(Arg&& i_out)
  {
    out = std::forward<Arg>(i_out);
  }

  void when_allocate(void* i_data, std::size_t i_size)
  {
    std::unique_lock<std::mutex> ul(lock);
    if (!ignore_first)
    {
      ignore_first = i_data;
      return;
    }
    pointer_map.emplace(i_data, std::make_pair(i_size, std::cref(*regions.emplace(backtrace()).first)));
    memory_counter += i_size;
  }

  void when_deallocate(void* i_data, std::size_t i_size)
  {
    if (!i_data)
      return;
    std::unique_lock<std::mutex> ul(lock);
    std::stringstream            ss;
    if (i_data == ignore_first)
    {
      ignore_first = (void*)0x1;
      return;
    }
    auto it = pointer_map.find(i_data);
    if (it == pointer_map.end())
    {
      ss << "\nInvalid memory free -> \n";
      ss << backtrace();
    }
    pointer_map.erase(it);
    memory_counter -= i_size;
    out(ss.str());
  }

  std::unordered_map<void*, std::pair<std::size_t, std::reference_wrapper<const backtrace>>> pointer_map;
  std::unordered_set<backtrace, hasher>                                                      regions;

  static memory_tracker_impl<tag_arg, debug_tracer>& get_instance()
  {
    static memory_tracker_impl<tag_arg, debug_tracer> instance;
    return instance;
  }

  out_stream  out;
  void*       ignore_first   = nullptr;
  std::size_t memory_counter = 0;
  std::mutex  lock;
};

template <typename tag_arg, typename debug_tracer>
struct ACL_API memory_tracker<tag_arg, debug_tracer, true>
{
  template <typename Arg>
  static void set_out_stream(Arg&& i_out)
  {
    memory_tracker_impl<tag_arg, debug_tracer>::get_instance().set_out_stream(std::forward<Arg>(i_out));
  }

  static void* when_allocate(void* i_data, std::size_t i_size)
  {
    memory_tracker_impl<tag_arg, debug_tracer>::get_instance().when_allocate(i_data, i_size);
    return i_data;
  }
  static void* when_deallocate(void* i_data, std::size_t i_size)
  {
    if (i_data)
    {
      memory_tracker_impl<tag_arg, debug_tracer>::get_instance().when_deallocate(i_data, i_size);
    }
    return i_data;
  }
};
} // namespace detail
} // namespace acl