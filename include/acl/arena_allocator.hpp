#pragma once
#include "detail/arena_allocator_impl.hpp"
#include "strat_best_fit_tree.hpp"
#include "strat_best_fit_v0.hpp"
#include "strat_slotted_v0.hpp"
#include "strat_slotted_v1.hpp"
#include "strat_slotted_v2.hpp"
#include "strat_greedy_v0.hpp"
#include "strat_greedy_v1.hpp"

namespace acl
{

//  -█████╗-██████╗-███████╗███╗---██╗-█████╗----------█████╗-██╗-----██╗------██████╗--██████╗-█████╗-████████╗-██████╗-██████╗-
//  ██╔══██╗██╔══██╗██╔════╝████╗--██║██╔══██╗--------██╔══██╗██║-----██║-----██╔═══██╗██╔════╝██╔══██╗╚══██╔══╝██╔═══██╗██╔══██╗
//  ███████║██████╔╝█████╗--██╔██╗-██║███████║--------███████║██║-----██║-----██║---██║██║-----███████║---██║---██║---██║██████╔╝
//  ██╔══██║██╔══██╗██╔══╝--██║╚██╗██║██╔══██║--------██╔══██║██║-----██║-----██║---██║██║-----██╔══██║---██║---██║---██║██╔══██╗
//  ██║--██║██║--██║███████╗██║-╚████║██║--██║███████╗██║--██║███████╗███████╗╚██████╔╝╚██████╗██║--██║---██║---╚██████╔╝██║--██║
//  ╚═╝--╚═╝╚═╝--╚═╝╚══════╝╚═╝--╚═══╝╚═╝--╚═╝╚══════╝╚═╝--╚═╝╚══════╝╚══════╝-╚═════╝--╚═════╝╚═╝--╚═╝---╚═╝----╚═════╝-╚═╝--╚═╝
//  -----------------------------------------------------------------------------------------------------------------------------
template <typename strategy, typename manager, typename usize_t = std::size_t, bool k_compute_stats_v = false>
class arena_allocator : public detail::arena_allocator_impl<strategy, manager, usize_t, k_compute_stats_v>
{
protected:
  using super = detail::arena_allocator_impl<strategy, manager, usize_t, k_compute_stats_v>;

public:
  static constexpr usize_t min_granularity = strategy::min_granularity;

  using size_type  = usize_t;
  using alloc_info = acl::alloc_info<size_type>;
  
  template <typename... Args>
  arena_allocator(size_type i_arena_size, manager& i_manager, Args&&... args)
      : super(i_arena_size, i_manager, std::forward<Args>(args)...)
  {}
};

} // namespace acl
