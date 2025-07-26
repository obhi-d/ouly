// SPDX-License-Identifier: MIT
#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>

namespace ouly
{
class task_context;
}

namespace ouly::detail
{

// Simplified work item for now - will be replaced with proper task_delegate
using work_item = std::function<void(ouly::task_context const&)>;

/**
 * @brief Simple MPMC ring buffer for mailbox implementation
 * Based on the existing mpmc_ring but simplified for our specific needs
 */
template <typename T, size_t Capacity>
class simple_mpmc_ring
{
  static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2");
  static constexpr size_t mask = Capacity - 1;

public:
  simple_mpmc_ring() noexcept
  {
    for (std::size_t i = 0; i < Capacity; ++i)
    {
      buffer_[i].sequence_.store(i, std::memory_order_relaxed);
    }
  }

  ~simple_mpmc_ring() noexcept                                 = default;
  simple_mpmc_ring(const simple_mpmc_ring&)                    = delete;
  auto operator=(const simple_mpmc_ring&) -> simple_mpmc_ring& = delete;
  simple_mpmc_ring(simple_mpmc_ring&&)                         = delete;
  auto operator=(simple_mpmc_ring&&) -> simple_mpmc_ring&      = delete;

  auto push(T&& value) noexcept -> bool
  {
    node_t*     node_ptr = nullptr;
    std::size_t pos      = head_.load(std::memory_order_relaxed);
    for (;;)
    {
      node_ptr         = &buffer_[pos & mask];
      std::size_t seq  = node_ptr->sequence_.load(std::memory_order_acquire);
      intptr_t    diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos);
      if (diff == 0)
      {
        if (head_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
        {
          break;
        }
      }
      else if (diff < 0)
      {
        return false; // full
      }
      else
      {
        pos = head_.load(std::memory_order_relaxed);
      }
    }

    new (&node_ptr->storage_) T(std::move(value));
    node_ptr->sequence_.store(pos + 1, std::memory_order_release);
    return true;
  }

  template <class... Args>
  auto emplace(Args&&... args) noexcept -> bool
  {
    node_t*     node_ptr = nullptr;
    std::size_t pos      = head_.load(std::memory_order_relaxed);
    for (;;)
    {
      node_ptr         = &buffer_[pos & mask];
      std::size_t seq  = node_ptr->sequence_.load(std::memory_order_acquire);
      intptr_t    diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos);
      if (diff == 0)
      {
        if (head_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
        {
          break;
        }
      }
      else if (diff < 0)
      {
        return false; // full
      }
      else
      {
        pos = head_.load(std::memory_order_relaxed);
      }
    }

    new (&node_ptr->storage_) T(std::forward<Args>(args)...);
    node_ptr->sequence_.store(pos + 1, std::memory_order_release);
    return true;
  }

  auto pop(T& out) noexcept -> bool
  {
    node_t*     node_ptr = nullptr;
    std::size_t pos      = tail_.load(std::memory_order_relaxed);
    for (;;)
    {
      node_ptr         = &buffer_[pos & mask];
      std::size_t seq  = node_ptr->sequence_.load(std::memory_order_acquire);
      intptr_t    diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos + 1);
      if (diff == 0)
      {
        if (tail_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
        {
          break;
        }
      }
      else if (diff < 0)
      {
        return false; // empty
      }
      else
      {
        pos = tail_.load(std::memory_order_relaxed);
      }
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) - Required for type-punning
    out = std::move(*std::launder(reinterpret_cast<T*>(&node_ptr->storage_)));
    if constexpr (!std::is_trivially_destructible_v<T>)
    {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) - Required for type-punning
      std::destroy_at(std::launder(reinterpret_cast<T*>(&node_ptr->storage_)));
    }

    node_ptr->sequence_.store(pos + Capacity, std::memory_order_release);
    return true;
  }

  [[nodiscard]] auto empty() const noexcept -> bool
  {
    return head_.load(std::memory_order_relaxed) == tail_.load(std::memory_order_relaxed);
  }

  [[nodiscard]] auto size() const noexcept -> size_t
  {
    std::size_t head = head_.load(std::memory_order_relaxed);
    std::size_t tail = tail_.load(std::memory_order_relaxed);
    return head >= tail ? head - tail : 0;
  }

private:
  static constexpr size_t cache_line_size = 64;

  struct node_t
  {
    alignas(cache_line_size) std::atomic<std::size_t> sequence_{0};
    alignas(alignof(T)) std::byte storage_[sizeof(T)] = {};
  };

  alignas(cache_line_size) std::atomic<std::size_t> head_{0};
  alignas(cache_line_size) std::atomic<std::size_t> tail_{0};
  std::array<node_t, Capacity> buffer_;
};

/**
 * @brief Mailbox for cross-workgroup communication
 *
 * A mailbox is an MPMC (Multi-Producer, Multi-Consumer) queue that allows
 * workers from different workgroups to send work items to a specific workgroup.
 * This is used for the submit API where workers can submit tasks to other workgroups.
 */
class workgroup_mailbox
{
public:
  static constexpr size_t default_mailbox_capacity = 512;

  workgroup_mailbox() noexcept  = default;
  ~workgroup_mailbox() noexcept = default;

  workgroup_mailbox(const workgroup_mailbox&)                    = delete;
  auto operator=(const workgroup_mailbox&) -> workgroup_mailbox& = delete;
  workgroup_mailbox(workgroup_mailbox&&)                         = delete;
  auto operator=(workgroup_mailbox&&) -> workgroup_mailbox&      = delete;

  /**
   * @brief Send a work item to this mailbox (thread-safe)
   * @param item Work item to send
   * @return true if successfully sent, false if mailbox is full
   */
  auto send(work_item&& item) noexcept -> bool
  {
    return queue_.push(std::move(item));
  }

  /**
   * @brief Send a work item to this mailbox (thread-safe)
   * @param args Arguments to construct work item in-place
   * @return true if successfully sent, false if mailbox is full
   */
  template <typename... Args>
  auto send(Args&&... args) noexcept -> bool
  {
    return queue_.emplace(std::forward<Args>(args)...);
  }

  /**
   * @brief Receive a work item from this mailbox (thread-safe)
   * @param out Output parameter to store the received work item
   * @return true if a work item was received, false if mailbox is empty
   */
  auto receive(work_item& out) noexcept -> bool
  {
    return queue_.pop(out);
  }

  /**
   * @brief Check if the mailbox has pending work (approximate)
   * This is only a hint and may not be accurate due to concurrent operations
   */
  [[nodiscard]] auto has_work() const noexcept -> bool
  {
    return !queue_.empty();
  }

  /**
   * @brief Get approximate number of pending work items
   * This is only a hint and may not be accurate due to concurrent operations
   */
  [[nodiscard]] auto size() const noexcept -> size_t
  {
    return queue_.size();
  }

private:
  simple_mpmc_ring<work_item, default_mailbox_capacity> queue_;
};

} // namespace ouly::detail
