// SPDX-License-Identifier: MIT

#pragma once

#include "ouly/scheduler/detail/parallel_executer.hpp"
#include "ouly/scheduler/worker_structs.hpp"
#include "ouly/utility/subrange.hpp"
#include "ouly/utility/type_traits.hpp"
#include <functional>
#include <latch>
#include <type_traits>

namespace ouly
{

/**
 * @brief A parallel execution utility for processing ranges of data across multiple workers
 *
 * @namespace ouly
 *
 * This namespace contains implementations for parallel task execution, particularly the parallel_for
 * utility which distributes work across multiple workers in a work group.
 *
 * Key Components:
 *
 * - default_partitioner_traits: Configuration struct for controlling parallel execution behavior
 *   - batches_per_worker: Controls granularity of work distribution
 *   - parallel_execution_threshold: Minimum task count for parallel execution
 *   - fixed_batch_size: Optional override for batch size
 *
 * - parallel_for: Main interface for parallel execution
 *   Supports two types of lambda functions:
 *   1. Range-based: void(Iterator begin, Iterator end, task_context const& context)
 *   2. Element-based: void(T& element, task_context const& context)
 *
 * Usage Examples:
 * @code
 *   // Range-based execution
 *   parallel_for([](auto begin, auto end, auto& ctx) {
 *       // Process range [begin, end)
 *   }, container, workgroup_id);
 *
 *   // Element-based execution
 *   parallel_for([](auto& element, auto& ctx) {
 *       // Process single element
 *   }, container, workgroup_id);
 * @endcode
 *
 * The implementation automatically:
 * - Determines optimal batch sizes based on task traits
 * - Handles task distribution across available workers
 * - Manages synchronization using std::latch
 * - Falls back to sequential execution for small ranges
 *
 * @note The parallel execution is only triggered if the task count exceeds
 *       parallel_execution_threshold and can be effectively parallelized
 *
 * @see default_partitioner_traits For customizing execution behavior
 * @see task_context For execution context details
 * @see scheduler For task scheduling implementation
 */

template <typename Iterator, typename L>
struct parallel_for_data
{
  parallel_for_data(L& lambda, Iterator f, uint32_t task_count) noexcept
      : first_(f), counter_(static_cast<ptrdiff_t>(task_count)), lambda_instance_(lambda)
  {}

  Iterator                  first_;
  std::latch                counter_;
  std::reference_wrapper<L> lambda_instance_;
};

template <typename L, TaskContext WC>
void execute_sequential(L& lambda, auto&& range, WC const& this_context);

template <TaskContext WC, typename L>
void execute_remaining_work(L& lambda, auto&& range, uint32_t current_pos, uint32_t count, WC const& this_context);

template <TaskContext WC>
inline void cooperative_wait(std::latch& counter, WC const& this_context);

template <typename L, TaskContext WC>
void launch_parallel_tasks(L& lambda, auto&& range, uint32_t work_count, uint32_t /*fixed_batch_size*/, uint32_t count,
                           WC const& this_context)
{
  auto& scheduler = this_context.get_scheduler();

  using iterator_t = decltype(std::begin(range));

  // Optimized work distribution: ensure current thread gets meaningful work
  // and reduce the number of parallel tasks to prevent over-subscription
  const uint32_t available_workers    = scheduler.get_worker_count(this_context.get_workgroup());
  const uint32_t effective_work_count = std::min(work_count, available_workers);
  const uint32_t parallel_tasks       = effective_work_count > 1 ? effective_work_count - 1 : 0;

  if (parallel_tasks == 0)
  {
    execute_sequential(lambda, std::forward<decltype(range)>(range), this_context);
    return;
  }

  parallel_for_data<iterator_t, L> pfor_instance(lambda, std::begin(range), parallel_tasks);

  // Better work distribution: ensure each task gets roughly equal work
  uint32_t current_pos = submit_parallel_tasks(pfor_instance, effective_work_count, count, this_context);

  // Current thread processes the remaining work
  execute_remaining_work(lambda, std::forward<decltype(range)>(range), current_pos, count, this_context);

  // Cooperative wait: instead of blocking, yield to process other work
  cooperative_wait(pfor_instance.counter_, this_context);
}

template <typename L, TaskContext WC>
void execute_sequential(L& lambda, auto&& range, WC const& this_context)
{
  using iterator_t                 = decltype(std::begin(range));
  constexpr bool is_range_executor = ouly::detail::RangeExecutor<L, iterator_t, WC>;

  if constexpr (is_range_executor)
  {
    lambda(std::begin(range), std::end(range), this_context);
  }
  else
  {
    for (auto it = std::begin(range), end = std::end(range); it != end; ++it)
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

template <typename L, typename Iterator, TaskContext WC>
auto submit_parallel_tasks(parallel_for_data<Iterator, L>& pfor_instance, uint32_t effective_work_count, uint32_t count,
                           WC const& this_context) -> uint32_t
{
  auto&          scheduler          = this_context.get_scheduler();
  const uint32_t parallel_tasks     = effective_work_count - 1;
  const uint32_t base_work_per_task = count / effective_work_count;
  const uint32_t extra_work         = count % effective_work_count;

  uint32_t current_pos = 0;
  for (uint32_t i = 0; i < parallel_tasks; ++i)
  {
    // Give some tasks one extra element to handle remainder
    const uint32_t current_task_work = base_work_per_task + (i < extra_work ? 1 : 0);
    const uint32_t task_end          = current_pos + current_task_work;

    scheduler.submit(this_context, create_task_lambda<WC>(pfor_instance, current_pos, task_end));
    current_pos = task_end;
  }
  return current_pos;
}

template <TaskContext WC, typename L, typename Iterator>
auto create_task_lambda(parallel_for_data<Iterator, L>& pfor_instance, uint32_t start, uint32_t end)
{
  return [instance = &pfor_instance, start, end](WC const& wc)
  {
    using iterator_t = Iterator;
    if constexpr (ouly::detail::RangeExecutor<L, iterator_t, WC>)
    {
      instance->lambda_instance_.get()(instance->first_ + start, instance->first_ + end, wc);
    }
    else
    {
      for (auto pos = start; pos < end; ++pos)
      {
        if constexpr (std::is_integral_v<std::decay_t<decltype(instance->first_)>>)
        {
          instance->lambda_instance_.get()((instance->first_ + pos), wc);
        }
        else
        {
          instance->lambda_instance_.get()(*(instance->first_ + pos), wc);
        }
      }
    }
    instance->counter_.count_down();
  };
}

template <TaskContext WC, typename L>
void execute_remaining_work(L& lambda, auto&& range, uint32_t current_pos, uint32_t count, WC const& this_context)
{
  using iterator_t                 = decltype(std::begin(range));
  constexpr bool is_range_executor = ouly::detail::RangeExecutor<L, iterator_t, WC>;

  const uint32_t remaining_work = count - current_pos;
  if (remaining_work > 0)
  {
    if constexpr (is_range_executor)
    {
      lambda(std::begin(range) + current_pos, std::begin(range) + count, this_context);
    }
    else
    {
      for (auto it = std::begin(range) + current_pos, end = std::begin(range) + count; it != end; ++it)
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
}

template <TaskContext WC>
inline void cooperative_wait(std::latch& counter, WC const& this_context)
{
  auto& scheduler = this_context.get_scheduler();
  while (!counter.try_wait())
  {
    // Try to do other work while waiting for parallel tasks to complete
    scheduler.busy_work(this_context);
  }
}

template <typename L, typename FwIt, TaskContext WC, typename TaskTr = default_partitioner_traits>
void default_parallel_for(L lambda, FwIt&& range, WC const& this_context, TaskTr /*unused*/ = {})
{
  using iterator_t                 = decltype(std::begin(range));
  constexpr bool is_range_executor = ouly::detail::RangeExecutor<L, iterator_t, WC>;
  using it_helper                  = ouly::detail::it_size_type<FwIt>;
  using size_type                  = uint32_t; // Range is limited
  using traits                     = ouly::detail::final_task_traits<TaskTr>;

  size_type count = it_helper::size(range);

  const size_type work_count = [&]()
  {
    if (!is_range_executor)
    {
      return count;
    }
    if constexpr (traits::fixed_batch_size > 0)
    {
      return (count + traits::fixed_batch_size - 1) / traits::fixed_batch_size;
    }
    else
    {
      constexpr uint32_t min_batches_per_worker = 1;
      return ouly::detail::get_work_count(std::max(min_batches_per_worker, traits::batches_per_worker),
                                          this_context.get_scheduler().get_worker_count(this_context.get_workgroup()),
                                          count);
    }
  }();

  const size_type fixed_batch_size = [&]()
  {
    if (!is_range_executor)
    {
      return size_type{1};
    }
    if (traits::fixed_batch_size)
    {
      return traits::fixed_batch_size;
    }
    return (count + work_count - 1) / work_count;
  }();

  if (count <= traits::parallel_execution_threshold || work_count <= 1)
  {
    if constexpr (is_range_executor)
    {
      lambda(std::begin(range), std::end(range), this_context);
    }
    else
    {
      for (auto it = std::begin(range), end = std::end(range); it != end; ++it)
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
  else
  {
    launch_parallel_tasks(lambda, std::forward<FwIt>(range), work_count, fixed_batch_size, count, this_context);
  }
}

} // namespace ouly
