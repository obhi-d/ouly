#pragma once
// SPDX-License-Identifier: MIT
#include <concepts>
#include <cstdint>

namespace ouly
{
template <typename T>
concept DefaultPartitionerTraits = requires(T t) {
  { T::batches_per_worker } -> std::convertible_to<uint32_t>;
  { T::parallel_execution_threshold } -> std::convertible_to<uint32_t>;
  { T::fixed_batch_size } -> std::convertible_to<uint32_t>;
};

struct default_partitioner_traits
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

template <typename T>
concept AutoParitionerTraits = requires(T t) {
  { T::grain_size } -> std::convertible_to<uint32_t>;
  { T::max_depth } -> std::convertible_to<uint32_t>;
  { T::depth_increment } -> std::convertible_to<uint32_t>;
  { T::sequential_threshold } -> std::convertible_to<uint32_t>;
  { T::range_pool_capacity } -> std::convertible_to<uint32_t>;
};

/**
 * @brief Auto partitioner traits following TBB's design
 */
struct auto_partitioner_traits
{
  /** Grain size for auto partitioning */
  static constexpr uint32_t grain_size = 1U;

  /** Maximum depth for partitioning */
  static constexpr uint32_t max_depth = 8;

  /** Depth increment due to steal */
  static constexpr uint32_t depth_increment = 1;

  /** Sequential threshold for splitting */
  static constexpr uint32_t sequential_threshold = 128;

  /** Range pool capacity  */
  static constexpr uint32_t range_pool_capacity = 8;
};
} // namespace ouly