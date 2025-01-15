#pragma once
#include <cstdint>
#include <cstring>
#include <functional>
#include <mutex>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace acl::detail
{
struct dummy_debug_tracer
{
  struct backtrace
  {
    template <typename Stream>
    friend auto operator<<(Stream& s, const backtrace& me) -> Stream&
    {
      s << "Unknown";
      return s;
    }
    friend auto operator==(const backtrace& /*unused*/, const backtrace& /*unused*/) -> bool
    {
      return true;
    }
    friend auto operator!=(const backtrace& /*unused*/, const backtrace& /*unused*/) -> bool
    {
      return false;
    }
  };

  struct hasher
  {
    auto operator()(const backtrace& /*unused*/) const -> std::size_t
    {
      return 0;
    }
  };

  struct trace_output
  {
    void operator()(std::string_view /*unused*/) const {}
  };
};

template <typename TagArg, typename DebugTracer = dummy_debug_tracer, bool Enabled = false>
struct memory_tracker
{
  static auto when_allocate(void* i_data, [[maybe_unused]] std::size_t i_size) -> void*
  {
    return i_data;
  }
  static auto when_deallocate(void* i_data, [[maybe_unused]] std::size_t i_size) -> void*
  {
    return i_data;
  }
};

template <typename TagArg, typename DebugTracer>
struct memory_tracker_impl
{
  using out_stream = typename DebugTracer::trace_output;
  using backtrace  = typename DebugTracer::backtrace;
  using hasher     = typename DebugTracer::hasher;

  memory_tracker_impl() noexcept                                     = default;
  memory_tracker_impl(const memory_tracker_impl&)                    = delete;
  memory_tracker_impl(memory_tracker_impl&&)                         = delete;
  auto operator=(const memory_tracker_impl&) -> memory_tracker_impl& = delete;
  auto operator=(memory_tracker_impl&&) -> memory_tracker_impl&      = delete;

  ~memory_tracker_impl()
  {
    if (!pointer_map_.empty())
    {
      out_("\nPossible leaks\n");
      std::ostringstream stream;
      for (auto& e : pointer_map_)
      {
        stream << "\n[" << e.first << "] for " << e.second.first << " bytes from\n" << e.second.second.get();
      }
      out_(stream.str());
    }
  }

  template <typename Arg>
  void set_out_stream(Arg&& i_out)
  {
    out_ = std::forward<Arg>(i_out);
  }

  void when_allocate(void* i_data, std::size_t i_size)
  {
    std::unique_lock<std::mutex> ul(lock_);
    if (ignore_first_ == nullptr)
    {
      ignore_first_ = i_data;
      return;
    }
    pointer_map_.emplace(i_data, std::make_pair(i_size, std::cref(*regions_.emplace(backtrace()).first)));
    memory_counter_ += i_size;
  }

  void when_deallocate(void* i_data, std::size_t i_size)
  {
    if (i_data == nullptr)
    {
      return;
    }
    std::unique_lock<std::mutex> ul(lock_);
    std::stringstream            ss;
    if (i_data == ignore_first_)
    {
      ignore_first_ = reinterpret_cast<void*>(0x1); // NOLINT
      return;
    }
    auto it = pointer_map_.find(i_data);
    if (it == pointer_map_.end())
    {
      ss << "\nInvalid memory free -> \n";
      ss << backtrace();
    }
    pointer_map_.erase(it);
    memory_counter_ -= i_size;
    out_(ss.str());
  }

  std::unordered_map<void*, std::pair<std::size_t, std::reference_wrapper<const backtrace>>> pointer_map_;
  std::unordered_set<backtrace, hasher>                                                      regions_;

  static auto get_instance() -> memory_tracker_impl<TagArg, DebugTracer>&
  {
    static memory_tracker_impl<TagArg, DebugTracer> instance;
    return instance;
  }

  out_stream  out_;
  void*       ignore_first_   = nullptr;
  std::size_t memory_counter_ = 0;
  std::mutex  lock_;
};

template <typename TagArg, typename DebugTracer>
struct memory_tracker<TagArg, DebugTracer, true>
{
  template <typename Arg>
  static void set_out_stream(Arg&& i_out)
  {
    memory_tracker_impl<TagArg, DebugTracer>::get_instance().set_out_stream(std::forward<Arg>(i_out));
  }

  static auto when_allocate(void* i_data, std::size_t i_size) -> void*
  {
    memory_tracker_impl<TagArg, DebugTracer>::get_instance().when_allocate(i_data, i_size);
    return i_data;
  }
  static auto when_deallocate(void* i_data, std::size_t i_size) -> void*
  {
    if (i_data != nullptr)
    {
      memory_tracker_impl<TagArg, DebugTracer>::get_instance().when_deallocate(i_data, i_size);
    }
    return i_data;
  }
};
} // namespace acl::detail
