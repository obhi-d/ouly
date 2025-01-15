#pragma once

#include <acl/allocators/config.hpp>
#include <acl/reflection/type_name.hpp>
#include <acl/utility/common.hpp>
#include <atomic>
#include <chrono>
#include <cstdint>

namespace acl::detail
{
template <typename T>
concept HasComputeStats = T::compute_stats_v == acl::cfg::memory_stat_type::e_compute ||
                          T::compute_stats_v == acl::cfg::memory_stat_type::e_compute_atomic;

template <typename T>
concept HasBaseStats = requires { typename T::base_stat_type; };

struct default_base_stats
{
  [[nodiscard]] static auto print() -> std::string
  {
    return {};
  }
};

template <typename Config>
struct base_stat_type_deduction
{
  using type = default_base_stats;
};

template <HasBaseStats Config>
struct base_stat_type_deduction<Config>
{
  using type = typename Config::base_stat_type;
};

template <typename T>
using base_stat_type = typename base_stat_type_deduction<T>::type;

template <typename Config>
struct stats_impl
{
  static constexpr acl::cfg::memory_stat_type option = acl::cfg::memory_stat_type::e_none;
};

template <HasComputeStats T>
struct stats_impl<T>
{
  static constexpr acl::cfg::memory_stat_type option = T::compute_stats_v;
};

struct timer_t
{
  struct scoped
  {
    scoped(timer_t& t) noexcept : timer_(&t)
    {
      start_ = std::chrono::high_resolution_clock::now();
    }
    scoped(scoped const&) = delete;
    scoped(scoped&& other) noexcept : timer_(other.timer_), start_(other.start_)
    {
      other.timer_ = nullptr;
    }
    auto operator=(scoped const&) -> scoped& = delete;
    auto operator=(scoped&& other) noexcept -> scoped&
    {
      timer_       = other.timer_;
      start_       = other.start_;
      other.timer_ = nullptr;
      return *this;
    }

    ~scoped()
    {
      if (timer_ != nullptr)
      {
        auto end = std::chrono::high_resolution_clock::now();
        timer_->elapsed_time_ += std::chrono::duration_cast<std::chrono::microseconds>(end - start_).count();
      }
    }

    timer_t*                                       timer_ = nullptr;
    std::chrono::high_resolution_clock::time_point start_;
  };

  [[nodiscard]] auto elapsed_time_count() const -> std::uint64_t
  {
    return elapsed_time_;
  }

  uint64_t elapsed_time_ = 0;
};

template <typename Tag, typename Base, acl::cfg::memory_stat_type = acl::cfg::memory_stat_type::e_none>
struct statistics_impl
{

  void        print() {}
  static auto report_new_arena(std::uint32_t count = 1) -> std::false_type
  {
    return std::false_type{};
  }
  static auto report_allocate(std::size_t size) -> std::false_type
  {
    return std::false_type{};
  }
  static auto report_deallocate(std::size_t size) -> std::false_type
  {
    return std::false_type{};
  }

  [[nodiscard]] auto get_arenas_allocated() const -> std::uint32_t
  {
    return 0;
  }
};

template <typename TagArg, typename Base>
struct statistics_impl<TagArg, Base, acl::cfg::memory_stat_type::e_compute_atomic> : public Base
{
  statistics_impl() noexcept = default;
  statistics_impl(const statistics_impl& /*unused*/) noexcept {}
  statistics_impl(statistics_impl&& /*unused*/) noexcept {}
  auto operator=(const statistics_impl& /*unused*/) noexcept -> statistics_impl&
  {
    return *this;
  }
  auto operator=(statistics_impl&& /*unused*/) noexcept -> statistics_impl&
  {
    return *this;
  }

  std::atomic_uint32_t arenas_allocated_ = 0;

  std::atomic_uint64_t peak_allocation_    = 0;
  std::atomic_uint64_t allocation_         = 0;
  std::atomic_uint64_t deallocation_count_ = 0;
  std::atomic_uint64_t allocation_count_   = 0;
  timer_t              allocation_timing_;
  timer_t              deallocation_timing_;
  bool                 stats_printed_ = false;

  ~statistics_impl() noexcept
  {
    print_to_debug();
  }

  void print_to_debug() noexcept
  {
    if (stats_printed_)
    {
      return;
    }
    ACL_PRINT_DEBUG(print());
    stats_printed_ = true;
  }

  auto print() -> std::string
  {
    constexpr uint32_t max_size = 79;
    std::string        line(max_size, '=');
    line += "\n";
    std::stringstream ss;
    ss << line;
    ss << "Stats for: " << (std::string_view)acl::type_name<TagArg>() << "\n";
    ss << line;
    ss << "Arenas allocated: " << arenas_allocated_.load() << "\n"
       << "Peak allocation: " << peak_allocation_.load() << "\n"
       << "Final allocation: " << allocation_.load() << "\n"
       << "Total allocation call: " << allocation_count_.load() << "\n"
       << "Total deallocation call: " << deallocation_count_.load() << "\n"
       << "Total allocation time: " << allocation_timing_.elapsed_time_count() << " us \n"
       << "Total deallocation time: " << deallocation_timing_.elapsed_time_count() << " us\n";
    if (allocation_count_ > 0)
    {
      ss << "Avg allocation time: " << allocation_timing_.elapsed_time_count() / allocation_count_.load() << " us\n";
    }
    if (deallocation_count_ > 0)
    {
      ss << "Avg deallocation time: " << deallocation_timing_.elapsed_time_count() / deallocation_count_.load()
         << " us\n";
    }
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
    arenas_allocated_ += count;
  }

  [[nodiscard]] auto report_allocate(std::size_t size) -> timer_t::scoped
  {
    allocation_count_++;
    allocation_ += size;
    peak_allocation_ = std::max<std::size_t>(allocation_.load(), peak_allocation_.load());
    return timer_t::scoped(allocation_timing_);
  }
  [[nodiscard]] auto report_deallocate(std::size_t size) -> timer_t::scoped
  {
    deallocation_count_++;
    allocation_ -= size;
    return timer_t::scoped(deallocation_timing_);
  }

  [[nodiscard]] auto get_arenas_allocated() const -> std::uint32_t
  {
    return arenas_allocated_.load();
  }
};

template <typename TagArg, typename Base>
struct statistics_impl<TagArg, Base, acl::cfg::memory_stat_type::e_compute> : public Base
{

  statistics_impl() noexcept                                     = default;
  statistics_impl(const statistics_impl&)                        = delete;
  statistics_impl(statistics_impl&&)                             = delete;
  auto     operator=(const statistics_impl&) -> statistics_impl& = delete;
  auto     operator=(statistics_impl&&) -> statistics_impl&      = delete;
  uint32_t arenas_allocated_                                     = 0;

  uint64_t peak_allocation_    = 0;
  uint64_t allocation_         = 0;
  uint64_t deallocation_count_ = 0;
  uint64_t allocation_count_   = 0;
  timer_t  allocation_timing_;
  timer_t  deallocation_timing_;
  bool     stats_printed_ = false;

  ~statistics_impl() noexcept
  {
    print_to_debug();
  }

  void print_to_debug() noexcept
  {
    if (stats_printed_)
    {
      return;
    }
    ACL_PRINT_DEBUG(print());
    stats_printed_ = true;
  }

  auto print() -> std::string
  {
    constexpr uint32_t max_size = 79;
    std::string        line(max_size, '=');
    line += "\n";
    std::stringstream ss;
    ss << line;
    ss << "Stats for: " << (std::string_view)acl::type_name<TagArg>() << "\n";
    ss << line;
    ss << "Arenas allocated: " << arenas_allocated_ << "\n"
       << "Peak allocation: " << peak_allocation_ << "\n"
       << "Final allocation: " << allocation_ << "\n"
       << "Total allocation call: " << allocation_count_ << "\n"
       << "Total deallocation call: " << deallocation_count_ << "\n"
       << "Total allocation time: " << allocation_timing_.elapsed_time_count() << " us \n"
       << "Total deallocation time: " << deallocation_timing_.elapsed_time_count() << " us\n";
    if (allocation_count_ > 0)
    {
      ss << "Avg allocation time: " << allocation_timing_.elapsed_time_count() / allocation_count_ << " us\n";
    }
    if (deallocation_count_ > 0)
    {
      ss << "Avg deallocation time: " << deallocation_timing_.elapsed_time_count() / deallocation_count_ << " us\n";
    }
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
    arenas_allocated_ += count;
  }

  [[nodiscard]] auto report_allocate(std::size_t size) -> timer_t::scoped
  {
    allocation_count_++;
    allocation_ += size;
    peak_allocation_ = std::max<std::size_t>(allocation_, peak_allocation_);
    return timer_t::scoped(allocation_timing_);
  }
  [[nodiscard]] auto report_deallocate(std::size_t size) -> timer_t::scoped
  {
    deallocation_count_++;
    allocation_ -= size;
    return timer_t::scoped(deallocation_timing_);
  }

  [[nodiscard]] auto get_arenas_allocated() const -> std::uint32_t
  {
    return arenas_allocated_;
  }
};

template <typename Tag, typename Config = acl::config<>>
struct statistics
    : public statistics_impl<Tag, acl::detail::base_stat_type<Config>, acl::detail::stats_impl<Config>::option>
{

  using super = statistics_impl<Tag, acl::detail::base_stat_type<Config>, acl::detail::stats_impl<Config>::option>;
  using super::get_arenas_allocated;
  using super::print;
  using super::report_allocate;
  using super::report_deallocate;
  using super::report_new_arena;
};

} // namespace acl::detail
