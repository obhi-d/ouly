// SPDX-License-Identifier: MIT
#pragma once

#include "ouly/scheduler/detail/cache_optimized_data.hpp"
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <memory>
#include <optional>
#include <type_traits>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4324) // structure was padded due to alignment specifier
#endif

namespace ouly::detail
{

static constexpr size_t initial_chase_lev_capacity = 256;

/**
 * @brief Chase-Lev work-stealing deque implementation
 *
 * This is a lock-free deque that supports owner push/pop from front (LIFO)
 * and thief pop from back (FIFO). Based on the Chase-Lev algorithm.
 *
 * Owner operations (single-threaded):
 * - push_front: Add work to the front (most recent)
 * - pop_front: Remove work from the front (LIFO order)
 *
 * Thief operations (multi-threaded):
 * - pop_back: Remove work from the back (FIFO order)
 *
 * @tparam T The type of elements stored in the queue
 * @tparam InitialCapacity Initial capacity (must be power of 2)
 */
template <typename T, size_t InitialCapacity = initial_chase_lev_capacity>
class chase_lev_queue
{
  static_assert(std::is_trivially_destructible_v<T>, "T must be trivially destructible for lock-free queue");
  static_assert((InitialCapacity & (InitialCapacity - 1)) == 0, "InitialCapacity must be power of 2");

public:
  chase_lev_queue() noexcept : top_(0), bottom_(0), array_(std::make_unique<circular_array>(InitialCapacity)) {}
  ~chase_lev_queue() noexcept = default;

  chase_lev_queue(const chase_lev_queue&)                    = delete;
  auto operator=(const chase_lev_queue&) -> chase_lev_queue& = delete;
  chase_lev_queue(chase_lev_queue&&)                         = delete;
  auto operator=(chase_lev_queue&&) -> chase_lev_queue&      = delete;

  /**
   * @brief Push an item to the front (owner operation - not thread-safe)
   * Only the owner thread should call this method
   */
  void push_front(T const& item) noexcept
  {
    int64_t b = bottom_.load(std::memory_order_relaxed);
    int64_t t = top_.load(std::memory_order_acquire);

    auto* a = array_.load(std::memory_order_relaxed);

    if (b - t > a->capacity() - 1)
    {
      // Queue is full, need to resize
      resize();
      a = array_.load(std::memory_order_relaxed);
    }

    a->put(b, item);
    std::atomic_thread_fence(std::memory_order_release);
    bottom_.store(b + 1, std::memory_order_relaxed);
  }

  /**
   * @brief Pop an item from the front (owner operation - not thread-safe)
   * Only the owner thread should call this method
   * @return Optional item, nullopt if queue is empty
   */
  auto pop_front(T& out) noexcept -> bool
  {
    int64_t b = bottom_.load(std::memory_order_relaxed) - 1;
    auto*   a = array_.load(std::memory_order_relaxed);
    bottom_.store(b, std::memory_order_relaxed);

    std::atomic_thread_fence(std::memory_order_seq_cst);
    int64_t t = top_.load(std::memory_order_relaxed);

    if (t <= b)
    {
      // Non-empty queue
      out = a->get(b);
      if (t == b)
      {
        // Single item, compete with thieves
        if (!top_.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed))
        {
          // Lost race with thief
          return false;
        }
        bottom_.store(b + 1, std::memory_order_relaxed);
      }
      return true;
    }
    else
    {
      // Empty queue
      bottom_.store(b + 1, std::memory_order_relaxed);
    }

    return false;
  }

  /**
   * @brief Pop an item from the back (thief operation - thread-safe)
   * Multiple threads can call this method concurrently
   * @return Optional item, nullopt if queue is empty or lost race
   */
  auto pop_back(T& out) noexcept -> bool
  {
    int64_t t = top_.load(std::memory_order_acquire);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    int64_t b = bottom_.load(std::memory_order_acquire);

    if (t < b)
    {
      // Non-empty queue
      auto* a = array_.load(std::memory_order_consume);
      out     = a->get(t);
      return top_.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed);
    }

    return false;
  }

  /**
   * @brief Check if the queue is empty (approximate)
   * This is only a hint and may not be accurate due to concurrent operations
   */
  [[nodiscard]] auto empty() const noexcept -> bool
  {
    int64_t b = bottom_.load(std::memory_order_relaxed);
    int64_t t = top_.load(std::memory_order_relaxed);
    return t >= b;
  }

  /**
   * @brief Get approximate size of the queue
   * This is only a hint and may not be accurate due to concurrent operations
   */
  [[nodiscard]] auto size() const noexcept -> size_t
  {
    int64_t b = bottom_.load(std::memory_order_relaxed);
    int64_t t = top_.load(std::memory_order_relaxed);
    return static_cast<size_t>(std::max(int64_t{0}, b - t));
  }

private:
  struct circular_array
  {
    explicit circular_array() noexcept : data_(std::make_unique<storage_slot[]>(inline_capacity)) {}

    void put(int64_t index, T const& item) noexcept
    {
      new (&data_[index & inline_capacity_mask].storage_) T(item);
    }

    auto get(int64_t index) const noexcept -> T const&
    {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) - Required for type-punning in lock-free data
      return *std::launder(reinterpret_cast<const T*>(&data_[index & inline_capacity_mask].storage_));
    }

    [[nodiscard]] auto capacity() const noexcept -> size_t
    {
      return inline_capacity;
    }

    [[nodiscard]] auto grow(int64_t top, int64_t bottom) const -> std::unique_ptr<circular_array>
    {
      auto new_array = std::make_unique<circular_array>(2 * inline_capacity);
      for (int64_t i = top; i < bottom; ++i)
      {
        new_array->put(i, get(i));
      }
      return new_array;
    }

  private:
    consteval static auto log2(size_t n) noexcept -> size_t
    {
      size_t result = 0;
      while ((n >>= 1) != 0U)
      {
        ++result;
      }
      return result;
    }

    struct storage_slot
    {
      alignas(T) std::byte storage_[sizeof(T)];
    };

    constexpr static size_t         log_capacity         = log2(InitialCapacity);
    constexpr static size_t         inline_capacity      = InitialCapacity;
    constexpr static size_t         inline_capacity_mask = inline_capacity - 1;
    std::unique_ptr<storage_slot[]> data_;
  };

  void resize() noexcept
  {
    auto*   old_array = array_.load(std::memory_order_relaxed);
    int64_t top       = top_.load(std::memory_order_relaxed);
    int64_t bottom    = bottom_.load(std::memory_order_relaxed);

    auto new_array = old_array->grow(top, bottom);
    array_.store(new_array.release(), std::memory_order_release);

    // Note: old_array cleanup is deferred to avoid ABA problems
    // In a production system, you'd want epoch-based reclamation here
  }

  alignas(cache_line_size) std::atomic<int64_t> top_;
  alignas(cache_line_size) std::atomic<int64_t> bottom_;
  alignas(cache_line_size) std::atomic<circular_array*> array_;
};

} // namespace ouly::detail

#ifdef _MSC_VER
#pragma warning(pop)
#endif