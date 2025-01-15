#pragma once
#include <acl/allocators/default_allocator.hpp>
#include <acl/containers/podvector.hpp>
#include <acl/utility/common.hpp>
#include <acl/utility/utils.hpp>
#include <type_traits>
#include <vector>

namespace acl
{
/**
 * @brief A container class that manages a pool of elements with recycled indices
 *
 * @tparam T The type of elements stored in the table
 * @tparam IsPOD Boolean indicating if T is a Plain Old Data type
 *
 * This container provides constant-time insertion and deletion operations while
 * maintaining stable indices to elements. When elements are erased, their slots
 * are recycled for future insertions. The implementation differs based on whether
 * T is a POD type or not:
 *
 * For POD types:
 * - Uses a single free_idx structure to track unused slots
 * - Reuses memory of deleted elements to store the free list
 * - Maintains a count of valid elements
 *
 * For non-POD types:
 * - Uses a separate vector to store free indices
 * - Resets deleted elements to default state
 * - Recycles indices from the free list
 *
 * Memory layout is optimized for POD types by avoiding additional storage for
 * managing free slots.
 *
 * @note Indices remain stable until the referenced element is erased
 * @note The implementation uses reinterpret_cast for POD types which assumes
 *       the size of T is sufficient to store a 32-bit index
 */
template <DefaultConstructible T, bool IsPOD = std::is_trivial_v<T>>
class table
{
  struct free_idx
  {
    std::uint32_t unused_ = std::numeric_limits<uint32_t>::max();
    std::uint32_t valids_ = 0;
  };

  using vector   = std::conditional_t<IsPOD, podvector<T>, acl::vector<T>>;
  using freepool = std::conditional_t<IsPOD, free_idx, podvector<std::uint32_t>>;

public:
  table() noexcept = default;
  table(vector pool, freepool free_pool) : pool_(std::move(pool)), free_pool_(std::move(free_pool)) {}
  table(table const&)     = default;
  table(table&&) noexcept = default;
  ~table() noexcept       = default;

  auto operator=(table const&) -> table&     = default;
  auto operator=(table&&) noexcept -> table& = default;

  template <typename... Args>
  auto emplace(Args&&... args) -> std::uint32_t
  {
    std::uint32_t index = 0;
    if constexpr (IsPOD)
    {
      if (free_pool_.unused_ != std::numeric_limits<uint32_t>::max())
      {
        index = free_pool_.unused_;
        // NOLINTNEXTLINE
        free_pool_.unused_ = reinterpret_cast<std::uint32_t&>(pool_[free_pool_.unused_]);
      }
      else
      {
        index = static_cast<std::uint32_t>(pool_.size());
        pool_.resize(index + 1);
      }
      pool_[index] = T(std::forward<Args>(args)...);
      free_pool_.valids_++;
    }
    else
    {
      if (!free_pool_.empty())
      {
        index = free_pool_.back();
        free_pool_.pop_back();
        pool_[index] = std::move(T(std::forward<Args>(args)...));
      }
      else
      {
        index = static_cast<std::uint32_t>(pool_.size());
        pool_.emplace_back(std::forward<Args>(args)...);
      }
    }
    return index;
  }

  void erase(std::uint32_t index)
  {
    if constexpr (IsPOD)
    {
      // NOLINTNEXTLINE
      reinterpret_cast<std::uint32_t&>(pool_[index]) = free_pool_.unused_;
      free_pool_.unused_                             = index;
      free_pool_.valids_--;
    }
    else
    {
      pool_[index] = T();
      free_pool_.emplace_back(index);
    }
  }

  auto operator[](std::uint32_t i) -> T&
  {
    // NOLINTNEXTLINE
    return reinterpret_cast<T&>(pool_[i]);
  }

  auto operator[](std::uint32_t i) const -> T const&
  {
    // NOLINTNEXTLINE
    return reinterpret_cast<T const&>(pool_[i]);
  }

  auto at(std::uint32_t i) -> T&
  {
    // NOLINTNEXTLINE
    return reinterpret_cast<T&>(pool_[i]);
  }

  auto at(std::uint32_t i) const -> T const&
  {
    // NOLINTNEXTLINE
    return reinterpret_cast<T const&>(pool_[i]);
  }

  [[nodiscard]] auto size() const -> std::uint32_t
  {
    if constexpr (IsPOD)
    {
      return free_pool_.valids_;
    }
    else
    {
      return static_cast<std::uint32_t>(pool_.size() - free_pool_.size());
    }
  }

  [[nodiscard]] auto capacity() const -> uint32_t
  {
    return static_cast<uint32_t>(pool_.size());
  }

private:
  vector   pool_;
  freepool free_pool_;
};

} // namespace acl
