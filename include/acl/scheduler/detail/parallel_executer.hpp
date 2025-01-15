#pragma once

#include <acl/scheduler/scheduler.hpp>
#include <acl/scheduler/task_traits.hpp>
#include <iterator>

namespace acl::detail
{

template <typename L, typename It>
concept RangeExcuter = requires(L l, It range, worker_context const& wc) { l(range, range, wc); };
template <typename I>
concept HasIteratorDiff = requires(I s) {
  { s.size() } -> std::integral;
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
  static auto size(T const& range) noexcept -> uint32_t
  {
    return static_cast<uint32_t>(range.size());
  }
};

template <HasIteratorTraits T>
struct it_size_type<T>
{
  using type = typename std::iterator_traits<T>::difference_type;

  static auto size(T const& range) noexcept -> uint32_t
  {
    return static_cast<uint32_t>(std::distance(std::begin(range), std::end(range)));
  }
};

// Concept to check if a type has 'fixed_batch_size'
template <typename T>
concept HasFixedBatchSize = requires {
  { T::fixed_batch_size } -> std::convertible_to<uint32_t>;
};

// Concept to check if a type has 'batches_per_worker'
template <typename T>
concept HasBatchesPerWorker = requires {
  { T::batches_per_worker } -> std::convertible_to<uint32_t>;
};

// Concept to check if a type has 'parallel_execution_threshold'
template <typename T>
concept HasParallelExecutionThreshold = requires {
  { T::parallel_execution_threshold } -> std::convertible_to<uint32_t>;
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

constexpr auto get_work_count(uint32_t batches_per_wk, uint32_t wk_count, uint32_t tk_count) -> uint32_t
{
  uint32_t batch_count = wk_count * batches_per_wk;
  return (tk_count + batch_count - 1) / batch_count;
}
} // namespace acl::detail