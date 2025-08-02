// SPDX-License-Identifier: MIT

#pragma once

#include "ouly/scheduler/detail/cache_optimized_data.hpp"
#include "ouly/scheduler/detail/parallel_executer.hpp"
#include "ouly/scheduler/worker_structs.hpp"
#include "ouly/utility/subrange.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <bit> // for std::bit_width
#include <functional>
#include <sys/types.h>
#include <type_traits>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4324) // structure was padded due to alignment specifier
#endif

namespace ouly
{

/**
 * @brief Range pool for storing work ranges with depths (similar to TBB's range_vector)
 */
template <typename Range, uint32_t MaxCapacity = auto_partitioner_traits::range_pool_capacity>
class range_pool
{
  static constexpr uint32_t max_allowed_capacity = 256; // Maximum allowed capacity for the range pool
  static_assert(std::has_single_bit(MaxCapacity) && MaxCapacity > 0 && MaxCapacity <= max_allowed_capacity,
                "MaxCapacity must be a power of two and between 1 and 256");

private:
  uint8_t                          head_{0};
  uint8_t                          tail_{1};
  uint8_t                          size_{1};
  std::array<uint8_t, MaxCapacity> depths_;
  std::array<Range, MaxCapacity>   ranges_;

public:
  explicit range_pool(const Range& initial_range) noexcept
  {
    ranges_[0] = initial_range;
    depths_[0] = 0;
  }

  [[nodiscard]] auto size() const noexcept -> uint8_t
  {
    return size_;
  }
  [[nodiscard]] auto empty() const noexcept -> bool
  {
    return size_ == 0;
  }
  [[nodiscard]] auto full() const noexcept -> bool
  {
    return size_ == MaxCapacity;
  }

  [[nodiscard]] auto front() -> Range&
  {
    return ranges_[head_];
  }
  [[nodiscard]] auto front_depth() const noexcept -> uint8_t
  {
    return depths_[head_];
  }
  [[nodiscard]] auto back() -> Range&
  {
    const uint8_t back_idx = (tail_ - 1) & (MaxCapacity - 1);
    return ranges_[back_idx];
  }
  [[nodiscard]] auto back_depth() const noexcept -> uint8_t
  {
    const uint8_t back_idx = (tail_ - 1) & (MaxCapacity - 1);
    return depths_[back_idx];
  }
  void pop_front()
  {
    if (size_ > 0)
    {
      head_ = (head_ + 1) & (MaxCapacity - 1);
      --size_;
    }
  }

  void push_back(const Range& range, uint8_t depth)
  {
    if (size_ < MaxCapacity)
    {
      ranges_[tail_] = range;
      depths_[tail_] = depth;
      tail_          = (tail_ + 1) & (MaxCapacity - 1);
      ++size_;
    }
  }

  /**
   * @brief Insert a range at the front of the deque
   *
   * Keeping the left‑hand (cache‑warm) part of a split at the back
   * while exposing the right‑hand half at the front lets the current
   * thread continue depth‑first, and idle threads steal breadth‑first.
   */
  void push_front(const Range& range, uint8_t depth)
  {
    if (size_ < MaxCapacity)
    {
      head_          = (head_ - 1) & (MaxCapacity - 1);
      ranges_[head_] = range;
      depths_[head_] = depth;
      ++size_;
    }
  }

  void pop_back()
  {
    if (size_ > 0)
    {
      tail_ = (tail_ - 1) & (MaxCapacity - 1);
      --size_;
    }
  }

  [[nodiscard]] auto is_divisible(uint8_t max_depth, uint64_t granularity) const noexcept -> bool
  {
    if (empty())
    {
      return false;
    }
    const uint8_t back_idx = (tail_ - 1) & (MaxCapacity - 1);
    return ranges_[back_idx].size() > granularity && depths_[back_idx] < max_depth;
  }

  /**
   * @brief Fill the pool by *alternating* between back‑ and front‑splits.
   *
   * ‑ We always keep the **left** (cache‑local) half for the current thread.
   * ‑ The **right** half is inserted on the *opposite* end of the deque so
   *   idle threads can steal it quickly.
   *
   */
  void split_to_fill(uint8_t max_depth, uint64_t granularity)
  {
    bool split_from_back = true; // alternate sides to stay depth‑first on the hot branch

    while (!full() && is_divisible(max_depth, granularity))
    {
      // Choose which end to split this iteration
      const uint8_t idx           = split_from_back ? (tail_ - 1) & (MaxCapacity - 1) : head_;
      Range&        target_range  = ranges_[idx];
      uint8_t       current_depth = depths_[idx];

      // If this range cannot be split, try the other end once; if that also
      // fails we are done.
      if (target_range.size() > granularity && current_depth < max_depth)
      {
        split_from_back       = !split_from_back;
        const uint8_t alt_idx = split_from_back ? (tail_ - 1) & (MaxCapacity - 1) : head_;
        if (alt_idx == idx)
        {
          break;
        }

        Range&  alt_range = ranges_[alt_idx];
        uint8_t alt_depth = depths_[alt_idx];
        if (alt_range.size() < granularity && alt_depth < max_depth)
        {
          break; // neither end is further divisible
        }

        // switch to the alternate end
        target_range  = alt_range;
        current_depth = alt_depth;
      }

      // Perform the split: 'split()' returns the *right* half.
      Range split_range = target_range.split();

      // Update depth for the kept (left) half
      depths_[idx] = current_depth + 1;

      // Expose the right half on the opposite end for stealing
      if (split_from_back)
      {
        push_front(split_range, current_depth + 1);
      }
      else
      {
        push_back(split_range, current_depth + 1);
      }

      // Next iteration starts from the other end
      split_from_back = !split_from_back;
    }
  }
};

/**
 * @brief Auto partition range
 */
template <typename StateData, typename Traits = auto_partitioner_traits>
struct auto_range
{
  using iterator    = typename StateData::iterator;
  using range_type  = typename StateData::range_type;
  using lambda_type = typename StateData::lambda_type;

  StateData* state_{};
  uint32_t   start_{};
  uint32_t   size_{};
  uint16_t   spawn_worker_index_{}; // assuming we have maximum 256 workers
  uint8_t    max_depth_{};
  uint8_t    divisor_log2_{0}; // log2(divisor)

  [[nodiscard]] auto divisor() const noexcept -> uint32_t
  {
    return 1U << divisor_log2_;
  }

  [[nodiscard]] auto span() const noexcept -> range_type
  {
    auto start = state_->first_ + start_;
    return {start, start + size_};
  }

  [[nodiscard]] auto size() const noexcept -> uint32_t
  {
    return size_;
  }

  [[nodiscard]] auto empty() const noexcept -> bool
  {
    return size_ == 0;
  }

  [[nodiscard]] auto is_divisible() const noexcept -> bool
  {
    // A range is divisible only when it still contains at least
    // grain_size * divisor() elements; divisor() == 1 << divisor_log2_.
    return size() > (static_cast<uint64_t>(Traits::grain_size) << divisor_log2_);
  }

  template <TaskContext WC>
  void execute_sequential_auto(WC const& this_context)
  {
    constexpr bool is_range_executor = ouly::detail::RangeExecutor<lambda_type, iterator, WC>;
    auto&          lambda            = state_->lambda_instance_.get();

    if constexpr (is_range_executor)
    {
      auto start = state_->first_ + start_;
      lambda(start, start + size_, this_context);
    }
    else
    {
      const auto start = state_->first_ + start_;
      for (auto it = start, end = start + size_; it != end; ++it)
      {
        if constexpr (std::is_integral_v<std::decay_t<decltype(it)>>)
        {
          lambda(it, this_context);
        }
        else
        {
          lambda(*it, this_context);
        }
      }
    }
  }

  template <TaskContext WC>
  void execute(WC const& this_context)
  {
    // uint32_t group_mask = 1U << this_context.get_group_offset();
    // auto     old_mask   = state_->group_offset_mask_.fetch_or(group_mask, std::memory_order_relaxed);
    // OULY_ASSERT((group_mask & old_mask) == 0);
    uint8_t    max_depth       = max_depth_;
    uint8_t    divisor_log2    = divisor_log2_;
    auto       execution_index = static_cast<uint16_t>(this_context.get_worker().get_index());
    const bool is_stolen       = execution_index != spawn_worker_index_;

    // If the task was stolen, let it split more conservatively:
    //   • raise depth budget a bit
    //   • multiply its divisor by 4   (log2 += 2)
    if (is_stolen)
    {
      if (max_depth < Traits::max_depth)
      {
        max_depth += Traits::depth_increment;
      }

      constexpr uint8_t divisor_increment = 2;  // log2(4)
      constexpr uint8_t max_divisor       = 31; // 2^31 is the maximum divisor we can use
      divisor_log2 = static_cast<uint8_t>(std::min<uint8_t>(divisor_log2 + divisor_increment, max_divisor));
    }

    // Make the new value visible to helpers that call divisor()
    divisor_log2_ = divisor_log2;

    if (!is_divisible() || max_depth == 0)
    {
      execute_sequential_auto(this_context);
      // OULY_ASSERT((group_mask & state_->group_offset_mask_.fetch_and(~group_mask, std::memory_order_relaxed)) != 0);
      return;
    }

    auto                                                range = range_type{start_, start_ + size_};
    range_pool<range_type, Traits::range_pool_capacity> pool(range);

    auto& scheduler = this_context.get_scheduler();

    auto granularity = static_cast<uint64_t>(Traits::grain_size) << divisor_log2_;
    while (!pool.empty())
    {
      // Fill the range pool by splitting ranges
      pool.split_to_fill(max_depth, granularity);

      // Check for work stealing demand
      const bool has_demand = is_stolen || (Traits::grain_size > 1);

      if (has_demand && pool.size() > 1)
      {
        // Offer work to the scheduler - create a new task for the front range
        auto    work_range = pool.front();
        uint8_t work_depth = pool.front_depth();
        pool.pop_front();

        uint8_t child_divisor = divisor_log2 > 0 ? static_cast<uint8_t>(divisor_log2 - 1) : 0;

        state_->spawns_.fetch_add(1, std::memory_order_relaxed);
        // state_->total_spawns_.fetch_add(1, std::memory_order_relaxed);
        scheduler.submit(this_context,
                         [new_range = auto_range{state_, work_range.begin(), static_cast<uint32_t>(work_range.size()),
                                                 execution_index, work_depth, child_divisor}](WC const& wc) mutable
                         {
                           new_range.execute(wc);
                           new_range.state_->spawns_.fetch_sub(1, std::memory_order_release);
                           // new_range.state_->total_executed_.fetch_add(1, std::memory_order_release);
                         });

        // Continue to process remaining ranges in the pool
        continue;
      }

      // Execute the back range sequentially
      auto& back_range = pool.back();
      auto  new_range  = auto_range{state_,          back_range.begin(), static_cast<uint32_t>(back_range.size()),
                                  execution_index, pool.back_depth(),  divisor_log2};
      pool.pop_back();

      new_range.execute_sequential_auto(this_context);

      // Continue processing any remaining ranges
    }
    // OULY_ASSERT((group_mask & state_->group_offset_mask_.fetch_and(~group_mask, std::memory_order_relaxed)) != 0);
  }
};

template <typename FwIt, typename L, typename Traits = auto_partitioner_traits>
struct auto_parallel_for_state
{
  auto_parallel_for_state(L& lambda, FwIt f) noexcept : first_(f), lambda_instance_(lambda) {}

  using range_type  = ouly::subrange<uint32_t>;
  using iterator    = FwIt;
  using lambda_type = L;

  alignas(ouly::detail::cache_line_size) std::atomic<int64_t> spawns_{0};
  // std::atomic<int64_t>      total_spawns_{0};   // Total number of spawned tasks
  // std::atomic<int64_t>      total_executed_{0}; // Total number of executed tasks
  // std::atomic<uint64_t>     group_offset_mask_{0};
  iterator                  first_;
  std::reference_wrapper<L> lambda_instance_;
};

/**
 * @brief Execute lambda sequentially over a range
 */
template <typename L, typename FwIt, TaskContext WC>
void execute_sequential_auto(L& lambda, FwIt&& range, WC const& this_context)
{
  using iterator_t                 = decltype(std::begin(range));
  constexpr bool is_range_executor = ouly::detail::RangeExecutor<L, iterator_t, WC>;

  if constexpr (is_range_executor)
  {
    lambda(std::begin(std::forward<FwIt>(range)), std::end(std::forward<FwIt>(range)), this_context);
  }
  else
  {
    for (auto it = std::begin(std::forward<FwIt>(range)), end = std::end(std::forward<FwIt>(range)); it != end; ++it)
    {
      if constexpr (std::is_integral_v<std::decay_t<decltype(it)>>)
      {
        lambda(it, this_context);
      }
      else
      {
        lambda(*it, this_context);
      }
    }
  }
}

/**
 * @brief Launch auto parallel tasks with adaptive partitioning
 */
template <typename L, typename FwIt, TaskContext WC, typename Traits = auto_partitioner_traits>
void launch_auto_parallel_tasks(L lambda, FwIt&& range, uint32_t initial_divisor, uint32_t count,
                                WC const& this_context)
{
  using iterator_t   = decltype(std::begin(range));
  using state_t      = auto_parallel_for_state<iterator_t, L>;
  using auto_range_t = auto_range<state_t, Traits>;

  auto& scheduler = this_context.get_scheduler();

  // Create shared state for tracking spawned tasks
  state_t state(lambda, std::begin(std::forward<FwIt>(range)));

  // Calculate initial chunk size
  const uint32_t chunk_size = count / initial_divisor;
  const uint32_t remainder  = count % initial_divisor;

  const auto initial_divisor_log2 = static_cast<uint8_t>(std::bit_width(initial_divisor) - 1);

  // Launch parallel tasks
  uint32_t current_pos = 0;
  for (uint32_t i = 0; i < initial_divisor - 1; ++i)
  {
    // Give some tasks one extra element to handle remainder
    const uint32_t current_chunk_size = chunk_size + (i < remainder ? 1 : 0);
    const auto     worker_index       = static_cast<uint16_t>(this_context.get_worker().get_index());

    auto task_range = auto_range_t{&state, current_pos, current_chunk_size, worker_index, 0, initial_divisor_log2};

    state.spawns_.fetch_add(1, std::memory_order_relaxed);
    // state.total_spawns_.fetch_add(1, std::memory_order_relaxed);
    scheduler.submit(this_context,
                     [task_range](WC const& wc) mutable
                     {
                       task_range.execute(wc);
                       task_range.state_->spawns_.fetch_sub(1, std::memory_order_release);
                       // task_range.state_->total_executed_.fetch_add(1, std::memory_order_release);
                     });

    current_pos += current_chunk_size;
  }

  // Execute remaining work in current thread
  if (current_pos < count)
  {
    const uint32_t remaining_size = count - current_pos;
    const auto     worker_index   = static_cast<uint16_t>(this_context.get_worker().get_index());

    auto current_range = auto_range_t{&state, current_pos, remaining_size, worker_index, 0, initial_divisor_log2};

    current_range.execute(this_context);
  }

  // Wait for any additional spawned tasks to complete
  while (state.spawns_.load(std::memory_order_acquire) > 0)
  {
    scheduler.busy_work(this_context);
  }
}

/**
 * @brief Auto parallel_for implementation with adaptive partitioning
 *
 * This implementation uses TBB-style auto partitioning that adapts to load imbalances
 * and work stealing patterns for optimal performance across different workloads.
 */
template <typename L, typename FwIt, TaskContext WC, typename Traits = auto_partitioner_traits>
void auto_parallel_for(L lambda, FwIt&& range, WC const& this_context, Traits /*traits*/ = {})
{
  using it_helper = ouly::detail::it_size_type<FwIt>;
  using size_type = uint32_t;

  size_type count = it_helper::size(range);

  if (count <= Traits::sequential_threshold)
  {
    execute_sequential_auto(lambda, std::forward<FwIt>(range), this_context);
    return;
  }

  // Auto partitioner: calculate initial divisor based on concurrency and work characteristics
  const uint32_t available_workers = this_context.get_scheduler().get_worker_count(this_context.get_workgroup());
  const uint32_t initial_divisor   = std::min(available_workers * Traits::grain_size, count / Traits::grain_size);

  if (initial_divisor <= 1)
  {
    execute_sequential_auto(lambda, std::forward<FwIt>(range), this_context);
    return;
  }

  launch_auto_parallel_tasks(lambda, std::forward<FwIt>(range), initial_divisor, count, this_context);
}

} // namespace ouly

#ifdef _MSC_VER
#pragma warning(pop)
#endif
