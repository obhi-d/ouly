// SPDX-License-Identifier: MIT

#pragma once

#include "ouly/allocators/alignment.hpp"
#include "ouly/allocators/allocation_id.hpp"
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
 * @brief Identifier for a GPU relocation operation.
 *
 * Like `allocation_id`, relocation ids are 32-bit table/stream identifiers with the maximum value
 * reserved as the null value.
 */
struct gpu_move_id
{
  uint32_t id_ = std::numeric_limits<uint32_t>::max();

  [[nodiscard]] auto get() const noexcept -> uint32_t
  {
    return id_;
  }

  auto operator<=>(gpu_move_id const&) const noexcept = default;
};

using gpu_timeline_value = uint64_t;

/** @brief Runtime properties of a GPU suballocation. */
struct gpu_allocation_options
{
  allocation_size_type alignment_      = 1;
  uint32_t             memory_class_   = 0;
  uint32_t             resource_class_ = 0;
  bool                 movable_        = true;
  bool                 dedicated_      = false;
};

/** @brief Result returned by `gpu_allocator::allocate`. */
struct gpu_allocation
{
  allocation_size_type offset_ = 0;
  allocation_id        id_;
  arena_id             arena_;

  auto operator<=>(gpu_allocation const&) const noexcept = default;

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

/** @brief A direct GPU relocation submitted by the API-specific executor. */
struct gpu_relocation
{
  gpu_move_id          move_;
  allocation_id        allocation_;
  arena_id             source_arena_;
  arena_id             destination_arena_;
  allocation_size_type source_offset_      = 0;
  allocation_size_type destination_offset_ = 0;
  allocation_size_type size_               = 0;
  uint32_t             memory_class_       = 0;
  uint32_t             resource_class_     = 0;
};

/** @brief Per-call limits for incremental GPU defragmentation. */
struct gpu_defrag_budget
{
  allocation_size_type max_copy_bytes_     = std::numeric_limits<allocation_size_type>::max();
  uint32_t             max_move_count_     = std::numeric_limits<uint32_t>::max();
  uint32_t             max_blocks_scanned_ = ouly::cfg::default_gpu_block_to_scan;
};

/** @brief Work performed by one defragmentation/completion-processing call. */
struct gpu_defrag_result
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

/** @brief Lifetime totals reported by `gpu_allocator::statistics`. */
struct gpu_defrag_statistics
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
concept GpuMemoryManagerWithClass = requires(T manager) {
  { manager.add(arena_id(), allocation_size_type(), uint32_t()) } -> std::same_as<void>;
};

template <typename T>
concept GpuMemoryManagerWithoutClass = requires(T manager) {
  { manager.add(arena_id(), allocation_size_type()) } -> std::same_as<void>;
};

/**
 * @brief Backing-memory owner used by `gpu_allocator`.
 *
 * `add` creates an API heap/buffer for an arena. Managers that distinguish memory classes accept
 * the three-argument form; simple managers may use the two-argument form. `remove` destroys it.
 */
template <typename T>
concept GpuMemoryManager = (GpuMemoryManagerWithClass<T> || GpuMemoryManagerWithoutClass<T>) && requires(T manager) {
  { manager.remove(arena_id()) } -> std::same_as<void>;
};

/**
 * @brief API-specific asynchronous relocation executor.
 *
 * `schedule_copy` returns the timeline value at which the destination contains the copied bytes.
 * `switch_references` runs after that point, updates future users of the stable allocation id, and
 * returns the timeline value after which old source users can no longer access the source.
 */
template <typename T>
concept GpuRelocationExecutor = requires(T executor, gpu_relocation const& relocation, gpu_timeline_value value) {
  { executor.schedule_copy(relocation) } -> std::same_as<std::optional<gpu_timeline_value>>;
  { executor.switch_references(relocation) } -> std::same_as<std::optional<gpu_timeline_value>>;
  { executor.is_complete(value) } -> std::convertible_to<bool>;
};

/**
 * @brief Scratch allocator `gpu_allocator::defragment` falls back to when the caller supplies none.
 *
 * Arenas are created lazily, so a call that schedules no new evacuation does not allocate at all.
 */
using gpu_defrag_scratch = ouly::linear_stack_allocator<>;

/**
 * @brief Arena-based GPU suballocator with budgeted asynchronous whole-arena evacuation.
 *
 * The allocator manages virtual placements only. A `GpuMemoryManager` owns backing API resources,
 * and a `GpuRelocationExecutor` records copies, rebinds stable allocation handles, and supplies
 * timeline completion information. Both are used through concepts: there are no virtual interfaces
 * or type-erased callbacks on the allocation path. Evacuation planning needs temporary storage, and
 * the caller can hand any `ScratchAllocator` (a frame allocator, for instance) to `defragment` to
 * keep that storage out of the heap.
 *
 * Defragmentation deliberately evacuates one releasable arena at a time. Before an arena is frozen,
 * every destination is simulated with alignment-aware best fit and then reserved. Copies may then be
 * submitted across many calls under byte and move-count budgets without risking a partially evacuated
 * arena that cannot be completed.
 *
 * Dedicated and explicitly non-movable allocations are pinned. New allocations never enter an active
 * evacuation source. The public `allocation_id` remains stable while its arena and offset change only
 * after `switch_references` succeeds.
 *
 * @note The class is not internally synchronized. Allocation, completion processing, and
 * defragmentation must be externally serialized.
 */
class gpu_allocator
{
public:
  using size_type = allocation_size_type;

  OULY_API gpu_allocator();
  OULY_API explicit gpu_allocator(size_type arena_size);

  OULY_API void set_arena_size(size_type size) noexcept;

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

  template <GpuMemoryManager M>
  auto allocate(size_type size, M& manager, gpu_allocation_options options) -> gpu_allocation
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
    auto allocation    = try_allocate_in_arena(arena.get(), size, options);
    OULY_ASSERT(allocation.id_ != allocation_id());
    return allocation;
  }

  template <GpuMemoryManager M, typename Alignment = ouly::alignment<>, typename Dedicated = std::false_type>
    requires requires(Alignment value) {
      { static_cast<size_type>(value) } -> std::same_as<size_type>;
      { Dedicated::value } -> std::convertible_to<bool>;
    }
  auto allocate(size_type size, M& manager, Alignment alignment = {}, Dedicated /*unused*/ = {}) -> gpu_allocation
  {
    return allocate(size, manager,
                    gpu_allocation_options{.alignment_ = static_cast<size_type>(alignment),
                                           .dedicated_ = static_cast<bool>(Dedicated::value)});
  }

  /**
   * @brief Release an allocation.
   *
   * A copy-scheduled allocation cannot be released until its completion has been processed because
   * the executor still owns its source/destination transition. `try_deallocate` reports this case;
   * `deallocate` asserts that it is safe.
   */
  template <GpuMemoryManager M>
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

  template <GpuMemoryManager M>
  void deallocate(allocation_id id, M& manager)
  {
    [[maybe_unused]] bool const released = try_deallocate(id, manager);
    OULY_ASSERT(released && "cannot deallocate while a GPU relocation copy is pending");
  }

  /**
   * @brief Process completed copies/retirements and schedule more evacuation work under `budget`.
   *
   * Planning an evacuation needs temporary storage proportional to the number of arenas and their
   * free ranges. It is taken from a call-local `gpu_defrag_scratch`; use the overload taking a
   * `ScratchAllocator` to draw it from a frame allocator instead.
   */
  template <GpuMemoryManager M, GpuRelocationExecutor E>
  auto defragment(M& manager, E& executor, gpu_defrag_budget budget = {}) -> gpu_defrag_result
  {
    gpu_defrag_scratch scratch{ouly::cfg::default_gpu_scratch_size};
    return defragment(manager, executor, budget, scratch);
  }

  /**
   * @brief Process completed copies/retirements and schedule more evacuation work under `budget`.
   *
   * @param scratch Allocator the evacuation planner takes its temporary buffers from. Nothing is
   * allocated from it unless a new evacuation is planned, and every buffer is dead by the time the
   * call returns, so a linear allocator that is rewound per frame is the expected choice.
   */
  template <GpuMemoryManager M, GpuRelocationExecutor E, ScratchAllocator S>
  auto defragment(M& manager, E& executor, gpu_defrag_budget budget, S& scratch) -> gpu_defrag_result
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
  template <GpuMemoryManager M, GpuRelocationExecutor E>
  auto process_completions(M& manager, E& executor) -> gpu_defrag_result
  {
    gpu_defrag_result result;
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
        move.state_ = move_state::source_retired;
        ++result.moves_completed_;
        ++statistics_.moves_completed_;
      }
    }

    finish_target_if_possible(manager, &result);
    set_result_state(result);
    return result;
  }

  [[nodiscard]] OULY_API auto statistics() const noexcept -> gpu_defrag_statistics;

  OULY_API void validate_integrity() const;

private:
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

  struct free_range
  {
    size_type offset_ = 0;
    size_type size_   = 0;
  };

  struct arena_state
  {
    std::vector<free_range> free_ranges_;
    size_type               capacity_     = 0;
    size_type               free_         = 0;
    uint32_t                allocations_  = 0;
    uint32_t                reservations_ = 0;
    uint32_t                retiring_     = 0;
    uint32_t                memory_class_ = 0;
    arena_role              role_         = arena_role::normal;
    bool                    active_       = false;
    bool                    dedicated_    = false;
  };

  struct planned_move
  {
    gpu_move_id        move_;
    allocation_id      allocation_;
    uint16_t           source_arena_       = 0;
    uint16_t           destination_arena_  = 0;
    size_type          source_offset_      = 0;
    size_type          destination_offset_ = 0;
    size_type          size_               = 0;
    move_state         state_              = move_state::reserved;
    gpu_timeline_value copy_completion_    = 0;
    gpu_timeline_value source_retirement_  = 0;
  };

  struct evacuation_target
  {
    uint16_t                  source_arena_ = 0;
    std::vector<planned_move> moves_;
    bool                      aborted_ = false;
  };

  /** @brief What an arena currently holds, as seen by the evacuation planner */
  struct arena_usage
  {
    size_type live_   = 0;
    uint32_t  count_  = 0;
    bool      pinned_ = false;
  };

  /** @brief An arena that may be evacuated, ranked by how much it would recover */
  struct evacuation_candidate
  {
    uint16_t  arena_    = 0;
    size_type capacity_ = 0;
    size_type live_     = 0;
    uint32_t  count_    = 0;
  };

  /** @brief A destination arena as the planner sees it while it places moves one by one */
  struct simulated_arena
  {
    uint16_t                         arena_ = 0;
    size_type                        free_  = 0;
    ouly::scratch_vector<free_range> ranges_;
  };

  /** @brief Where a move would land, or a null arena when nothing fits */
  struct simulated_fit
  {
    simulated_arena* arena_  = nullptr;
    size_t           range_  = 0;
    size_type        offset_ = 0;
  };

  template <GpuMemoryManager M>
  static void notify_add(M& manager, arena_id arena, size_type size, uint32_t memory_class)
  {
    if constexpr (GpuMemoryManagerWithClass<M>)
    {
      manager.add(arena, size, memory_class);
    }
    else
    {
      manager.add(arena, size);
    }
  }

  template <GpuMemoryManager M>
  void try_release_arena(M& manager, uint16_t arena)
  {
    if (can_release_arena(arena))
    {
      drop_arena(arena);
      manager.remove(arena_id{arena});
    }
  }

  template <GpuMemoryManager M>
  void finish_target_if_possible(M& manager, gpu_defrag_result* result = nullptr)
  {
    if (!target_ || !target_finished())
    {
      return;
    }

    auto const source                                      = target_->source_arena_;
    ouly::detail::vector_access(arena_pool_, source).role_ = arena_role::normal;
    target_.reset();

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

  OULY_API auto try_allocate(size_type size, gpu_allocation_options const& options) -> gpu_allocation;
  OULY_API auto try_allocate_in_arena(uint16_t arena, size_type size, gpu_allocation_options const& options)
   -> gpu_allocation;
  OULY_API auto create_arena(size_type capacity, uint32_t memory_class, bool dedicated) -> arena_id;
  OULY_API auto push_allocation(uint16_t arena, size_type offset, size_type size, gpu_allocation_options const& options)
   -> allocation_id;
  OULY_API auto release_allocation(allocation_id id) -> uint16_t;
  OULY_API auto prepare_deallocate(allocation_id id) -> bool;

  [[nodiscard]] OULY_API auto        measure_arena(uint16_t arena) const -> arena_usage;
  [[nodiscard]] OULY_API auto        can_evacuate(uint16_t arena) const -> bool;
  [[nodiscard]] OULY_API static auto recovers_more(evacuation_candidate const& lhs, evacuation_candidate const& rhs)
   -> bool;
  OULY_API auto               reserve_plan(evacuation_target& plan) -> bool;
  OULY_API void               begin_evacuation(uint32_t max_blocks_scanned, scratch_allocator_ref scratch);
  [[nodiscard]] OULY_API auto simulate_evacuation(uint16_t source, scratch_allocator_ref scratch) const
   -> std::optional<evacuation_target>;
  [[nodiscard]] OULY_API auto collect_destinations(uint16_t source, size_t move_count,
                                                   scratch_allocator_ref scratch) const
   -> ouly::scratch_vector<simulated_arena>;
  [[nodiscard]] OULY_API static auto find_best_fit(ouly::scratch_vector<simulated_arena>& destinations, size_type size,
                                                   size_type alignment) -> simulated_fit;
  OULY_API static void occupy(simulated_arena& destination, size_t range_index, size_type offset, size_type size);
  OULY_API auto        reserve_range(uint16_t arena, size_type offset, size_type size) -> bool;
  OULY_API void        release_reserved_range(uint16_t arena, size_type offset, size_type size);
  OULY_API void        free_range_in_arena(uint16_t arena, size_type offset, size_type size);
  OULY_API void        cancel_move(planned_move& move);
  OULY_API void        abort_evacuation();
  [[nodiscard]] OULY_API auto find_schedulable_move(size_type remaining_bytes) -> planned_move*;
  [[nodiscard]] OULY_API auto has_reserved_moves() const -> bool;
  [[nodiscard]] OULY_API auto target_finished() const -> bool;
  [[nodiscard]] OULY_API auto make_relocation(planned_move const& move) const -> gpu_relocation;
  OULY_API void               switch_allocation(planned_move const& move);
  OULY_API void               retire_source(planned_move const& move);

  [[nodiscard]] OULY_API auto can_release_arena(uint16_t arena) const -> bool;
  OULY_API auto               drop_arena(uint16_t arena) -> size_type;
  OULY_API void               set_result_state(gpu_defrag_result& result) const;
  [[nodiscard]] OULY_API auto allocate_move_id() -> gpu_move_id;

  std::vector<arena_state> arena_pool_ = {arena_state{}};
  std::vector<uint16_t>    arena_order_;
  std::vector<uint16_t>    free_arenas_;

  std::vector<size_type> entry_offsets_          = {0};
  std::vector<size_type> entry_sizes_            = {0};
  std::vector<uint16_t>  entry_arenas_           = {0};
  std::vector<uint8_t>   entry_alignments_       = {0};
  std::vector<uint32_t>  entry_memory_classes_   = {0};
  std::vector<uint32_t>  entry_resource_classes_ = {0};
  std::vector<bool>      entry_live_             = {false};
  std::vector<bool>      entry_movable_          = {false};
  uint32_t               free_entry_             = 0;

  std::optional<evacuation_target> target_;
  gpu_defrag_statistics            statistics_;
  size_type                        arena_size_   = 0;
  uint32_t                         next_move_id_ = 0;
};

} // namespace ouly
