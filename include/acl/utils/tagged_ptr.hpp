#pragma once

#include "config.hpp"
#include <compare>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>

namespace acl
{
namespace detail
{
template <typename T>
struct reference_type
{
  using type = T&;
};

template <>
struct reference_type<void>
{
  using type = void*;
};

// Pointer compression, should not be used unless you are sure
template <typename T>
class compressed_ptr
{

public:
  using tag_t     = int8_t;
  using pointer_t = T*;
  using class_t   = T;

private:
  union pack
  {
    uintptr_t value = 0;
    int8_t    parts[8];
  };

  pack value;

  static constexpr uintptr_t TagIndex = 7;
  static constexpr uintptr_t TagMask  = 0xffffffffffffffull;
  static constexpr uintptr_t PtrMask  = ~TagMask;

  static pointer_t extract_ptr(compressed_ptr const& i)
  {
    return reinterpret_cast<pointer_t>(i.value.value & TagMask);
  }

  static tag_t extract_tag(compressed_ptr const& i)
  {
    return i.value.parts[TagIndex];
  }

  static auto pack_ptr(pointer_t ptr, tag_t tag)
  {
    pack packer;
    packer.value           = reinterpret_cast<uintptr_t>(ptr);
    packer.parts[TagIndex] = tag;
    return packer.value;
  }

public:
  compressed_ptr(std::nullptr_t) noexcept {}
  compressed_ptr() noexcept = default;
  compressed_ptr(pointer_t val, tag_t tag) noexcept : value(pack_ptr(val, tag)) {}

  void set(pointer_t p, tag_t t)
  {
    value.value = pack_ptr(p, t);
  }

  inline auto operator==(compressed_ptr const& p) const noexcept
  {
    return value.value == p.value.value;
  }

  inline auto operator!=(compressed_ptr const& p) const noexcept
  {
    return value.value != p.value.value;
  }

  inline auto operator<=>(compressed_ptr const& p) const noexcept
  {
    return value.value <=> p.value.value;
  }

  pointer_t get_ptr() const noexcept
  {
    return extract_ptr(*this);
  }

  void set_ptr(pointer_t p) noexcept
  {
    tag_t tag   = get_tag();
    value.value = pack_ptr(p, tag);
  }

  tag_t get_tag() const noexcept
  {
    return extract_tag(*this);
  }

  std::pair<pointer_t, tag_t> unpack() const noexcept
  {
    return std::make_pair<pointer_t, tag_t>(get_ptr(), get_tag());
  }

  tag_t get_next_tag() const noexcept
  {
    tag_t next = get_tag() + 1;
    return next;
  }

  void set_tag(tag_t t) noexcept
  {
    pointer_t p = get_ptr();
    value.value = pack_ptr(p, t);
  }

  /** smart pointer support  */
  /* @{ */
  typename detail::reference_type<class_t>::type operator*() const noexcept
    requires(!std::is_same_v<class_t, void>)
  {
    return *get_ptr();
  }

  pointer_t operator->() const noexcept
    requires(std::is_class_v<class_t> || std::is_union_v<class_t>)
  {
    return get_ptr();
  }

  explicit operator bool(void) const noexcept
  {
    return get_ptr() != 0;
  }
  /* @} */
};

// Pointer compression, should not be used unless you are sure
template <typename T>
class tagged_ptr
{

public:
  using tag_t     = int8_t;
  using pointer_t = T*;
  using class_t   = T;

private:
  T*    pointer = nullptr;
  tag_t tag     = 0;

public:
  tagged_ptr(std::nullptr_t) noexcept {}
  tagged_ptr() noexcept = default;
  tagged_ptr(pointer_t val, tag_t tag_v) noexcept : pointer(val), tag(tag_v) {}

  void set(pointer_t p, tag_t t)
  {
    pointer = p;
    tag     = t;
  }

  inline auto operator<=>(tagged_ptr const& other) const noexcept = default;

  pointer_t get_ptr() const noexcept
  {
    return pointer;
  }

  void set_ptr(pointer_t p) noexcept
  {
    pointer = p;
  }

  tag_t get_tag() const noexcept
  {
    return tag;
  }

  std::pair<pointer_t, tag_t> unpack() const noexcept
  {
    return std::make_pair<pointer_t, tag_t>(get_ptr(), get_tag());
  }

  tag_t get_next_tag() const noexcept
  {
    tag_t next = get_tag() + 1;
    return next;
  }

  void set_tag(tag_t t) noexcept
  {
    tag = t;
  }

  /** smart pointer support  */
  /* @{ */
  typename detail::reference_type<class_t>::type operator*() const noexcept
    requires(!std::is_same_v<class_t, void>)
  {
    return *get_ptr();
  }

  pointer_t operator->() const noexcept
    requires(std::is_class_v<class_t> || std::is_union_v<class_t>)
  {
    return get_ptr();
  }

  explicit operator bool(void) const noexcept
  {
    return get_ptr() != 0;
  }
  /* @} */
};

} // namespace detail

#ifdef ACL_PACK_TAGGED_POINTER
template <typename T>
using tagged_ptr = detail::compressed_ptr<T>;
#else
template <typename T>
using tagged_ptr = detail::tagged_ptr<T>;
#endif

} // namespace acl
