// SPDX-License-Identifier: MIT
#pragma once
#include "ouly/allocators/default_allocator.hpp"
#include "ouly/utility/common.hpp"
#include "ouly/utility/utils.hpp"
#include <limits>
#include <type_traits>
#include <utility>
#include <vector>

namespace ouly
{
namespace detail
{

template <typename T, auto Policy, typename = void>
struct table_policy_traits
{
  static constexpr bool uses_projection = false;
  static constexpr bool is_pod_like     = static_cast<bool>(Policy);
  using member_pointer                  = void;
  using member_type                     = std::uint32_t;
};

template <typename T, auto Policy>
struct table_policy_traits<T, Policy, std::enable_if_t<std::is_member_object_pointer_v<decltype(Policy)>>>
{
  static constexpr bool uses_projection = true;
  static constexpr bool is_pod_like     = true;
  using member_pointer                  = decltype(Policy);
  using member_type                     = std::remove_cvref_t<decltype(std::declval<T>().*Policy)>;
};

} // namespace detail
/**
 * @brief A container class that manages a pool of elements with recycled indices
 *
 * @tparam T The type of elements stored in the table
 * @tparam Policy Compile-time policy that selects how free indices are stored. It can be
 *         either a boolean (true enables the POD-optimized path, false uses the general
 *         path) or a pointer-to-member of T that should receive the recycled index.
 *
 * This container provides constant-time insertion and deletion operations while
 * maintaining stable indices to elements. When elements are erased, their slots
 * are recycled for future insertions. The implementation differs based on whether
 * T is a POD type or not:
 *
 * For POD/projection policies:
 * - Uses a single free_idx structure to track unused slots
 * - Reuses memory of deleted elements to store the free list
 * - Optionally writes free indices into the specified member when a projection is provided
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
 * @note The boolean POD policy uses reinterpret_cast which assumes the size of T is
 *       sufficient to store a 32-bit index. When a projection is supplied, the free index
 *       is stored in the selected member instead.
 */
template <DefaultConstructible T, auto Policy = std::is_trivial_v<T>>
class table
{

  static constexpr std::uint32_t null_index = std::numeric_limits<std::uint32_t>::max();

  using traits                         = detail::table_policy_traits<T, Policy>;
  static constexpr bool use_projection = std::is_member_object_pointer_v<decltype(Policy)>;
  static constexpr bool is_pod         = traits::is_pod_like;

  struct free_idx
  {
    std::uint32_t unused_ = null_index;
    std::uint32_t valids_ = 0;
  };

  using vector                    = std::vector<T>;
  using freepool                  = std::conditional_t<is_pod, free_idx, std::vector<std::uint32_t>>;
  using projection_member_pointer = typename traits::member_pointer;
  using projection_member_type    = typename traits::member_type;

  static_assert(is_pod || !use_projection, "projection requires POD policy");
  static_assert(!use_projection || sizeof(projection_member_type) >= sizeof(std::uint32_t),
                "projection member must be large enough to store the free index");
  static_assert(!use_projection || std::is_assignable_v<projection_member_type&, std::uint32_t>,
                "projection member must accept std::uint32_t values");
  static_assert(!use_projection || std::is_convertible_v<projection_member_type, std::uint32_t>,
                "projection member must be convertible to std::uint32_t");

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
    if constexpr (is_pod)
    {
      if (free_pool_.unused_ != null_index)
      {
        index              = free_pool_.unused_;
        free_pool_.unused_ = read_free_link(index);
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

  template <typename MemberPointer>
    requires(is_pod && std::is_member_object_pointer_v<MemberPointer>)
  void erase(std::uint32_t index, MemberPointer member)
  {
    write_free_link(index, free_pool_.unused_);
    assign_member_value(pool_[index], member, free_pool_.unused_);
    free_pool_.unused_ = index;
    free_pool_.valids_--;
  }

  void erase(std::uint32_t index)
  {
    if constexpr (is_pod)
    {
      write_free_link(index, free_pool_.unused_);
      free_pool_.unused_ = index;
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
    if constexpr (is_pod)
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

  void swap(table& other) noexcept
  {
    using std::swap;
    swap(pool_, other.pool_);
    swap(free_pool_, other.free_pool_);
  }

  friend void swap(table& lhs, table& rhs) noexcept
  {
    lhs.swap(rhs);
  }

  auto data() noexcept -> T*
  {
    return pool_.data();
  }

  auto data() const noexcept -> T const*
  {
    return pool_.data();
  }

  auto begin() noexcept -> typename vector::iterator
  {
    return pool_.begin();
  }

  auto end() noexcept -> typename vector::iterator
  {
    return pool_.end();
  }

  auto begin() const noexcept -> typename vector::const_iterator
  {
    return pool_.begin();
  }

  auto end() const noexcept -> typename vector::const_iterator
  {
    return pool_.end();
  }

private:
  [[nodiscard]] auto read_free_link(std::uint32_t index) -> std::uint32_t
  {
    if constexpr (use_projection)
    {
      return static_cast<std::uint32_t>(pool_[index].*Policy);
    }
    else
    {
      // NOLINTNEXTLINE
      return reinterpret_cast<std::uint32_t&>(pool_[index]);
    }
  }

  void write_free_link(std::uint32_t index, std::uint32_t value)
  {
    if constexpr (use_projection)
    {
      assign_member_value(pool_[index], Policy, value);
    }
    else
    {
      // NOLINTNEXTLINE
      reinterpret_cast<std::uint32_t&>(pool_[index]) = value;
    }
  }

  template <typename MemberPointer>
  static void assign_member_value(T& element, MemberPointer member, std::uint32_t value)
  {
    using member_type = std::remove_cvref_t<decltype(element.*member)>;
    static_assert(std::is_member_object_pointer_v<MemberPointer>, "member must be a pointer to member");
    static_assert(sizeof(member_type) >= sizeof(std::uint32_t), "member must be large enough to store the free index");
    static_assert(std::is_assignable_v<member_type&, std::uint32_t>, "member must accept std::uint32_t values");
    element.*member = static_cast<member_type>(value);
  }

  vector   pool_;
  freepool free_pool_;
};

} // namespace ouly
