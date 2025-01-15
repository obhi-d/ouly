
#pragma once

#include <acl/scheduler/detail/parallel_executer.hpp>
#include <acl/utility/integer_range.hpp>
#include <acl/utility/type_traits.hpp>
#include <functional>
#include <latch>
#include <type_traits>

namespace acl
{

/**
 * @brief A parallel execution utility for processing ranges of data across multiple workers
 *
 * @namespace acl
 *
 * This namespace contains implementations for parallel task execution, particularly the parallel_for
 * utility which distributes work across multiple workers in a work group.
 *
 * Key Components:
 *
 * - default_task_traits: Configuration struct for controlling parallel execution behavior
 *   - batches_per_worker: Controls granularity of work distribution
 *   - parallel_execution_threshold: Minimum task count for parallel execution
 *   - fixed_batch_size: Optional override for batch size
 *
 * - parallel_for: Main interface for parallel execution
 *   Supports two types of lambda functions:
 *   1. Range-based: void(Iterator begin, Iterator end, worker_context const& context)
 *   2. Element-based: void(T& element, worker_context const& context)
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
 * @see default_task_traits For customizing execution behavior
 * @see worker_context For execution context details
 * @see scheduler For task scheduling implementation
 */

template <typename Iterator, typename L>
struct parallel_for_data
{
  parallel_for_data(L& lambda, Iterator f, uint32_t task_count) noexcept
      : lambda_instance_(lambda), first_(f), counter_(static_cast<ptrdiff_t>(task_count))
  {}

  Iterator                  first_;
  std::latch                counter_;
  std::reference_wrapper<L> lambda_instance_;
};

template <typename L>
void launch_parallel_tasks(L& lambda, auto range, uint32_t work_count, uint32_t fixed_batch_size, uint32_t count,
                           worker_context const& this_context)
{
  auto& scheduler = this_context.get_scheduler();

  using iterator_t                                   = decltype(std::begin(range));
  using size_type                                    = uint32_t;
  constexpr bool                   is_range_executor = acl::detail::RangeExcuter<L, iterator_t>;
  parallel_for_data<iterator_t, L> pfor_instance(lambda, std::begin(range), work_count - 1);
  size_type                        begin = 0;
  for (size_type i = 1; i < work_count; ++i)
  {
    auto next = std::min(begin + fixed_batch_size, count);
    scheduler.submit(this_context.get_worker(), this_context.get_workgroup(),
                     [instance = &pfor_instance, start = begin, end = next](worker_context const& wc)
                     {
                       if constexpr (acl::detail::RangeExcuter<L, iterator_t>)
                       {
                         instance->lambda_instance_.get()(instance->first_ + start, instance->first_ + end, wc);
                       }
                       else
                       {
                         if constexpr (std::is_integral_v<std::decay_t<decltype(instance->first_)>>)
                         {
                           instance->lambda_instance_.get()((instance->first_ + start), wc);
                         }
                         else
                         {
                           instance->lambda_instance_.get()(*(instance->first_ + start), wc);
                         }
                       }
                       instance->counter_.count_down();
                     });
    begin = next;
  }

  // Work before wait
  {
    auto next = std::min(begin + fixed_batch_size, count);
    if constexpr (is_range_executor)
    {
      lambda(std::begin(range) + begin, std::begin(range) + next, this_context);
    }
    else
    {
      for (auto it = std::begin(range) + begin, end = std::begin(range) + next; it != end; ++it)
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

  pfor_instance.counter_.wait();
}

template <typename L, typename FwIt, typename TaskTr = default_task_traits>
void parallel_for(L lambda, FwIt range, worker_context const& this_context, TaskTr /*unused*/ = {})
{
  using iterator_t                 = decltype(std::begin(range));
  constexpr bool is_range_executor = acl::detail::RangeExcuter<L, iterator_t>;
  using it_helper                  = acl::detail::it_size_type<FwIt>;
  using size_type                  = uint32_t; // Range is limited
  using traits                     = acl::detail::final_task_traits<TaskTr>;

  size_type count = it_helper::size(range);

  constexpr uint32_t min_batches_per_worker = 1;
  const size_type    work_count             = [&]()
  {
    if (!is_range_executor)
    {
      return count;
    }
    if (traits::fixed_batch_size)
    {
      return (count + traits::fixed_batch_size - 1) / traits::fixed_batch_size;
    }
    return acl::detail::get_work_count(std::max(min_batches_per_worker, traits::batches_per_worker),
                                       this_context.get_scheduler().get_worker_count(this_context.get_workgroup()),
                                       count);
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
    launch_parallel_tasks(lambda, range, work_count, fixed_batch_size, count, this_context);
  }
}

/**
 *
 * Call this method with either of these lambda functions:
 * ```cpp
 *   lambda(It begin, It end, acl::worker_context const& context);
 *   lambda(Ty& value_type, acl::worker_context const& context);
 * ```
 * Depending upon the passed function parameter type, either a batch executor, or a single instance executer will be
 * called.
 *
 * Use TaskTraits to modify the behavior of the execution. Check @class default_task_traits
 *
 */
template <typename L, typename FwIt, typename TaskTraits = default_task_traits>
void parallel_for(L&& lambda, FwIt range, worker_id current, workgroup_id workgroup, scheduler& s, TaskTraits tt = {})
{
  auto const& this_context = s.get_context(current, workgroup);
  // Assert this context belongs to the work group selected for submission
  assert(
   this_context.belongs_to(workgroup) &&
   "Current worker does not belong to the work group for 'parallel_for' submission and thus cannot execute the task.");
  parallel_for(std::forward<L>(lambda), range, this_context, tt);
}

template <typename L, typename FwIt, typename TaskTraits = default_task_traits>
void parallel_for(L&& lambda, FwIt range, workgroup_id workgroup, TaskTraits tt = default_task_traits{})
{
  auto const& this_context = worker_context::get(workgroup);
  parallel_for(std::forward<L>(lambda), range, this_context, tt);
}

} // namespace acl
