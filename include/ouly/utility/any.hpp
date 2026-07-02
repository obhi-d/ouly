// SPDX-License-Identifier: MIT
#pragma once

/**
 * @file any.hpp
 * @brief A std::any-equivalent type-erased value container with a configurable
 *        inline (small-buffer-optimized) storage size.
 *
 * Unlike std::any, the inline buffer size is a template parameter. Values whose
 * size/alignment fit the inline buffer (and are nothrow-move-constructible) are
 * stored in place; larger values are heap allocated. The stored type is tracked
 * with a stable per-type key so that get_if<T>() can validate the access.
 */

#include "ouly/reflection/type_name.hpp"
#include "ouly/utility/user_config.hpp"

#include <cstddef>
#include <cstdint>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace ouly
{

/**
 * @brief Thrown when a copy is requested on an ouly::any holding a non-copyable
 *        value.
 */
struct bad_any_copy : std::runtime_error
{
  bad_any_copy() : std::runtime_error("ouly::any: stored type is not copyable") {}
};

/**
 * @brief Type-erased value with a configurable inline storage size.
 *
 * @tparam InlineSize Number of bytes reserved for in-place storage.
 * @tparam Align      Alignment of the inline storage.
 */
template <std::size_t InlineSize = ouly_any_default_inline_size, std::size_t Align = alignof(std::max_align_t)>
class basic_any
{
  // The inline buffer must at least be able to hold a heap pointer.
  static constexpr std::size_t buffer_size = (InlineSize < sizeof(void*)) ? sizeof(void*) : InlineSize;

  template <typename T>
  static constexpr bool fits_inline =
   sizeof(T) <= buffer_size && alignof(T) <= Align && std::is_nothrow_move_constructible_v<T>;

  enum class op : uint8_t
  {
    destroy,
    move,
    copy,
    type,
    type_hash
  };

  using manager_fn = auto (*)(op, basic_any*, basic_any*) -> void const*;

  // Stable, unique key per stored type used to validate casts.
  template <typename T>
  static auto type_key() noexcept -> void const*
  {
    static const char key{};
    return &key;
  }

  template <typename T>
  static auto manage(op operation, basic_any* self, basic_any* other) -> void const*
  {
    if constexpr (fits_inline<T>)
    {
      switch (operation)
      {
      case op::destroy:
        // NOLINTNEXTLINE
        std::launder(reinterpret_cast<T*>(self->storage_))->~T();
        return nullptr;
      case op::move:
      {
        // NOLINTNEXTLINE
        T* src = std::launder(reinterpret_cast<T*>(other->storage_));
        ::new (static_cast<void*>(self->storage_)) T(std::move(*src));
        src->~T();
        return nullptr;
      }
      case op::copy:
      {
        if constexpr (std::is_copy_constructible_v<T>)
        {
          // NOLINTNEXTLINE
          T const* src = std::launder(reinterpret_cast<T const*>(other->storage_));
          ::new (static_cast<void*>(self->storage_)) T(*src);
          return nullptr;
        }
        else
        {
          throw bad_any_copy();
        }
      }
      case op::type:
        return type_key<T>();
      case op::type_hash:
      {
        static constexpr auto hash = ouly::type_hash<T>();
        return &hash;
      }
      }
    }
    else
    {
      switch (operation)
      {
      case op::destroy:
        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
        delete static_cast<T*>(self->heap_);
        return nullptr;
      case op::move:
        self->heap_  = other->heap_;
        other->heap_ = nullptr;
        return nullptr;
      case op::copy:
      {
        if constexpr (std::is_copy_constructible_v<T>)
        {
          // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
          self->heap_ = new T(*static_cast<T const*>(other->heap_));
          return nullptr;
        }
        else
        {
          throw bad_any_copy();
        }
      }
      case op::type:
        return type_key<T>();
      case op::type_hash:
      {
        static constexpr auto hash = ouly::type_hash<T>();
        return &hash;
      }
      }
    }
    return nullptr;
  }

  template <typename T>
  auto cast_ptr() noexcept -> T*
  {
    if constexpr (fits_inline<T>)
    {
      // NOLINTNEXTLINE
      return std::launder(reinterpret_cast<T*>(storage_));
    }
    else
    {
      return static_cast<T*>(heap_);
    }
  }

  union
  {
    alignas(Align) std::byte storage_[buffer_size];
    void* heap_;
  };
  manager_fn manager_ = nullptr;

public:
  using is_any = void;

  basic_any() noexcept : heap_(nullptr) {}

  basic_any(basic_any const& other) : heap_(nullptr)
  {
    if (other.manager_ != nullptr)
    {
      other.manager_(op::copy, this, const_cast<basic_any*>(&other));
      manager_ = other.manager_;
    }
  }

  basic_any(basic_any&& other) noexcept : heap_(nullptr)
  {
    if (other.manager_ != nullptr)
    {
      other.manager_(op::move, this, &other);
      manager_       = other.manager_;
      other.manager_ = nullptr;
    }
  }

  auto operator=(basic_any const& other) -> basic_any&
  {
    if (this != &other)
    {
      basic_any tmp(other);
      swap(tmp);
    }
    return *this;
  }

  auto operator=(basic_any&& other) noexcept -> basic_any&
  {
    if (this != &other)
    {
      reset();
      if (other.manager_ != nullptr)
      {
        other.manager_(op::move, this, &other);
        manager_       = other.manager_;
        other.manager_ = nullptr;
      }
    }
    return *this;
  }

  ~basic_any() noexcept
  {
    reset();
  }

  /**
   * @brief Construct a value of type T in place, replacing any current value.
   * @return Reference to the newly constructed value.
   */
  template <typename T, typename... Args>
  auto emplace(Args&&... args) -> std::decay_t<T>&
  {
    using U = std::decay_t<T>;
    reset();
    if constexpr (fits_inline<U>)
    {
      ::new (static_cast<void*>(storage_)) U(std::forward<Args>(args)...);
    }
    else
    {
      // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
      heap_ = new U(std::forward<Args>(args)...);
    }
    manager_ = &manage<U>;
    return *cast_ptr<U>();
  }

  /**
   * @brief Returns a pointer to the stored value if it is of type T, else null.
   */
  template <typename T>
  auto get_if() noexcept -> T*
  {
    using U = std::decay_t<T>;
    if (manager_ != nullptr && manager_(op::type, this, nullptr) == type_key<U>())
    {
      return cast_ptr<U>();
    }
    return nullptr;
  }

  template <typename T>
  auto get_if() const noexcept -> T const*
  {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    return const_cast<basic_any*>(this)->template get_if<T>();
  }

  [[nodiscard]] auto has_value() const noexcept -> bool
  {
    return manager_ != nullptr;
  }

  [[nodiscard]] auto type_hash() const noexcept -> std::uint32_t
  {
    if (manager_ == nullptr)
    {
      return 0;
    }
    auto const* hash =
     static_cast<std::uint32_t const*>(manager_(op::type_hash, const_cast<basic_any*>(this), nullptr));
    return *hash;
  }

  void reset() noexcept
  {
    if (manager_ != nullptr)
    {
      manager_(op::destroy, this, nullptr);
      manager_ = nullptr;
    }
  }

  void swap(basic_any& other) noexcept
  {
    basic_any tmp(std::move(other));
    other = std::move(*this);
    *this = std::move(tmp);
  }
};

using any = basic_any<>;

} // namespace ouly
