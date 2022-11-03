#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <new>
#include <random>
#include <sstream>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <variant>
#include <vector>

#include <acl/type_traits.hpp>

#ifdef _MSC_VER
#include <malloc.h>
#endif

#ifndef ACL_CUSTOM_MALLOC_NS
#define ACL_NO_CUSTOM_MALLOC_NS
#define ACL_CUSTOM_MALLOC_NS
#endif

namespace acl
{
inline void* malloc(std::size_t s)
{
  return ACL_CUSTOM_MALLOC_NS::malloc(s);
}
inline void free(void* f)
{
  return ACL_CUSTOM_MALLOC_NS::free(f);
}
inline void* aligned_alloc(std::size_t alignment, std::size_t size)
{
#ifdef ACL_NO_CUSTOM_MALLOC_NS
#ifdef _MSC_VER
  return _aligned_malloc(size, alignment);
#else
  return ::aligned_alloc(alignment, size);
#endif

#else
  return ACL_CUSTOM_MALLOC_NS::aligned_alloc(alignment, size);
#endif
}
inline void aligned_free(void* ptr)
{
#ifdef ACL_NO_CUSTOM_MALLOC_NS
#ifdef _MSC_VER
  return _aligned_free(ptr);
#else
  return ::free(ptr);
#endif
#else
  return ACL_CUSTOM_MALLOC_NS::aligned_free(ptr);
#endif
}
} // namespace acl

#if defined(_MSC_VER)
#include <intrin.h>
#define ACL_EXPORT              __declspec(dllexport)
#define ACL_IMPORT              __declspec(dllimport)
#define ACL_EMPTY_BASES         __declspec(empty_bases)
#define ACL_POPCOUNT(v)         __popcnt(v)
#define ACL_PREFETCH_ONETIME(x) _mm_prefetch((const char*)(x), _MM_HINT_T0)
#define ACL_LIKELY(x)           (x)
#define ACL_UNLIKELY(x)         (x)
#else
#define ACL_EXPORT __attribute__((visibility("default")))
#define ACL_IMPORT __attribute__((visibility("default")))
#define ACL_EMPTY_BASES
#define ACL_POPCOUNT(v)         __builtin_popcount(v)
#define ACL_PREFETCH_ONETIME(x) __builtin_prefetch((x))
#define ACL_LIKELY(x)           __builtin_expect((x), 1)
#define ACL_UNLIKELY(x)         __builtin_expect((x), 0)
#endif

#ifdef ACL_DLL_IMPL
#ifdef ACL_EXPORT_SYMBOLS
#define ACL_API ACL_EXPORT
#else
#define ACL_API ACL_IMPORT
#endif
#else
#define ACL_API
#endif

#if _DEBUG
#define ACL_VALIDITY_CHECKS
#endif

#ifndef ACL_PRINT_DEBUG
#define ACL_PRINT_DEBUG(info) acl::detail::print_debug_info(info)
#endif

#define ACL_EXTERN extern "C"

namespace acl
{
constexpr std::uint32_t safety_offset = alignof(void*);

using uhandle = std::uint32_t;
using ihandle = std::uint32_t;

namespace detail
{

template <typename size_type>
constexpr size_type     k_null_sz  = std::numeric_limits<size_type>::max();
constexpr std::uint32_t k_null_32  = std::numeric_limits<std::uint32_t>::max();
constexpr std::int32_t  k_null_i32 = std::numeric_limits<std::int32_t>::min();
constexpr std::uint64_t k_null_64  = std::numeric_limits<std::uint64_t>::max();
constexpr uhandle       k_null_uh  = std::numeric_limits<uhandle>::max();

enum ordering_by : std::uint32_t
{
  e_size,
  e_offset,
  k_count
};

inline void print_debug_info(std::string const& s)
{
  std::cout << s;
}

struct stats_base
{
  static std::string print()
  {
    return std::string();
  }
};

template <typename tag, bool k_compute_stats = false, typename base_t = stats_base, bool k_print = true>
struct statistics : public base_t
{
  using stats_base_t = stats_base;
  template <typename... Args>
  statistics(Args&&... i_args) : base_t(std::forward<Args>(i_args)...)
  {}

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

struct timer_t
{
  struct scoped
  {
    scoped(timer_t& t) : timer(&t)
    {
      start = std::chrono::high_resolution_clock::now();
    }
    scoped(scoped const&) = delete;
    scoped(scoped&& other) noexcept : timer(other.timer), start(other.start)
    {
      other.timer = nullptr;
    }
    scoped& operator=(scoped const&) = delete;
    scoped& operator=(scoped&& other)
    {
      timer       = other.timer;
      start       = other.start;
      other.timer = nullptr;
      return *this;
    }

    ~scoped()
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
    return elapsed_time.load();
  }

  std::atomic_uint64_t elapsed_time = 0;
};

template <typename tag_arg, typename base_arg, bool k_print>
struct statistics<tag_arg, true, base_arg, k_print> : public base_arg
{
  std::atomic_uint32_t arenas_allocated   = 0;
  std::atomic_uint64_t peak_allocation    = 0;
  std::atomic_uint64_t allocation         = 0;
  std::atomic_uint64_t deallocation_count = 0;
  std::atomic_uint64_t allocation_count   = 0;
  std::atomic_uint64_t allocator_data     = 0;
  timer_t              allocation_timing;
  timer_t              deallocation_timing;
  bool                 stats_printed = false;

  template <typename... Args>
  statistics(Args&&... i_args) : base_arg(std::forward<Args>(i_args)...)
  {}
  ~statistics()
  {
    print();
  }

  void print()
  {
    if (k_print && !stats_printed)
    {
      std::string line(79, '=');
      std::string line2(79, '-');
      line2 += "\n";
      line += "\n";
      std::stringstream ss;
      ss << "Stats for: " << acl::type_name<tag_arg>() << "\n";
      ss << line;
      std::string base_stats = base_arg::print();
      if (!base_stats.empty())
        ss << "Allocator specific stats\n" << line2 << base_arg::print() << "\n" << line2;

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
      ACL_PRINT_DEBUG(ss.str());
      stats_printed = true;
    }
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

} // namespace detail

} // namespace acl
