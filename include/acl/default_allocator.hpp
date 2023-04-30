#pragma once
#include "detail/common.hpp"
#include "detail/memory_tracker.hpp"
#include "std_allocator_wrapper.hpp"
#include "type_traits.hpp"

namespace acl
{
struct default_allocator_tag
{};

template <>
struct allocator_traits<default_allocator_tag>
{
  using is_always_equal                        = std::true_type;
  using propagate_on_container_move_assignment = std::false_type;
  using propagate_on_container_copy_assignment = std::false_type;
  using propagate_on_container_swap            = std::false_type;
};


namespace detail
{
template <>
struct is_static<default_allocator_tag>
{
  constexpr inline static bool value = true;
};
} // namespace detail

namespace detail
{
template <bool k_compute = false>
struct default_alloc_statistics
{
  inline static int report_allocate(std::size_t)
  {
    return 0;
  }
  inline static int report_deallocate(std::size_t)
  {
    return 0;
  }
  inline static std::string print()
  {
    return std::string();
  }
};

#ifdef ACL_REC_STATS

ACL_EXTERN ACL_API detail::statistics<default_allocator_tag, true> default_allocator_statistics_instance;

template <>
struct default_alloc_statistics<true>
{
  inline static timer_t::scoped report_allocate(std::size_t i_sz)
  {
    return get_instance().report_allocate(i_sz);
  }
  inline static timer_t::scoped report_deallocate(std::size_t i_sz)
  {
    return get_instance().report_deallocate(i_sz);
  }
  inline static detail::statistics<default_allocator_tag, true>& get_instance()
  {
    return default_allocator_statistics_instance;
  }
  inline static std::string print()
  {
    return std::string();
  }

  inline bool operator==(default_alloc_statistics<true> const&) const
  {
    return true;
  }

  inline bool operator!=(default_alloc_statistics<true> const&) const
  {
    return false;
  }
};

#endif
} // namespace detail

inline void print_final_stats()
{
#ifdef ACL_REC_STATS
  detail::default_allocator_statistics_instance.print();
#endif
}

template <typename size_arg = std::size_t,
          bool k_compute_stats = false, bool k_track_memory = false, typename debug_tracer = std::monostate>
struct ACL_EMPTY_BASES default_allocator : detail::default_alloc_statistics<k_compute_stats>,
                                           detail::memory_tracker<default_allocator_tag, debug_tracer, k_track_memory>
{
  using tag        = default_allocator_tag;
  using address    = void*;
  using size_type  = size_arg;
  using statistics = detail::default_alloc_statistics<k_compute_stats>;
  using tracker    = detail::memory_tracker<default_allocator_tag, debug_tracer, k_track_memory>;

  default_allocator() {}
  default_allocator(default_allocator const&)  {}
  default_allocator& operator=(default_allocator const&)  {    return *this;  }

  inline static address allocate(size_type i_sz, size_type i_alignment = 0)
  {
    auto measure = statistics::report_allocate(i_sz);
    return tracker::when_allocate(i_alignment > alignof(std::max_align_t) ? acl::aligned_alloc(i_alignment, i_sz) : acl::malloc(i_sz), i_sz);
  }

  
  inline static address zero_allocate(size_type i_sz, size_type i_alignment = 0)
  {
    auto measure = statistics::report_allocate(i_sz);
    return tracker::when_allocate(i_alignment > alignof(std::max_align_t) ? acl::aligned_zalloc(i_alignment, i_sz) : acl::zmalloc(i_sz), i_sz);
  }

  inline static void deallocate(address i_addr, size_type i_sz, size_type i_alignment = 0)
  {
    auto  measure = statistics::report_deallocate(i_sz);
    void* fixup   = tracker::when_deallocate(i_addr, i_sz);
    if (i_alignment > alignof(std::max_align_t))
      acl::aligned_free(fixup);
    else
      acl::free(fixup);
  }

  static constexpr void* null()
  {
    return nullptr;
  }

  inline bool operator==(default_allocator const&) const
  {
    return true;
  }

  inline bool operator!=(default_allocator const&) const
  {
    return false;
  }
};

template <typename T, typename UA = default_allocator<std::size_t>>
using vector = std::vector<T, acl::allocator_wrapper<T, UA>>;
} // namespace acl
