
#pragma once

#include <acl/allocators/config.hpp>
#include <acl/allocators/strat/best_fit_v2.hpp>
#include <format>

namespace acl::detail
{
template <typename T>
struct manager
{
  using manager_t = std::false_type;
};

template <HasMemoryManager T>
struct manager<T>
{
  using manager_t = typename T::manager_t;
};

template <typename T>
struct strategy
{
  using strategy_t = acl::strat::best_fit_v2<acl::cfg::bsearch_min1>;
};

template <HasAllocStrategy T>
struct strategy<T>
{
  using strategy_t = typename T::strategy_t;
};

struct defrag_stats
{
  std::uint32_t total_mem_move_merge_ = 0;
  std::uint32_t total_arenas_removed_ = 0;

  void report_defrag_mem_move_merge()
  {
    total_mem_move_merge_++;
  }

  void report_defrag_arenas_removed()
  {
    total_arenas_removed_++;
  }

  [[nodiscard]] auto print() const -> std::string
  {
    return std::format("Defrag memory move merges: {}\nDefrag arenas removed: {}", total_mem_move_merge_,
                       total_arenas_removed_);
  }
};

struct arena_allocator_tag
{};
} // namespace acl::detail