#pragma once
#include "detail/arena_allocator_impl.hpp"

namespace acl
{

//  -█████╗-██████╗-███████╗███╗---██╗-█████╗----------█████╗-██╗-----██╗------██████╗--██████╗-█████╗-████████╗-██████╗-██████╗-
//  ██╔══██╗██╔══██╗██╔════╝████╗--██║██╔══██╗--------██╔══██╗██║-----██║-----██╔═══██╗██╔════╝██╔══██╗╚══██╔══╝██╔═══██╗██╔══██╗
//  ███████║██████╔╝█████╗--██╔██╗-██║███████║--------███████║██║-----██║-----██║---██║██║-----███████║---██║---██║---██║██████╔╝
//  ██╔══██║██╔══██╗██╔══╝--██║╚██╗██║██╔══██║--------██╔══██║██║-----██║-----██║---██║██║-----██╔══██║---██║---██║---██║██╔══██╗
//  ██║--██║██║--██║███████╗██║-╚████║██║--██║███████╗██║--██║███████╗███████╗╚██████╔╝╚██████╗██║--██║---██║---╚██████╔╝██║--██║
//  ╚═╝--╚═╝╚═╝--╚═╝╚══════╝╚═╝--╚═══╝╚═╝--╚═╝╚══════╝╚═╝--╚═╝╚══════╝╚══════╝-╚═════╝--╚═════╝╚═╝--╚═╝---╚═╝----╚═════╝-╚═╝--╚═╝
//  -----------------------------------------------------------------------------------------------------------------------------
template <typename manager_t, typename usize_t = std::size_t, alloc_strategy strategy_v = alloc_strategy::best_fit_tree,
          bool k_compute_stats_v = false>
class arena_allocator : public detail::arena_allocator_impl<
                          detail::arena_allocator_traits<manager_t, usize_t, strategy_v, k_compute_stats_v>>
{
  using traits    = detail::arena_allocator_traits<manager_t, usize_t, strategy_v, k_compute_stats_v>;
  using size_type = typename traits::size_type;
  using base      = detail::arena_allocator_impl<traits>;

public:
  using tag        = typename base::tag;
  using alloc_info = acl::alloc_info<size_type>;
  using alloc_desc = acl::alloc_desc<size_type>;

  template <typename... Args>
  arena_allocator(size_type i_arena_size, typename traits::manager& i_manager, Args&&... args)
      : base(i_arena_size, i_manager, std::forward<Args>(args)...)
  {}
};

} // namespace acl
