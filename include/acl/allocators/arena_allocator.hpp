#pragma once

#include <acl/allocators/config.hpp>
#include <acl/allocators/detail/arena.hpp>
#include <acl/allocators/detail/arena_manager_defs.hpp>

#include <bit>
#include <concepts>
#include <functional>
#include <tuple>

/**
 * @file arena_allocator.hpp
 * @brief Arena-based memory allocation system with defragmentation support
 *
 * This file implements a sophisticated arena allocator that manages memory in fixed-size
 * blocks (arenas) with support for:
 * - Custom allocation strategies
 * - Memory defragmentation
 * - Memory tracking and statistics
 * - Configurable memory managers
 * - Alignment control
 *
 * Key components:
 * - arena_allocator: Main allocator class template
 * - MemoryManager concept: Interface for external memory management
 * - HasDefragmentSupport concept: Interface for defragmentation capabilities
 *
 * Features:
 * - Efficient memory allocation and deallocation
 * - Block coalescing for reduced fragmentation
 * - Optional memory manager integration
 * - Customizable allocation strategies
 * - Memory movement tracking during defragmentation
 * - Arena size customization
 * - Memory alignment support
 * - Statistics collection
 *
 * The allocator supports different operation modes:
 * - Standalone mode (no external memory manager)
 * - Managed mode (with external memory manager)
 * - With or without defragmentation support
 * - With or without statistics tracking
 *
 * @note The implementation uses template metaprogramming extensively to provide
 * compile-time configuration config through the Config parameter.
 *
 * @see acl::arena_allocator
 * @see acl::MemoryManager
 * @see acl::HasDefragmentSupport
 */

namespace acl
{
template <typename T>
concept MemoryManager = requires(T m) {
  /**
   * Drop Arena
   */
  { m.drop_arena(std::uint32_t()) } -> std::same_as<bool>;
  /**
   * Add an arena
   */
  { m.add_arena(std::uint32_t(), std::size_t()) } -> std::same_as<std::uint32_t>;
  // Remoe an arena
  m.remove_arena(std::uint32_t());
};

template <typename T, typename A>
concept HasDefragmentSupport = requires(T a, A& allocator, std::uint32_t src_arena, std::uint32_t dst_arena,
                                        std::uint32_t alloc_info, std::size_t from, std::size_t to, std::size_t size) {
  // Begin defragment
  a.begin_defragment(allocator);
  // End defragmentation
  a.end_defragment(allocator);
  // Rebind an allocation to another value
  a.rebind_alloc(alloc_info, src_arena, alloc_info, typename A::size_type());
  // Move memory from source arena to another
  a.move_memory(src_arena, dst_arena, from, to, size);
};

template <typename Config = std::monostate>
/**
 * @brief A memory allocator that manages memory in arenas (contiguous memory blocks).
 *
 * The arena_allocator provides efficient memory allocation and deallocation by managing
 * memory in fixed-size arenas. It supports memory defragmentation, statistics tracking,
 * and custom memory management strategies.
 *
 * @tparam Config Configuration config for the allocator, including:
 *         - Memory manager support
 *         - Defragmentation capabilities
 *         - Statistics tracking
 *         - Custom allocation strategies
 *
 * Key features:
 * - Arena-based memory management
 * - Optional memory defragmentation
 * - Configurable arena sizes
 * - Memory movement tracking
 * - Memory alignment support
 * - Memory statistics collection
 * - Dedicated arena allocation for large memory blocks
 *
 * The allocator maintains internal data structures to track:
 * - Free and allocated memory blocks
 * - Arena organization
 * - Memory block ordering
 * - Allocation metadata
 *
 * Memory operations:
 * - Allocation with optional alignment
 * - Deallocation with automatic memory coalescing
 * - Memory defragmentation (when supported)
 * - Arena creation and management
 *
 * @note The allocator can be configured to work with or without an external memory manager
 *       through the Config template parameter.
 *
 * @see memory_move For memory movement tracking during defragmentation
 * @see alloc_info For allocation information storage
 */
class arena_allocator : acl::detail::statistics<acl::detail::arena_allocator_tag,
                                                acl::config<Config, cfg::base_stats<acl::detail::defrag_stats>>>
{

public:
  using strategy      = typename acl::detail::strategy<Config>::strategy_t;
  using extension     = typename strategy::extension;
  using arena_manager = typename acl::detail::manager<Config>::manager_t;
  using size_type     = acl::detail::choose_size_t<uint32_t, Config>;
  using this_type     = arena_allocator<Config>;

  static constexpr bool has_memory_mgr = acl::detail::HasMemoryManager<Config>;
  static constexpr bool can_defragment = HasDefragmentSupport<arena_manager, this_type>;

protected:
  using config = Config;

  using block          = acl::detail::block<size_type, extension>;
  using block_bank     = acl::detail::block_bank<size_type, extension>;
  using block_accessor = acl::detail::block_accessor<size_type, extension>;
  using block_list     = acl::detail::block_list<size_type, extension>;
  using arena_bank     = acl::detail::arena_bank<size_type, extension>;
  using arena_list     = acl::detail::arena_list<size_type, extension>;

  using super = acl::detail::statistics<acl::detail::arena_allocator_tag,
                                        acl::config<Config, cfg::base_stats<acl::detail::defrag_stats>>>;

  using statistics = super;
  using block_link = typename block_bank::link;

  using bank_data = acl::detail::bank_data<size_type, extension>;

  struct remap_data
  {
    bank_data bank_;
    strategy  strat_;
  };

public:
  /**
   * @brief Represents a memory movement operation between locations and arenas
   *
   * This structure tracks the source and destination information for memory moves,
   * including positions and arena identifiers.
   */
  struct memory_move
  {
    size_type     from_{};
    size_type     to_{};
    size_type     size_{};
    std::uint32_t arena_src_{};
    std::uint32_t arena_dst_{};

    /**
     * @brief Checks if memory has been moved from its original location
     * @return true if memory has been moved (different position or different arena)
     * @return false if memory is in original location
     */
    [[nodiscard]] auto is_moved() const noexcept -> bool
    {
      return (from_ != to_ || arena_src_ != arena_dst_);
    }

    /**
     * @brief Constructs a memory_move with specific movement parameters
     * @param ifrom Starting position in source arena
     * @param ito Destination position in target arena
     * @param isize Size of memory block to move
     * @param iarena_src Source arena identifier
     * @param iarena_dst Destination arena identifier
     */
    memory_move(size_type ifrom, size_type ito, size_type isize, std::uint32_t iarena_src, std::uint32_t iarena_dst)
        : from_(ifrom), to_(ito), size_(isize), arena_src_(iarena_src), arena_dst_(iarena_dst)
    {}

    memory_move() noexcept                             = default;
    memory_move(const memory_move&)                    = default;
    memory_move(memory_move&&)                         = default;
    auto operator=(const memory_move&) -> memory_move& = default;
    auto operator=(memory_move&&) -> memory_move&      = default;

    ~memory_move() = default;

    void reset()
    {
      from_      = 0;
      to_        = 0;
      size_      = 0;
      arena_src_ = 0;
      arena_dst_ = 0;
    }

    auto from() const noexcept -> size_type
    {
      return from_;
    }

    auto to() const noexcept -> size_type
    {
      return to_;
    }

    auto size() const noexcept -> size_type
    {
      return size_;
    }

    [[nodiscard]] auto arena_src() const noexcept -> std::uint32_t
    {
      return arena_src_;
    }

    [[nodiscard]] auto arena_dst() const noexcept -> std::uint32_t
    {
      return arena_dst_;
    }
  };

  /**
   * Allocation info: [optional] arena, offset, input_handle
   */
  using alloc_info = std::conditional_t<has_memory_mgr, std::tuple<std::uint32_t, std::uint32_t, size_type>,
                                        std::pair<std::uint32_t, size_type>>;

  /**
   * @brief Constructs an arena allocator with a specified size and manager
   * @param i_arena_size The size of the memory arena to be managed
   * @param i_manager Reference to the arena manager that will oversee this allocator
   * @requires has_memory_mgr must be true for this constructor to be available
   * @note This constructor is marked noexcept and will not throw exceptions
   */
  arena_allocator(size_type i_arena_size, arena_manager& i_manager) noexcept
    requires(has_memory_mgr)
      : mgr_(&i_manager), arena_size_(i_arena_size)
  {
    ibank_.strat_.init(*this);
  }

  /**
   * @brief Default constructor for the arena_allocator
   *
   * Initializes the internal bank strategy and, if no memory manager is present,
   * creates an initial arena of default size.
   *
   * @note This constructor is marked noexcept
   *
   * @details If has_memory_mgr is false, it automatically adds a default arena
   * using std::numeric_limits<size_type>::max() handle with arena_size_ and marks it as static
   */
  arena_allocator() noexcept
  {
    ibank_.strat_.init(*this);
    if constexpr (!has_memory_mgr)
    {
      add_arena(std::numeric_limits<uint32_t>::max(), arena_size_, true);
    }
  }

  template <typename... Args>
  arena_allocator(size_type i_arena_size) noexcept : arena_size_(i_arena_size)
  {
    ibank_.strat_.init(*this);
    if constexpr (!has_memory_mgr)
    {
      add_arena(std::numeric_limits<uint32_t>::max(), arena_size_, true);
    }
  }

  arena_allocator(arena_allocator const&) noexcept = default;
  arena_allocator(arena_allocator&&) noexcept      = default;

  ~arena_allocator() noexcept = default;

  auto operator=(arena_allocator const&) noexcept -> arena_allocator& = default;
  auto operator=(arena_allocator&&) noexcept -> arena_allocator&      = default;

  /**
   * @brief Returns the root memory block of the arena allocator.
   *
   * @return The root memory block handle.
   */
  auto get_root_block() const
  {
    return ibank_.bank_.root_blk;
  }

  /**
   * @brief Gets the arena and offset associated with a given handle.
   *
   * @param i_address The internal handle representing an allocation
   * @return std::pair<arena_id_t, size_t> Pair containing:
   *         - First: The arena identifier where allocation exists
   *         - Second: The offset within that arena
   */
  auto get_alloc_offset(std::uint32_t i_address) const
  {
    auto const& blk = ibank_.bank_.blocks()[block_link(i_address)];
    return std::pair(blk.arena_, blk.offset_);
  }

  /**
   * @brief Allocates memory from the arena with specified size and alignment
   *
   * @param isize Size of memory to allocate
   * @param i_alignment Alignment requirement for the allocation (default: no specific alignment)
   * @param huser User handle for the allocation (default: null handle)
   * @param Dedicated Whether the allocation requires a dedicated arena (unused parameter)
   *
   * @return alloc_info Structure containing:
   *         - If has_memory_mgr is true: pointer to allocated memory, block ID, and offset
   *         - If has_memory_mgr is false: block ID and offset
   *
   * @details
   * The allocation strategy is as follows:
   * 1. If allocation is dedicated or larger than arena size, creates a new arena
   * 2. Otherwise, attempts to allocate from existing arenas using the bank strategy
   * 3. If allocation fails and has_memory_mgr is true, attempts to create a new arena
   * 4. Returns empty alloc_info if allocation fails
   */
  template <typename Alignment = alignment<>, typename Dedicated = std::false_type>
  auto allocate(size_type isize, Alignment i_alignment = {}, std::uint32_t huser = {}, Dedicated /*unused*/ = {})
   -> alloc_info
  {
    auto measure = this->statistics::report_allocate(isize);
    auto size    = isize + static_cast<size_type>(i_alignment);

    if (Dedicated::value || size >= arena_size_)
    {
      auto ret = add_arena(huser, size, false);
      if constexpr (has_memory_mgr)
      {
        return alloc_info(ibank_.bank_.arenas()[ret.first].data_, ret.second, 0);
      }
      else
      {
        return alloc_info(ret.second, 0);
      }
    }

    std::uint32_t id = null();
    if (auto ta = ibank_.strat_.try_allocate(ibank_.bank_, size))
    {
      id = ibank_.strat_.commit(ibank_.bank_, size, ta);
    }
    else
    {
      if constexpr (has_memory_mgr)
      {
        if (id == null())
        {
          add_arena(std::numeric_limits<uint32_t>::max(), arena_size_, true);
          ta = ibank_.strat_.try_allocate(ibank_.bank_, size);
          if (ta)
          {
            id = ibank_.strat_.commit(ibank_.bank_, size, ta);
          }
        }
      }
    }

    if (id == null())
    {
      return alloc_info();
    }

    auto& blk = ibank_.bank_.blocks()[block_link(id)];

    if constexpr (has_memory_mgr)
    {
      return alloc_info(ibank_.bank_.arenas()[blk.arena_].data_, id, finalize_commit(blk, huser, i_alignment));
    }
    else
    {
      return alloc_info(id, finalize_commit(blk, huser, i_alignment));
    }
  }

  /**
   * @brief Deallocates a memory block and handles block merging
   *
   * This function deallocates a memory block identified by the given handle. It updates
   * the allocator statistics and performs block merging when possible. The function can
   * merge the block with adjacent free blocks (left and/or right) to reduce fragmentation.
   * If memory manager is enabled and an arena becomes completely free, it may be dropped.
   *
   * The merging strategy has four possible outcomes:
   * - No merge: The block is simply marked as free
   * - Left merge: Combines with free block on the left
   * - Right merge: Combines with free block on the right
   * - Both merge: Combines with free blocks on both sides
   *
   * @param node Handle to the memory block to deallocate
   *
   * @note This operation updates internal free size tracking and arena statistics
   */
  void deallocate(std::uint32_t node)
  {
    auto& blk     = ibank_.bank_.blocks()[block_link(node)];
    auto  measure = this->statistics::report_deallocate(blk.size());

    enum
    {
      f_left  = 1 << 0,
      f_right = 1 << 1,
    };

    enum merge_type
    {
      e_none,
      e_left,
      e_right,
      e_left_and_right
    };

    auto& arena     = ibank_.bank_.arenas()[blk.arena_];
    auto& node_list = arena.block_order();

    // last index is not used
    ibank_.bank_.free_size_ += blk.size();
    arena.free_ += blk.size();
    auto size = blk.size();

    std::uint32_t left   = 0;
    std::uint32_t right  = 0;
    std::uint32_t merges = 0;

    if (node != node_list.front() && ibank_.bank_.blocks()[block_link(blk.arena_order_.prev_)].is_free_)
    {
      left = blk.arena_order_.prev_;
      merges |= f_left;
    }

    if (node != node_list.back() && ibank_.bank_.blocks()[block_link(blk.arena_order_.next_)].is_free_)
    {
      right = blk.arena_order_.next_;
      merges |= f_right;
    }

    if constexpr (has_memory_mgr)
    {
      if (arena.free_ == arena.size() && mgr_->drop_arena(arena.data_))
      {
        // drop arena?
        if (left != 0U)
        {
          ibank_.strat_.erase(ibank_.bank_.blocks(), left);
        }
        if (right != 0U)
        {
          ibank_.strat_.erase(ibank_.bank_.blocks(), right);
        }

        std::uint32_t arena_id = blk.arena_;
        ibank_.bank_.free_size_ -= arena.size();
        arena.size_ = 0;
        arena.block_order().clear(ibank_.bank_.blocks());
        ibank_.bank_.arena_order_.erase(ibank_.bank_.arenas(), arena_id);
        return;
      }
    }

    switch (merges)
    {
    case merge_type::e_none:
      ibank_.strat_.add_free(ibank_.bank_.blocks(), node);
      blk.is_free_ = true;
      break;
    case merge_type::e_left:
    {
      auto left_size = ibank_.bank_.blocks()[block_link(left)].size();
      ibank_.strat_.grow_free_node(ibank_.bank_.blocks(), left, left_size + size);
      node_list.erase(ibank_.bank_.blocks(), node);
    }
    break;
    case merge_type::e_right:
    {
      auto right_size = ibank_.bank_.blocks()[block_link(right)].size();
      ibank_.strat_.replace_and_grow(ibank_.bank_.blocks(), right, node, right_size + size);
      node_list.erase(ibank_.bank_.blocks(), right);
      blk.is_free_ = true;
    }
    break;
    case merge_type::e_left_and_right:
    {
      auto left_size  = ibank_.bank_.blocks()[block_link(left)].size();
      auto right_size = ibank_.bank_.blocks()[block_link(right)].size();
      ibank_.strat_.erase(ibank_.bank_.blocks(), right);
      ibank_.strat_.grow_free_node(ibank_.bank_.blocks(), left, left_size + right_size + size);
      node_list.erase2(ibank_.bank_.blocks(), node);
    }
    break;
    default:
      break;
    }
  }

  // set default arena size
  void set_arena_size(size_type isz)
  {
    arena_size_ = isz;
  }

  // null
  static constexpr auto null() -> std::uint32_t
  {
    return 0;
  }

  // validate
  void validate_integrity() const
  {
    std::uint32_t total_free_nodes = 0;
    for (auto arena_it     = ibank_.bank_.arena_order_.begin(ibank_.bank_.arenas()),
              arena_end_it = ibank_.bank_.arena_order_.end(ibank_.bank_.arenas());
         arena_it != arena_end_it; ++arena_it)
    {
      auto& arena           = *arena_it;
      bool  arena_allocated = false;

      for (auto blk_it     = arena.block_order().begin(ibank_.bank_.blocks()),
                blk_end_it = arena.block_order().end(ibank_.bank_.blocks());
           blk_it != blk_end_it; ++blk_it)
      {
        auto& blk = *blk_it;
        if ((blk.is_free_))
        {
          total_free_nodes++;
        }
      }
    }

    assert(total_free_nodes == ibank_.strat_.total_free_nodes(ibank_.bank_.blocks()));
    auto total = ibank_.strat_.total_free_size(ibank_.bank_.blocks());
    assert(total == ibank_.bank_.free_size_);

    for (auto arena_it     = ibank_.bank_.arena_order_.begin(ibank_.bank_.arenas()),
              arena_end_it = ibank_.bank_.arena_order_.end(ibank_.bank_.arenas());
         arena_it != arena_end_it; ++arena_it)
    {
      auto&     arena           = *arena_it;
      bool      arena_allocated = false;
      size_type expected_offset = 0;

      for (auto blk_it     = arena.block_order().begin(ibank_.bank_.blocks()),
                blk_end_it = arena.block_order().end(ibank_.bank_.blocks());
           blk_it != blk_end_it; ++blk_it)
      {
        auto& blk = *blk_it;
        assert(blk.offset_ == expected_offset);
        expected_offset += blk.size();
      }
    }

    ibank_.strat_.validate_integrity(ibank_.bank_.blocks());
  }

  /**
   * @brief Defragments the memory arena by consolidating allocated blocks and removing empty arenas.
   *
   * This function performs memory defragmentation by:
   * 1. Creating a new memory layout with compacted allocations
   * 2. Moving allocated blocks to new locations
   * 3. Updating bindings for moved blocks
   * 4. Removing empty arenas
   *
   * The defragmentation process:
   * - Iterates through all arenas and their blocks
   * - Copies allocated blocks to new locations using a fresh allocation strategy
   * - Tracks memory moves and rebinding information
   * - Executes memory moves in correct sequence to prevent overwrites
   * - Updates all bindings to point to new locations
   * - Cleans up empty arenas
   *
   * @note This function is only available when the allocator supports defragmentation (can_defragment = true)
   * @note Statistics are updated if Config includes ComputeStats
   */
  void defragment()
    requires(can_defragment)
  {
    mgr_->begin_defragment(*this);
    std::uint32_t arena_id = ibank_.bank_.arena_order_.first_;
    // refresh all banks
    remap_data refresh;
    refresh.strat_.init(*this);

    acl::vector<std::uint32_t> rebinds;
    rebinds.reserve(ibank_.bank_.blocks().size());

    acl::vector<memory_move>            moves;
    decltype(ibank_.bank_.arena_order_) deleted_arenas;
    for (auto arena_it = ibank_.bank_.arena_order_.front(); arena_it != 0;)
    {
      auto& arena           = ibank_.bank_.arenas()[arena_it];
      bool  arena_allocated = false;

      for (auto blk_it = arena.block_order().begin(ibank_.bank_.blocks()); blk_it;
           blk_it      = arena.block_order().erase(blk_it))
      {
        auto& blk = *blk_it;
        if (!blk.is_free_)
        {
          auto ta = refresh.strat_.try_allocate(refresh.bank_, blk.size());
          if (!ta && !arena_allocated)
          {
            auto p = add_arena(refresh, std::numeric_limits<uint32_t>::max(), std::max(arena.size(), blk.size()), true);
            refresh.bank_.arenas()[p.first].data_ = arena.data_;
            ta                                    = refresh.strat_.try_allocate(refresh.bank_, blk.size());
            arena_allocated                       = true;
          }
          assert(ta);

          auto  new_blk_id = refresh.strat_.commit(refresh.bank_, blk.size(), ta);
          auto& new_blk    = refresh.bank_.blocks()[block_link(new_blk_id)];
          refresh.bank_.arenas()[new_blk.arena_].free_ -= blk.size();
          refresh.bank_.free_size_ -= blk.size();

          copy(blk, new_blk);
          rebinds.emplace_back(new_blk_id);
          auto blk_adj = blk.adjusted_block();
          push_memmove(
           moves, memory_move(blk_adj.first, new_blk.adjusted_offset(), blk_adj.second, blk.arena_, new_blk.arena_));
        }
      }

      if (!arena_allocated)
      {
        auto to_delete = arena_it;
        arena_it       = ibank_.bank_.arena_order_.unlink(ibank_.bank_.arenas(), to_delete);
        arena.free_    = arena.size();
        deleted_arenas.push_back(ibank_.bank_.arenas(), to_delete);
      }
      else
      {
        arena_it = ibank_.bank_.arena_order_.next(ibank_.bank_.arenas(), arena_it);
      }
    }

    for (auto& m : moves)
    {
      // follow the copy sequence to ensure there is no overwrite
      mgr_->move_memory(ibank_.bank_.arenas_[m.arena_src_].data_, refresh.bank_.arenas_[m.arena_dst_].data_, m.from_,
                        m.to_, m.size_);
    }

    for (auto rb : rebinds)
    {
      auto& dst_blk = refresh.bank_.blocks()[block_link(rb)];
      mgr_->rebind_alloc(dst_blk.data_, refresh.bank_.arenas()[dst_blk.arena_].data_, rb, dst_blk.adjusted_offset());
    }

    for (auto arena_it = deleted_arenas.begin(ibank_.bank_.arenas()); arena_it;
         arena_it      = deleted_arenas.erase(arena_it))
    {
      auto& arena = *arena_it;
      mgr_->remove_arena(arena.data_);
      if constexpr (acl::detail::HasComputeStats<Config>)
      {
        statistics::report_defrag_arenas_removed();
      }
    }

    ibank_.bank_  = std::move(refresh.bank_);
    ibank_.strat_ = std::move(refresh.strat_);
    mgr_->end_defragment(*this);
  }

private:
  auto add_arena(std::uint32_t handle, size_type iarena_size, bool empty) -> std::pair<std::uint32_t, std::uint32_t>
  {
    this->statistics::report_new_arena();
    auto ret = add_arena(ibank_, handle, iarena_size, empty);
    if constexpr (has_memory_mgr)
    {
      ibank_.bank_.arenas()[ret.first].data_ = mgr_->add_arena(ret.first, iarena_size);
    }
    return ret;
  }

  static auto add_arena(remap_data& ibank, std::uint32_t handle, size_type iarena_size, bool iempty)
   -> std::pair<std::uint32_t, std::uint32_t>
  {

    std::uint32_t arena_id  = ibank.bank_.arenas().emplace();
    auto&         arena_ref = ibank.bank_.arenas()[arena_id];
    arena_ref.size_         = iarena_size;
    auto   block_id         = ibank.bank_.blocks().emplace();
    block& block_ref        = ibank.bank_.blocks()[block_id];
    block_ref.offset_       = 0;
    block_ref.arena_        = arena_id;
    block_ref.data_         = handle;
    block_ref.size_         = iarena_size;
    if (iempty)
    {
      block_ref.is_free_ = true;
      arena_ref.free_    = iarena_size;
      ibank.strat_.add_free_arena(ibank.bank_.blocks(), (uint32_t)block_id);
      ibank.bank_.free_size_ += iarena_size;
    }
    else
    {
      arena_ref.free_ = 0;
    }
    arena_ref.block_order().push_back(ibank.bank_.blocks(), (uint32_t)block_id);
    ibank.bank_.arena_order_.push_back(ibank.bank_.arenas(), arena_id);
    return std::make_pair(arena_id, (uint32_t)block_id);
  }

  template <typename Alignment = alignment<>>
  auto finalize_commit(block& blk, std::uint32_t huser, Alignment ialign) -> size_type
  {
    blk.data_      = huser;
    blk.alignment_ = static_cast<std::uint8_t>(std::popcount(static_cast<uint32_t>(ialign)));
    ibank_.bank_.arenas()[blk.arena_].free_ -= blk.size_;
    ibank_.bank_.free_size_ -= blk.size_;
    auto alignment = static_cast<size_type>(ialign);
    return ((blk.offset_ + alignment) & ~alignment);
  }

  static void copy(block const& src, block& dst)
  {
    dst.data_      = src.data_;
    dst.alignment_ = src.alignment_;
  }

  void push_memmove(acl::vector<memory_move>& dst, memory_move value)
  {
    if (!value.is_moved())
    {
      return;
    }
    auto can_merge = [](memory_move const& m1, memory_move const& m2) -> bool
    {
      return ((m1.arena_dst_ == m2.arena_dst_ && m1.arena_src_ == m2.arena_src_) &&
              (m1.from_ + m1.size_ == m2.from_ && m1.to_ + m1.size_ == m2.to_));
    };
    if (dst.empty() || !can_merge(dst.back(), value))
    {
      dst.push_back(value);
    }
    else
    {
      dst.back().size_ += value.size_;
      if constexpr (acl::detail::HasComputeStats<Config>)
      {
        statistics::report_defrag_mem_move_merge();
      }
    }
  }

  remap_data     ibank_;
  size_type      arena_size_ = std::numeric_limits<size_type>::max();
  arena_manager* mgr_        = nullptr;
};

} // namespace acl
