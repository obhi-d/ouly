
#pragma once

#include "scheduler.hpp"
#include <acl/utils/integer_range.hpp>
#include <acl/utils/type_traits.hpp>
#include <iterator>
#include <latch>
#include <type_traits>

namespace acl
{

struct default_task_traits
{
  /**
   * Relevant for ranged executers, this value determines the number of batches dispatched per worker on average. Higher
   * value means the individual task batches are smaller.
   */
  static constexpr uint32_t batches_per_worker = 4;
  /**
   * This value is used as the minimum task count that will fire the parallel executer, if the task count is less than
   * this value, a for loop is executed instead.
   */
  static constexpr uint32_t parallel_execution_threshold = 16;
  /**
   * This value, if set to non-zero, would override the `batches_per_worker` value and instead be used as the batch size
   * for the tasks.
   */
  static constexpr uint32_t fixed_batch_size = 0;
};

namespace detail
{

template <typename L, typename It>
concept RangeExcuter = requires(L l, It range, worker_context const& wc) { l(range, range, wc); };
template <typename I>
concept HasIteratorDiff = requires(I s) {
  {
    s.size()
  } -> std::integral;
};

template <typename T>
concept HasIteratorTraits = requires() {
  typename std::iterator_traits<T>::difference_type;
  typename std::iterator_traits<T>::value_type;
  typename std::iterator_traits<T>::pointer;
  typename std::iterator_traits<T>::reference;
  typename std::iterator_traits<T>::iterator_category;
};

template <typename T>
struct it_size_type;

template <HasIteratorDiff T>
struct it_size_type<T>
{
  inline static uint32_t size(T const& range) noexcept
  {
    return static_cast<uint32_t>(range.size());
  }
};

template <HasIteratorTraits T>
struct it_size_type<T>
{
  using type = typename std::iterator_traits<T>::difference_type;

  inline static uint32_t size(T const& range) noexcept
  {
    return static_cast<uint32_t>(std::distance(std::begin(range), std::end(range)));
  }
};

// Concept to check if a type has 'fixed_batch_size'
template <typename T>
concept HasFixedBatchSize = requires {
  {
    T::fixed_batch_size
  } -> std::convertible_to<uint32_t>;
};

// Concept to check if a type has 'batches_per_worker'
template <typename T>
concept HasBatchesPerWorker = requires {
  {
    T::batches_per_worker
  } -> std::convertible_to<uint32_t>;
};

// Concept to check if a type has 'parallel_execution_threshold'
template <typename T>
concept HasParallelExecutionThreshold = requires {
  {
    T::parallel_execution_threshold
  } -> std::convertible_to<uint32_t>;
};

template <typename T>
struct fixed_batch_size_t
{
  static constexpr uint32_t value = default_task_traits::fixed_batch_size;
};

template <HasFixedBatchSize T>
struct fixed_batch_size_t<T>
{
  static constexpr uint32_t value = T::fixed_batch_size;
};

template <typename T>
struct batches_per_worker_t
{
  static constexpr uint32_t value = default_task_traits::batches_per_worker;
};

template <HasBatchesPerWorker T>
struct batches_per_worker_t<T>
{
  static constexpr uint32_t value = T::batches_per_worker;
};

template <typename T>
struct parallel_execution_threshold_t
{
  static constexpr uint32_t value = default_task_traits::parallel_execution_threshold;
};

template <HasParallelExecutionThreshold T>
struct parallel_execution_threshold_t<T>
{
  static constexpr uint32_t value = T::parallel_execution_threshold;
};

template <typename Traits>
struct final_task_traits
{
  static constexpr uint32_t fixed_batch_size = fixed_batch_size_t<Traits>::value;

  static constexpr uint32_t batches_per_worker = batches_per_worker_t<Traits>::value;

  static constexpr uint32_t parallel_execution_threshold = parallel_execution_threshold_t<Traits>::value;
};

inline constexpr uint32_t get_work_count(uint32_t batches_per_wk, uint32_t wk_count, uint32_t tk_count)
{
  uint32_t batch_count = wk_count * batches_per_wk;
  return (tk_count + batch_count - 1) / batch_count;
}

} // namespace detail

template <typename L, typename FwIt, typename TaskTr = default_task_traits>
void parallel_for(L&& lambda, FwIt range, worker_context const& this_context, TaskTr = {})
{
  using iterator_t                 = decltype(std::begin(range));
  constexpr bool is_range_executor = detail::RangeExcuter<L, iterator_t>;
  using it_helper                  = detail::it_size_type<FwIt>;
  using size_type                  = uint32_t; // Range is limited
  using traits                     = detail::final_task_traits<TaskTr>;

  struct parallel_for_data
  {
    parallel_for_data(L& lambda, iterator_t f, size_type task_count) noexcept
        : lambda_instance(lambda), first(f), counter(static_cast<ptrdiff_t>(task_count))
    {}

    iterator_t first;
    std::latch counter;
    L&         lambda_instance;
  };

  auto&     s     = this_context.get_scheduler();
  size_type count = it_helper::size(range);

  constexpr uint32_t min_batches_per_worker = 1;
  const size_type    work_count =
    is_range_executor
         ? (traits::fixed_batch_size ? (count + traits::fixed_batch_size - 1) / traits::fixed_batch_size
                                     : detail::get_work_count(std::max(min_batches_per_worker, traits::batches_per_worker),
                                                              s.get_worker_count(this_context.get_workgroup()), count))
         : count;

  const size_type fixed_batch_size =
    is_range_executor ? (traits::fixed_batch_size ? traits::fixed_batch_size : ((count + work_count - 1) / work_count))
                      : 1;

  if (count <= traits::parallel_execution_threshold || work_count <= 1)
  {
    if constexpr (is_range_executor)
      lambda(std::begin(range), std::end(range), this_context);
    else
    {
      for (auto it = std::begin(range), end = std::end(range); it != end; ++it)
        lambda(*it, this_context);
    }
  }
  else
  {
    auto pfor_data = parallel_for_data(lambda, std::begin(range), work_count - 1);

    size_type begin = 0;
    for (size_type i = 1; i < work_count; ++i)
    {
      auto next = std::min(begin + fixed_batch_size, count);
      s.submit(this_context.get_worker(), this_context.get_workgroup(),
               [instance = &pfor_data, start = begin, end = next](worker_context const& wc)
               {
                 if constexpr (detail::RangeExcuter<L, iterator_t>)
                 {
                   instance->lambda_instance(instance->first + start, instance->first + end, wc);
                 }
                 else
                 {
                   instance->lambda_instance(*(instance->first + start), wc);
                 }
                 instance->counter.count_down();
               });
      begin = next;
    }

    // Work before wait
    {
      auto next = std::min(begin + fixed_batch_size, count);
      if constexpr (is_range_executor)
        lambda(std::begin(range) + begin, std::begin(range) + next, this_context);
      else
      {
        for (auto it = std::begin(range) + begin, end = std::begin(range) + next; it != end; ++it)
          lambda(*it, this_context);
      }
    }

    pfor_data.counter.wait();
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
