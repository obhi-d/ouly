// SPDX-License-Identifier: MIT

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>

#include "ouly/scheduler/detail/cache_optimized_data.hpp"

/** The configuration is currently unused */
namespace ouly::cfg
{

/**
 * @brief Configuration parameters for scheduler performance tuning
 */
struct scheduler_config
{
  // Work stealing parameters
  uint32_t max_steal_attempts_       = 8;
  uint32_t steal_retry_delay_cycles_ = 4;
  uint32_t max_victims_per_group_    = 4;

  // Queue management
  uint32_t max_local_queue_size_ = 256;
  uint32_t work_batch_size_      = 16;

  // Performance tuning
  uint32_t spin_before_yield_    = 1000;
  uint32_t max_yield_iterations_ = 10;

  // Memory allocation
  bool use_unified_memory_layout_ = true;
  bool enable_numa_awareness_     = false;

  // Monitoring and debugging
  bool                      collect_performance_metrics_ = false;
  std::chrono::milliseconds metrics_collection_interval_{1000};

  /**
   * @brief Create a configuration optimized for high throughput workloads
   */
  static auto throughput_optimized() noexcept -> scheduler_config
  {
    constexpr uint32_t max_steal_attempts = 16;   // Maximum delay in cycles
    constexpr uint32_t work_batch_size    = 32;   // Maximum delay in cycles
    constexpr uint32_t spin_before_yield  = 2000; // Maximum total delay in cycles

    scheduler_config config;
    config.max_steal_attempts_ = max_steal_attempts;
    config.work_batch_size_    = work_batch_size;
    config.spin_before_yield_  = spin_before_yield;
    return config;
  }

  /**
   * @brief Create a configuration optimized for low latency workloads
   */
  static auto latency_optimized() noexcept -> scheduler_config
  {
    constexpr uint32_t max_steal_attempts       = 4;   // Maximum delay in cycles
    constexpr uint32_t steal_retry_delay_cycles = 2;   // Maximum delay in cycles
    constexpr uint32_t spin_before_yield        = 100; // Maximum total delay in cycles
    constexpr uint32_t max_yield_iterations     = 3;   // Maximum total

    scheduler_config config;
    config.max_steal_attempts_       = max_steal_attempts;
    config.steal_retry_delay_cycles_ = steal_retry_delay_cycles;
    config.spin_before_yield_        = spin_before_yield;
    config.max_yield_iterations_     = max_yield_iterations;
    return config;
  }

  /**
   * @brief Create a configuration from environment variables
   */
  static auto from_environment() noexcept -> scheduler_config
  {
    scheduler_config config;
    // Load from environment variables or system settings
    // This is a placeholder; actual implementation would read env vars
    // and set the config parameters accordingly.
    return config;
  }
};

/**
 * @brief Performance metrics collection for scheduler monitoring
 */
struct scheduler_metrics
{
  // Task execution metrics
  alignas(detail::cache_line_size) std::atomic<uint64_t> tasks_executed_{0};
  alignas(detail::cache_line_size) std::atomic<uint64_t> tasks_stolen_{0};
  alignas(detail::cache_line_size) std::atomic<uint64_t> steal_attempts_{0};
  alignas(detail::cache_line_size) std::atomic<uint64_t> failed_steals_{0};

  // Synchronization metrics
  alignas(detail::cache_line_size) std::atomic<uint64_t> wake_events_{0};
  alignas(detail::cache_line_size) std::atomic<uint64_t> spurious_wakeups_{0};

  // Performance indicators
  alignas(detail::cache_line_size) std::atomic<uint64_t> total_work_time_ns_{0};
  alignas(detail::cache_line_size) std::atomic<uint64_t> total_idle_time_ns_{0};

  /**
   * @brief Calculate work stealing success rate
   */
  [[nodiscard]] auto steal_success_rate() const noexcept -> double
  {
    auto attempts = steal_attempts_.load(std::memory_order_relaxed);
    auto failures = failed_steals_.load(std::memory_order_relaxed);
    return attempts > 0 ? 1.0 - (double(failures) / double(attempts)) : 0.0;
  }

  /**
   * @brief Calculate worker utilization percentage
   */
  [[nodiscard]] auto worker_utilization() const noexcept -> double
  {
    auto work_time  = total_work_time_ns_.load(std::memory_order_relaxed);
    auto idle_time  = total_idle_time_ns_.load(std::memory_order_relaxed);
    auto total_time = work_time + idle_time;
    return total_time > 0 ? (double(work_time) / double(total_time)) * 100.0 : 0.0;
  }

  /**
   * @brief Reset all metrics to zero
   */
  void reset() noexcept
  {
    tasks_executed_.store(0, std::memory_order_relaxed);
    tasks_stolen_.store(0, std::memory_order_relaxed);
    steal_attempts_.store(0, std::memory_order_relaxed);
    failed_steals_.store(0, std::memory_order_relaxed);
    wake_events_.store(0, std::memory_order_relaxed);
    spurious_wakeups_.store(0, std::memory_order_relaxed);
    total_work_time_ns_.store(0, std::memory_order_relaxed);
    total_idle_time_ns_.store(0, std::memory_order_relaxed);
  }
};

} // namespace ouly::cfg
