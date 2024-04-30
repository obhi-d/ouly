#pragma once

#include <acl/utils/common.hpp>

namespace acl::detail
{
enum class mmeory_stat_type
{
  e_none,
  e_compute,
  e_compute_atomic
};

}
namespace acl::opt
{
template <typename T>
struct base_stats
{
  using base_stat_type = T;
};

struct compute_stats
{
  static constexpr detail::mmeory_stat_type compute_stats_v = detail::mmeory_stat_type::e_compute;
};

struct compute_atomic_stats
{
  static constexpr detail::mmeory_stat_type compute_stats_v = detail::mmeory_stat_type::e_compute_atomic;
};

} // namespace acl::opt

namespace acl::detail
{
template <typename T>
concept HasComputeStats = std::same_as<decltype(T::compute_stats_v), mmeory_stat_type>;

template <typename T>
concept HasBaseStats = requires { typename T::base_stat_type; };

struct default_base_stats
{
  std::string print() const
  {
    return {};
  }
};

template <typename Opt>
struct base_stat_type_deduction
{
  using type = default_base_stats;
};

template <HasBaseStats Opt>
struct base_stat_type_deduction<Opt>
{
  using type = typename Opt::base_stat_type;
};

template <typename T>
using base_stat_type = typename base_stat_type_deduction<T>::type;

template <typename Opt>
struct stats_impl
{
  static constexpr detail::mmeory_stat_type option = detail::mmeory_stat_type::e_none;
};

template <HasComputeStats T>
struct stats_impl<T>
{
  static constexpr detail::mmeory_stat_type option = T::compute_stats_v;
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

template <typename Tag, typename Base, detail::mmeory_stat_type = detail::mmeory_stat_type::e_none>
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

template <typename tag_arg, typename Base>
struct statistics_impl<tag_arg, Base, detail::mmeory_stat_type::e_compute_atomic> : public Base
{
  statistics_impl() noexcept {}
  statistics_impl(const statistics_impl&) noexcept {}
  statistics_impl(statistics_impl&&) noexcept {}
  statistics_impl& operator=(const statistics_impl&) noexcept
  {
    return *this;
  }
  statistics_impl& operator=(statistics_impl&&) noexcept
  {
    return *this;
  }

  std::atomic_uint32_t arenas_allocated = 0;

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
    if (stats_printed)
      return;
    ACL_PRINT_DEBUG(print());
    stats_printed = true;
  }

  std::string print()
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
    auto bstats = this->Base::print();
    if (!bstats.empty())
    {
      ss << this->Base::print();
      ss << line;
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

template <typename tag_arg, typename Base>
struct statistics_impl<tag_arg, Base, detail::mmeory_stat_type::e_compute>
{

  uint32_t arenas_allocated   = 0;
  uint64_t peak_allocation    = 0;
  uint64_t allocation         = 0;
  uint64_t deallocation_count = 0;
  uint64_t allocation_count   = 0;
  timer_t  allocation_timing;
  timer_t  deallocation_timing;
  bool     stats_printed = false;

  inline ~statistics_impl() noexcept
  {
    print_to_debug();
  }

  void print_to_debug() noexcept
  {
    if (stats_printed)
      return;
    ACL_PRINT_DEBUG(print());
    stats_printed = true;
  }

  std::string print()
  {
    std::string line(79, '=');
    line += "\n";
    std::stringstream ss;
    ss << line;
    ss << "Stats for: " << acl::type_name<tag_arg>() << "\n";
    ss << line;
    ss << "Arenas allocated: " << arenas_allocated << "\n"
       << "Peak allocation: " << peak_allocation << "\n"
       << "Final allocation: " << allocation << "\n"
       << "Total allocation call: " << allocation_count << "\n"
       << "Total deallocation call: " << deallocation_count << "\n"
       << "Total allocation time: " << allocation_timing.elapsed_time_count() << " us \n"
       << "Total deallocation time: " << deallocation_timing.elapsed_time_count() << " us\n";
    if (allocation_count > 0)
      ss << "Avg allocation time: " << allocation_timing.elapsed_time_count() / allocation_count << " us\n";
    if (deallocation_count > 0)
      ss << "Avg deallocation time: " << deallocation_timing.elapsed_time_count() / deallocation_count << " us\n";
    ss << line;
    auto bstats = this->Base::print();
    if (!bstats.empty())
    {
      ss << this->Base::print();
      ss << line;
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
    peak_allocation = std::max<std::size_t>(allocation, peak_allocation);
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
    return arenas_allocated;
  }
};

template <typename tag, typename Options = acl::options<>>
struct statistics : public statistics_impl<tag, detail::base_stat_type<Options>, detail::stats_impl<Options>::option>
{

  using super = statistics_impl<tag, detail::base_stat_type<Options>, detail::stats_impl<Options>::option>;
  using super::get_arenas_allocated;
  using super::print;
  using super::report_allocate;
  using super::report_deallocate;
  using super::report_new_arena;
};

} // namespace acl::detail
