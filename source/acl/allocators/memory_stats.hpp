#pragma once

#include <acl/utils/common.hpp>

namespace acl::opt
{
struct compute_stats
{
  static constexpr bool compute_stats_v = true;
};

struct print_stats
{
  static constexpr bool print_stats_v = true;
};

template <typename T>
struct base_stats
{
  using base_stats_t = T;
};

} // namespace acl::opt

namespace acl::detail
{
template <typename T>
concept HasComputeStats = T::compute_stats_v;

template <typename T>
concept HasPrintStats = T::print_stats_v;

template <typename T>
concept HasBaseStats = requires { typename T::base_stats_t; };

template <typename T>
struct base_stats
{
  using type = std::monostate;
};

template <HasBaseStats T>
struct base_stats<T>
{
  using type = typename T::base_stats_t;
};

template <typename T>
using base_stats_t = typename base_stats<T>::type;

struct stats_base
{
  static std::string print()
  {
    return std::string();
  }
};

struct timer_t
{
  struct scoped
  {
    inline scoped(timer_t& t) noexcept : timer(&t)
    {
      start = std::chrono::high_resolution_clock::now();
    }
    inline scoped(scoped const&) = delete;
    inline scoped(scoped&& other) noexcept : timer(other.timer), start(other.start)
    {
      other.timer = nullptr;
    }
    inline scoped& operator=(scoped const&) = delete;
    inline scoped& operator=(scoped&& other) noexcept
    {
      timer       = other.timer;
      start       = other.start;
      other.timer = nullptr;
      return *this;
    }

    inline ~scoped()
    {
      if (timer)
      {
        auto end = std::chrono::high_resolution_clock::now();
        timer->elapsed_time += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
      }
    }

    timer_t*                                       timer = nullptr;
    std::chrono::high_resolution_clock::time_point start;
  };

  std::uint64_t elapsed_time_count()
  {
    return elapsed_time;
  }

  uint64_t elapsed_time = 0;
};

template <typename Tag, typename Opt = acl::options<>, typename T = std::false_type>
struct statistics_impl
{
  void                   print() {}
  static std::false_type report_new_arena(std::uint32_t count = 1)
  {
    return std::false_type{};
  }
  static std::false_type report_allocate(std::size_t size)
  {
    return std::false_type{};
  }
  static std::false_type report_deallocate(std::size_t size)
  {
    return std::false_type{};
  }

  std::uint32_t get_arenas_allocated() const
  {
    return 0;
  }
};

template <typename tag_arg, typename Opt>
struct statistics_impl<tag_arg, Opt, std::true_type> : base_stats_t<Opt>
{
  std::atomic_uint32_t arenas_allocated   = 0;
  std::atomic_uint64_t peak_allocation    = 0;
  std::atomic_uint64_t allocation         = 0;
  std::atomic_uint64_t deallocation_count = 0;
  std::atomic_uint64_t allocation_count   = 0;
  timer_t              allocation_timing;
  timer_t              deallocation_timing;
  bool                 stats_printed = false;

  inline ~statistics_impl() noexcept
  {
    print_to_debug();
  }

  void print_to_debug() noexcept
  {
    if constexpr (HasPrintStats<Opt>)
    {
      if (stats_printed)
        return;
      ACL_PRINT_DEBUG(print());
      stats_printed = true;
    }
  }

  std::string print()
  requires(HasPrintStats<Opt>)
  {
    std::string line(79, '=');
    line += "\n";
    std::stringstream ss;
    ss << line;
    ss << "Stats for: " << acl::type_name<tag_arg>() << "\n";
    ss << line;
    ss << "Arenas allocated: " << arenas_allocated.load() << "\n"
       << "Peak allocation: " << peak_allocation.load() << "\n"
       << "Final allocation: " << allocation.load() << "\n"
       << "Total allocation call: " << allocation_count.load() << "\n"
       << "Total deallocation call: " << deallocation_count.load() << "\n"
       << "Total allocation time: " << allocation_timing.elapsed_time_count() << " us \n"
       << "Total deallocation time: " << deallocation_timing.elapsed_time_count() << " us\n";
    if (allocation_count > 0)
      ss << "Avg allocation time: " << allocation_timing.elapsed_time_count() / allocation_count.load() << " us\n";
    if (deallocation_count > 0)
      ss << "Avg deallocation time: " << deallocation_timing.elapsed_time_count() / deallocation_count.load()
         << " us\n";
    ss << line;
    if constexpr (HasBaseStats<Opt>)
    {
      auto stats = base_stats_t<Opt>::print();
      if (!stats.empty())
      {
        ss << "BaseStats for: " << acl::type_name<base_stats_t<Opt>>() << "\n";
        ss << line;
        ss << base_stats_t<Opt>::print();
        ss << line;
      }
    }
    return ss.str();
  }

  void report_new_arena(std::uint32_t count = 1)
  {
    arenas_allocated += count;
  }

  [[nodiscard]] timer_t::scoped report_allocate(std::size_t size)
  {
    allocation_count++;
    allocation += size;
    peak_allocation = std::max<std::size_t>(allocation.load(), peak_allocation.load());
    return timer_t::scoped(allocation_timing);
  }
  [[nodiscard]] timer_t::scoped report_deallocate(std::size_t size)
  {
    deallocation_count++;
    allocation -= size;
    return timer_t::scoped(deallocation_timing);
  }

  std::uint32_t get_arenas_allocated() const
  {
    return arenas_allocated.load();
  }
};
template <typename tag, typename Options = acl::options<>>
struct statistics : public statistics_impl<tag, Options, std::bool_constant<HasComputeStats<Options>>>
{
  using super = statistics_impl<tag, Options, std::bool_constant<HasComputeStats<Options>>>;
  using super::get_arenas_allocated;
  using super::print;
  using super::report_allocate;
  using super::report_deallocate;
  using super::report_new_arena;
};

} // namespace acl::detail