// SPDX-License-Identifier: MIT

#pragma once

#include "ouly/allocators/alignment.hpp"
#include "ouly/allocators/allocation_id.hpp"
#include "ouly/allocators/detail/tlsf_bins.hpp"
#include "ouly/allocators/linear_stack_allocator.hpp"
#include "ouly/allocators/scratch_allocator.hpp"
#include "ouly/utility/user_config.hpp"
#include <algorithm>
#include <bit>
#include <compare>
#include <concepts>
#include <cstdint>
#include <limits>
#include <optional>
#include <type_traits>
#include <vector>

namespace ouly
{

/**
 * @brief Identifier for a relocation operation.
 *
 * Like `allocation_id`, relocation ids are 32-bit table/stream identifiers with the maximum value
 * reserved as the null value.
 */
struct ca_move_id
{
  uint32_t id_ = std::numeric_limits<uint32_t>::max();

  [[nodiscard]] auto get() const noexcept -> uint32_t
  {
    return id_;
  }

  auto operator<=>(ca_move_id const&) const noexcept = default;
};

using ca_timeline_value = uint64_t;

/** @brief Runtime properties of a suballocation. */
struct ca_allocation_options
{
  allocation_size_type alignment_      = 1;
  uint32_t             memory_class_   = 0;
  uint32_t             resource_class_ = 0;
  bool                 movable_        = true;
  bool                 dedicated_      = false;
};

/** @brief Result returned by `coalescing_arena_allocator::allocate`. */
struct ca_allocation
{
  allocation_size_type offset_ = 0;
  allocation_id        id_;
  arena_id             arena_;

  auto operator<=>(ca_allocation const&) const noexcept = default;

  [[nodiscard]] auto get_offset() const noexcept -> allocation_size_type
  {
    return offset_;
  }

  [[nodiscard]] auto get_allocation_id() const noexcept -> allocation_id
  {
    return id_;
  }

  [[nodiscard]] auto get_arena_id() const noexcept -> arena_id
  {
    return arena_;
  }
};

/** @brief A relocation handed to the executor: copy `size_` bytes between two placements. */
struct ca_relocation
{
  ca_move_id           move_;
  allocation_id        allocation_;
  arena_id             source_arena_;
  arena_id             destination_arena_;
  allocation_size_type source_offset_      = 0;
  allocation_size_type destination_offset_ = 0;
  allocation_size_type size_               = 0;
  uint32_t             memory_class_       = 0;
  uint32_t             resource_class_     = 0;
};

/** @brief Per-call limits for incremental defragmentation. */
struct ca_defrag_budget
{
  allocation_size_type max_copy_bytes_     = std::numeric_limits<allocation_size_type>::max();
  uint32_t             max_move_count_     = std::numeric_limits<uint32_t>::max();
  uint32_t             max_blocks_scanned_ = ouly::cfg::default_ca_arenas_to_scan;
};

/** @brief Work performed by one defragmentation/completion-processing call. */
struct ca_defrag_result
{
  allocation_size_type bytes_scheduled_   = 0;
  allocation_size_type bytes_copied_      = 0;
  allocation_size_type bytes_released_    = 0;
  uint32_t             moves_scheduled_   = 0;
  uint32_t             moves_completed_   = 0;
  uint32_t             arenas_removed_    = 0;
  bool                 budget_exhausted_  = false;
  bool                 evacuation_active_ = false;
  bool                 completed_         = true;
};

/** @brief Lifetime totals reported by `coalescing_arena_allocator::statistics`. */
struct ca_defrag_statistics
{
  uint64_t bytes_scheduled_           = 0;
  uint64_t bytes_copied_              = 0;
  uint64_t bytes_released_            = 0;
  uint64_t blocks_released_           = 0;
  uint64_t moves_scheduled_           = 0;
  uint64_t moves_completed_           = 0;
  uint32_t moves_pending_             = 0;
  uint32_t active_evacuation_targets_ = 0;
};

template <typename T>
concept CoalescingMemoryManagerWithClass = requires(T manager) {
  { manager.add(arena_id(), allocation_size_type(), uint32_t()) } -> std::same_as<void>;
};

template <typename T>
concept CoalescingMemoryManagerWithoutClass = requires(T manager) {
  { manager.add(arena_id(), allocation_size_type()) } -> std::same_as<void>;
};

/**
 * @brief Backing-memory owner used by `coalescing_arena_allocator`.
 *
 * `add` creates the backing memory (a heap, buffer or mapping) for an arena. Managers that distinguish memory classes
 * accept the three-argument form; simple managers may use the two-argument form. `remove` destroys it.
 */
template <typename T>
concept CoalescingMemoryManager =
 (CoalescingMemoryManagerWithClass<T> || CoalescingMemoryManagerWithoutClass<T>) && requires(T manager) {
   { manager.remove(arena_id()) } -> std::same_as<void>;
 };

/**
 * @brief Relocation executor used by `coalescing_arena_allocator::defragment`.
 *
 * Copies may be asynchronous, as on a GPU queue, or immediate: an executor that copies inside
 * `schedule_copy` and reports every timeline value as complete makes defragmentation synchronous.
 * `schedule_copy` returns the timeline value at which the destination contains the copied bytes.
 * `switch_references` runs after that point, updates future users of the stable allocation id, and
 * returns the timeline value after which old source users can no longer access the source.
 */
template <typename T>
concept CoalescingRelocationExecutor = requires(T executor, ca_relocation const& relocation, ca_timeline_value value) {
  { executor.schedule_copy(relocation) } -> std::same_as<std::optional<ca_timeline_value>>;
  { executor.switch_references(relocation) } -> std::same_as<std::optional<ca_timeline_value>>;
  { executor.is_complete(value) } -> std::convertible_to<bool>;
};

/**
 * @brief Scratch allocator `defragment` falls back to when the caller supplies none.
 *
 * Arenas are created lazily, so a call that schedules no new evacuation does not allocate at all.
 */
using ca_defrag_scratch = ouly::linear_stack_allocator<>;

/**
 * @brief Multi-arena offset suballocator with O(1) placement and budgeted whole-arena evacuation.
 *
 * Meant for GPU heaps, mapped files and other memory addressed by (arena, offset). The allocator
 * manages virtual placements only. A `CoalescingMemoryManager` owns the backing memory of each
 * arena, and a `CoalescingRelocationExecutor` records copies, rebinds stable allocation handles, and supplies
 * timeline completion information. Both are used through concepts: there are no virtual interfaces
 * or type-erased callbacks on the allocation path. Evacuation planning needs temporary storage, and
 * the caller can hand any `ScratchAllocator` (a frame allocator, for instance) to `defragment` to
 * keep that storage out of the heap.
 *
 * Placement is O(1): only occupied blocks are stored, each owning the free gap behind it, and the
 * gaps of every arena of a memory class share one set of TLSF size bins. Freeing coalesces by adding
 * the block and its gap to its predecessor's gap.
 *
 * Defragmentation deliberately evacuates one releasable arena at a time. Before an arena is frozen,
 * a destination is reserved for every allocation it holds (largest and most aligned first); if any of
 * them does not fit, the reservations are rolled back and the next candidate is tried. Copies may then
 * be submitted across many calls under byte and move-count budgets without risking a partially
 * evacuated arena that cannot be completed.
 *
 * Dedicated and explicitly non-movable allocations are pinned. New allocations never enter an active
 * evacuation source. The public `allocation_id` remains stable while its arena and offset change only
 * after `switch_references` succeeds.
 *
 * @note The class is not internally synchronized. Allocation, completion processing, and
 * defragmentation must be externally serialized.
 */
class coalescing_arena_allocator
{
public:
  using size_type = allocation_size_type;

  OULY_API coalescing_arena_allocator();
  OULY_API explicit coalescing_arena_allocator(size_type arena_size);

  OULY_API void set_arena_size(size_type size) noexcept;

  /**
   * @brief How many empty arenas each memory class keeps instead of returning them to the manager.
   *
   * An arena that becomes empty through deallocation stays registered (and is reused by the next
   * allocation of its class) while fewer than this many empty arenas of the class remain, so an
   * alloc/free pattern that empties an arena does not create and destroy backing memory each time.
   * Dedicated arenas and arenas emptied by an evacuation are always released. Defaults to 1; use
   * trim() to hand the retained arenas back. Lowering the count does not release arenas already
   * retained until the next trim().
   */
  OULY_API void set_retained_empty_arenas(uint32_t count) noexcept;

  [[nodiscard]] OULY_API auto get_retained_empty_arenas() const noexcept -> uint32_t;

  [[nodiscard]] OULY_API auto get_arena_size() const noexcept -> size_type;
  [[nodiscard]] OULY_API auto get_offset(allocation_id id) const noexcept -> size_type;
  [[nodiscard]] OULY_API auto get_size(allocation_id id) const noexcept -> size_type;
  [[nodiscard]] OULY_API auto get_alignment(allocation_id id) const noexcept -> size_type;
  [[nodiscard]] OULY_API auto get_arena(allocation_id id) const noexcept -> arena_id;
  [[nodiscard]] OULY_API auto get_memory_class(allocation_id id) const noexcept -> uint32_t;
  [[nodiscard]] OULY_API auto get_resource_class(allocation_id id) const noexcept -> uint32_t;
  [[nodiscard]] OULY_API auto is_movable(allocation_id id) const noexcept -> bool;
  [[nodiscard]] OULY_API auto is_relocating(allocation_id id) const noexcept -> bool;

  OULY_API void set_movable(allocation_id id, bool movable);

  template <CoalescingMemoryManager M>
  auto allocate(size_type size, M& manager, ca_allocation_options options) -> ca_allocation
  {
    OULY_ASSERT(size != 0);
    options.alignment_ = std::max<size_type>(options.alignment_, 1);
    OULY_ASSERT(std::has_single_bit(options.alignment_));

    bool const dedicated = options.dedicated_ || size >= arena_size_;
    if (!dedicated)
    {
      if (auto allocation = try_allocate(size, options); allocation.id_ != allocation_id())
      {
        return allocation;
      }
    }

    auto const capacity = dedicated ? size : arena_size_;
    OULY_ASSERT(capacity != 0);
    auto const arena = create_arena(capacity, options.memory_class_, dedicated);
    notify_add(manager, arena, capacity, options.memory_class_);

    options.dedicated_ = dedicated;
    options.movable_   = options.movable_ && !dedicated;
    auto allocation    = allocate_in_new_arena(arena.get(), size, options);
    OULY_ASSERT(allocation.id_ != allocation_id());
    return allocation;
  }

  template <CoalescingMemoryManager M, typename Alignment = ouly::alignment<>, typename Dedicated = std::false_type>
    requires requires(Alignment value) {
      { static_cast<size_type>(value) } -> std::same_as<size_type>;
      { Dedicated::value } -> std::convertible_to<bool>;
    }
  auto allocate(size_type size, M& manager, Alignment alignment = {}, Dedicated /*unused*/ = {}) -> ca_allocation
  {
    return allocate(size, manager,
                    ca_allocation_options{.alignment_ = static_cast<size_type>(alignment),
                                          .dedicated_ = static_cast<bool>(Dedicated::value)});
  }

  /**
   * @brief Release an allocation.
   *
   * A copy-scheduled allocation cannot be released until its completion has been processed because
   * the executor still owns its source/destination transition. `try_deallocate` reports this case;
   * `deallocate` asserts that it is safe.
   */
  template <CoalescingMemoryManager M>
  auto try_deallocate(allocation_id id, M& manager) -> bool
  {
    if (!prepare_deallocate(id))
    {
      return false;
    }
    auto const arena = release_allocation(id);
    try_release_arena(manager, arena);
    finish_target_if_possible(manager);
    return true;
  }

  template <CoalescingMemoryManager M>
  void deallocate(allocation_id id, M& manager)
  {
    [[maybe_unused]] bool const released = try_deallocate(id, manager);
    OULY_ASSERT(released && "cannot deallocate while a relocation copy is pending");
  }

  /**
   * @brief Process completed copies/retirements and schedule more evacuation work under `budget`.
   *
   * Planning an evacuation needs temporary storage proportional to the number of arenas. It is taken
   * from a call-local `ca_defrag_scratch`; use the overload taking a `ScratchAllocator` to draw it
   * from a frame allocator instead.
   */
  template <CoalescingMemoryManager M, CoalescingRelocationExecutor E>
  auto defragment(M& manager, E& executor, ca_defrag_budget budget = {}) -> ca_defrag_result
  {
    ca_defrag_scratch scratch{ouly::cfg::default_ca_scratch_size};
    return defragment(manager, executor, budget, scratch);
  }

  /**
   * @brief Process completed copies/retirements and schedule more evacuation work under `budget`.
   *
   * @param scratch Allocator the evacuation planner takes its temporary buffers from. Nothing is
   * allocated from it unless a new evacuation is planned, and every buffer is dead by the time the
   * call returns, so a linear allocator that is rewound per frame is the expected choice.
   */
  template <CoalescingMemoryManager M, CoalescingRelocationExecutor E, ScratchAllocator S>
  auto defragment(M& manager, E& executor, ca_defrag_budget budget, S& scratch) -> ca_defrag_result
  {
    auto result = process_completions(manager, executor);

    if (!target_ && budget.max_copy_bytes_ != 0 && budget.max_move_count_ != 0)
    {
      begin_evacuation(budget.max_blocks_scanned_, scratch_allocator_ref{scratch});
    }

    auto remaining_bytes = budget.max_copy_bytes_;
    auto remaining_moves = budget.max_move_count_;

    while (target_ && remaining_bytes != 0 && remaining_moves != 0)
    {
      auto* move = find_schedulable_move(remaining_bytes);
      if (move == nullptr)
      {
        break;
      }

      auto const command  = make_relocation(*move);
      auto const timeline = executor.schedule_copy(command);
      if (!timeline)
      {
        cancel_move(*move);
        abort_evacuation();
        break;
      }

      move->copy_completion_ = *timeline;
      move->state_           = move_state::copy_scheduled;
      --target_->reserved_moves_;
      remaining_bytes -= move->size_;
      --remaining_moves;

      result.bytes_scheduled_ += move->size_;
      ++result.moves_scheduled_;
      statistics_.bytes_scheduled_ += move->size_;
      ++statistics_.moves_scheduled_;
    }

    if (target_ && has_reserved_moves())
    {
      result.budget_exhausted_ =
       remaining_bytes == 0 || remaining_moves == 0 || find_schedulable_move(remaining_bytes) == nullptr;
    }
    finish_target_if_possible(manager, &result);
    set_result_state(result);
    return result;
  }

  /** @brief Advance copies and source retirement without scheduling new work. */
  template <CoalescingMemoryManager M, CoalescingRelocationExecutor E>
  auto process_completions(M& manager, E& executor) -> ca_defrag_result
  {
    ca_defrag_result result;
    if (!target_)
    {
      return result;
    }

    for (auto& move : target_->moves_)
    {
      if (move.state_ == move_state::copy_scheduled && executor.is_complete(move.copy_completion_))
      {
        result.bytes_copied_ += move.size_;
        statistics_.bytes_copied_ += move.size_;

        auto const command    = make_relocation(move);
        auto const retirement = executor.switch_references(command);
        if (!retirement)
        {
          cancel_move(move);
          abort_evacuation();
          continue;
        }

        switch_allocation(move);
        move.source_retirement_ = *retirement;
        move.state_             = move_state::references_switched;
      }

      if (move.state_ == move_state::references_switched && executor.is_complete(move.source_retirement_))
      {
        retire_source(move);
        ++result.moves_completed_;
        ++statistics_.moves_completed_;
      }
    }

    finish_target_if_possible(manager, &result);
    set_result_state(result);
    return result;
  }

  /**
   * @brief Return every empty arena, including the ones retained for reuse, to the manager.
   * @return Number of arenas released
   */
  template <CoalescingMemoryManager M>
  auto trim(M& manager) -> uint32_t
  {
    uint32_t released = 0;
    // drop_arena erases from arena_order_, so walk it backwards by index
    for (auto i = arena_order_.size(); i-- > 0;)
    {
      auto const arena = ouly::detail::vector_access(arena_order_, i);
      if (can_release_arena(arena))
      {
        drop_arena(arena);
        manager.remove(arena_id{arena});
        ++released;
      }
    }
    return released;
  }

  [[nodiscard]] OULY_API auto statistics() const noexcept -> ca_defrag_statistics;

  OULY_API void validate_integrity() const;

private:
  static constexpr uint32_t null_index = std::numeric_limits<uint32_t>::max();

  enum class arena_role : uint8_t
  {
    normal,
    evacuation_source
  };

  enum class move_state : uint8_t
  {
    reserved,
    copy_scheduled,
    references_switched,
    source_retired,
    cancelled
  };

  enum class block_kind : uint8_t
  {
    unused,
    head,
    allocation,
    reservation,
    retiring
  };

  /**
   * @brief An occupied range of an arena, followed by the free gap it owns.
   *
   * Blocks double as the allocation table: an `allocation_id` is the index of its block. Each arena
   * starts with a zero-sized head block that owns the leading gap. Reservations (evacuation
   * destinations) and retiring sources are blocks too, so the free space of an arena is exactly the
   * sum of its gaps.
   */
  struct block_node
  {
    size_type  offset_         = 0;
    size_type  size_           = 0;
    size_type  gap_            = 0;
    uint32_t   prev_           = null_index;
    uint32_t   next_           = null_index;
    uint32_t   bin_prev_       = null_index;
    uint32_t   bin_next_       = null_index;
    uint32_t   move_           = null_index;
    uint32_t   resource_class_ = 0;
    uint16_t   arena_          = 0;
    uint16_t   bin_            = std::numeric_limits<uint16_t>::max();
    uint8_t    alignment_      = 0;
    block_kind kind_           = block_kind::unused;
    bool       movable_        = false;
  };

  using bins_type = ouly::detail::tlsf_bins<block_node>;

  struct arena_state
  {
    size_type  capacity_     = 0;
    size_type  free_         = 0;
    size_type  live_         = 0;
    uint32_t   head_         = null_index;
    uint32_t   allocations_  = 0;
    uint32_t   reservations_ = 0;
    uint32_t   retiring_     = 0;
    uint32_t   pinned_       = 0;
    uint32_t   memory_class_ = 0;
    uint32_t   bins_         = 0;
    arena_role role_         = arena_role::normal;
    bool       active_       = false;
    bool       dedicated_    = false;
  };

  struct planned_move
  {
    ca_move_id        move_;
    allocation_id     allocation_;
    uint32_t          reservation_        = null_index;
    uint16_t          source_arena_       = 0;
    uint16_t          destination_arena_  = 0;
    size_type         source_offset_      = 0;
    size_type         destination_offset_ = 0;
    size_type         size_               = 0;
    move_state        state_              = move_state::reserved;
    ca_timeline_value copy_completion_    = 0;
    ca_timeline_value source_retirement_  = 0;
  };

  struct evacuation_target
  {
    uint16_t                  source_arena_ = 0;
    std::vector<planned_move> moves_;
    uint32_t                  open_moves_     = 0;
    uint32_t                  reserved_moves_ = 0;
    uint32_t                  next_reserved_  = 0;
    bool                      aborted_        = false;
  };

  /** @brief An arena that may be evacuated, ranked by how much it would recover */
  struct evacuation_candidate
  {
    uint16_t  arena_    = 0;
    size_type capacity_ = 0;
    size_type live_     = 0;
    uint32_t  count_    = 0;
  };

  template <CoalescingMemoryManager M>
  static void notify_add(M& manager, arena_id arena, size_type size, uint32_t memory_class)
  {
    if constexpr (CoalescingMemoryManagerWithClass<M>)
    {
      manager.add(arena, size, memory_class);
    }
    else
    {
      manager.add(arena, size);
    }
  }

  template <CoalescingMemoryManager M>
  void try_release_arena(M& manager, uint16_t arena)
  {
    if (can_release_arena(arena) && !should_retain_empty(arena))
    {
      drop_arena(arena);
      manager.remove(arena_id{arena});
    }
  }

  template <CoalescingMemoryManager M>
  void finish_target_if_possible(M& manager, ca_defrag_result* result = nullptr)
  {
    // cancelled reservations can leave a destination arena empty
    for (auto arena : emptied_)
    {
      try_release_arena(manager, arena);
    }
    emptied_.clear();

    if (!target_ || !target_finished())
    {
      return;
    }

    auto const source = target_->source_arena_;
    target_.reset();
    open_arena(source);

    if (can_release_arena(source))
    {
      auto const released = drop_arena(source);
      manager.remove(arena_id{source});
      statistics_.bytes_released_ += released;
      ++statistics_.blocks_released_;
      if (result != nullptr)
      {
        result->bytes_released_ += released;
        ++result->arenas_removed_;
      }
    }
  }

  OULY_API auto try_allocate(size_type size, ca_allocation_options const& options) -> ca_allocation;
  OULY_API auto allocate_in_new_arena(uint16_t arena, size_type size, ca_allocation_options const& options)
   -> ca_allocation;
  OULY_API auto create_arena(size_type capacity, uint32_t memory_class, bool dedicated) -> arena_id;
  OULY_API auto place_allocation(uint32_t pred, size_type offset, size_type size, ca_allocation_options const& options)
   -> ca_allocation;
  OULY_API auto release_allocation(allocation_id id) -> uint16_t;
  OULY_API auto prepare_deallocate(allocation_id id) -> bool;

  [[nodiscard]] OULY_API auto        find_bins(uint32_t memory_class) const -> uint32_t;
  OULY_API auto                      bins_for_class(uint32_t memory_class) -> uint32_t;
  [[nodiscard]] OULY_API auto        arena_bins(uint16_t arena) -> bins_type*;
  OULY_API void                      open_arena(uint16_t arena);
  OULY_API void                      close_arena(uint16_t arena);
  OULY_API auto                      make_block() -> uint32_t;
  OULY_API void                      free_block(uint32_t index);
  OULY_API void                      swap_positions(uint32_t lhs, uint32_t rhs);
  [[nodiscard]] OULY_API auto        can_evacuate(uint16_t arena) const -> bool;
  [[nodiscard]] OULY_API static auto recovers_more(evacuation_candidate const& lhs, evacuation_candidate const& rhs)
   -> bool;
  OULY_API void               begin_evacuation(uint32_t max_blocks_scanned, scratch_allocator_ref scratch);
  OULY_API auto               plan_evacuation(uint16_t source) -> bool;
  OULY_API void               release_reservation(planned_move const& move);
  OULY_API void               close_move(planned_move& move, move_state state);
  OULY_API void               cancel_move(planned_move& move);
  OULY_API void               abort_evacuation();
  [[nodiscard]] OULY_API auto find_schedulable_move(size_type remaining_bytes) -> planned_move*;
  [[nodiscard]] OULY_API auto has_reserved_moves() const -> bool;
  [[nodiscard]] OULY_API auto target_finished() const -> bool;
  [[nodiscard]] OULY_API auto make_relocation(planned_move const& move) const -> ca_relocation;
  OULY_API void               switch_allocation(planned_move& move);
  OULY_API void               retire_source(planned_move& move);

  [[nodiscard]] OULY_API auto can_release_arena(uint16_t arena) const -> bool;
  [[nodiscard]] OULY_API auto should_retain_empty(uint16_t arena) const -> bool;
  [[nodiscard]] OULY_API auto count_empty_arenas(uint32_t memory_class) const -> uint32_t;
  OULY_API auto               drop_arena(uint16_t arena) -> size_type;
  OULY_API void               set_result_state(ca_defrag_result& result) const;
  [[nodiscard]] OULY_API auto allocate_move_id() -> ca_move_id;

  std::vector<arena_state> arena_pool_ = {arena_state{}};
  std::vector<uint16_t>    arena_order_;
  std::vector<uint16_t>    free_arenas_;
  std::vector<uint16_t>    emptied_;

  std::vector<block_node>     blocks_;
  std::vector<bins_type> bins_;
  std::vector<uint32_t>  bin_classes_;
  uint32_t               free_block_ = null_index;

  std::optional<evacuation_target> target_;
  ca_defrag_statistics             statistics_;
  size_type                        arena_size_            = 0;
  uint32_t                         next_move_id_          = 0;
  uint32_t                         retained_empty_arenas_ = 1;
};

} // namespace ouly
