
#pragma once

#include "scheduler.hpp"
#include <acl/utils/type_traits.hpp>
#include <latch>

namespace acl
{
namespace detail
{
template <typename L, typename It>
concept RangeExcuter = requires(L l, It range, worker_context const& wc) { l(range, range, wc); };
}

template <typename L, typename FwIt>
void parallel_for(L&& lambda, FwIt range, uint32_t granularity, worker_context const& this_context)
{
  auto& s          = this_context.get_scheduler();
  using iterator_t = decltype(std::begin(range));
  union uint_range
  {
    struct
    {
      uint32_t begin;
      uint32_t end;
    };
    uintptr_t     range;
    task_context* task;
  };

  struct parallel_for_executer : public task
  {
    parallel_for_executer(L& lambda, iterator_t f, uint32_t task_count) noexcept
        : lambda_instance(lambda), first(f), counter(task_count)
    {}

    void operator()(task_context* data, worker_context const& wc) override
    {
      uint_range urange;
      urange.task = data;
      if constexpr (detail::RangeExcuter<L, iterator_t>)
      {
        lambda_instance(first + urange.begin, first + urange.end, wc);
      }
      else
      {
        lambda_instance(*(first + urange.begin), wc);
      }
      counter.count_down();
    }

    iterator_t first;
    std::latch counter;
    L&         lambda_instance;
  };

  constexpr bool is_range_executor = detail::RangeExcuter<L, iterator_t>;
  uint32_t       count             = static_cast<uint32_t>(std::distance(std::begin(range), std::end(range)));
  const uint32_t task_count =
    is_range_executor ? s.get_logical_divisor(this_context.get_work_group()) * granularity : count;
  const uint32_t fixed = is_range_executor ? ((count + task_count - 1) / task_count) : 1;
  if (!task_count)
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
    auto     executer = parallel_for_executer(lambda, std::begin(range), task_count);
    uint32_t begin    = 0;
    for (uint32_t i = 1; i < task_count; ++i)
    {
      uint_range urange;
      urange.begin = begin;
      urange.end   = std::min(begin + fixed, count);
      s.submit(&executer, urange.task, this_context.get_work_group(), this_context.get_worker());
      begin = urange.end;
    }

    // Work before wait
    {
      uint_range urange;
      urange.begin = begin;
      urange.end   = std::min(begin + fixed, count);
      executer(urange.task, this_context);
    }

    executer.counter.wait();
  }
}

template <typename L, typename FwIt>
void parallel_for(L&& lambda, FwIt range, uint32_t granularity, worker_id current, work_group_id work_group,
                  scheduler& s)
{
  auto const& this_context = s.get_context(current, work_group);
  // Assert this context belongs to the work group selected for submission
  assert(
    this_context.belongs_to(work_group) &&
    "Current worker does not belong to the work group for 'parallel_for' submission and thus cannot execute the task.");
  parallel_for(std::forward<L>(lambda), range, granularity, this_context);
}

template <typename L, typename FwIt>
void parallel_for(L&& lambda, FwIt range, uint32_t granularity, work_group_id work_group)
{
  auto const& this_context = worker_context::get_context(work_group);
  parallel_for(std::forward<L>(lambda), range, granularity, this_context);
}

} // namespace acl