#pragma once

#include <acl/utility/config.hpp>
#include <compare>
#include <cstdint>
#include <utility>

namespace acl::detail
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
    uintptr_t value_ = 0;
    int8_t    parts_[sizeof(uintptr_t) / sizeof(int8_t)];
  };

  pack value_;

  static constexpr uintptr_t tag_index = 7;
  static constexpr uintptr_t tag_mask  = 0xffffffffffffffULL;
  static constexpr uintptr_t ptr_mask  = ~tag_mask;

  static auto extract_ptr(compressed_ptr const& i) -> pointer_t
  {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<pointer_t>(i.value_.value_ & tag_mask);
  }

  static auto extract_tag(compressed_ptr const& i) -> tag_t
  {
    return i.value_.parts_[tag_index];
  }

  static auto pack_ptr(pointer_t ptr, tag_t tag)
  {
    pack packer;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    packer.value_            = reinterpret_cast<uintptr_t>(ptr);
    packer.parts_[tag_index] = tag;
    return packer.value_;
  }

public:
  compressed_ptr(std::nullptr_t) noexcept {}
  compressed_ptr() noexcept = default;
  compressed_ptr(pointer_t val, tag_t tag) noexcept : value_(pack_ptr(val, tag)) {}

  void set(pointer_t p, tag_t t)
  {
    value_.value_ = pack_ptr(p, t);
  }

  auto operator==(compressed_ptr const& p) const noexcept
  {
    return value_.value_ == p.value_.value_;
  }

  auto operator!=(compressed_ptr const& p) const noexcept
  {
    return value_.value_ != p.value_.value_;
  }

  auto operator<=>(compressed_ptr const& p) const noexcept
  {
    return value_.value_ <=> p.value_.value_;
  }

  auto get_ptr() const noexcept -> pointer_t
  {
    return extract_ptr(*this);
  }

  void set_ptr(pointer_t p) noexcept
  {
    tag_t tag     = get_tag();
    value_.value_ = pack_ptr(p, tag);
  }

  [[nodiscard]] auto get_tag() const noexcept -> tag_t
  {
    return extract_tag(*this);
  }

  auto unpack() const noexcept -> std::pair<pointer_t, tag_t>
  {
    return std::make_pair<pointer_t, tag_t>(get_ptr(), get_tag());
  }

  [[nodiscard]] auto get_next_tag() const noexcept -> tag_t
  {
    tag_t next = get_tag() + 1;
    return next;
  }

  void set_tag(tag_t t) noexcept
  {
    pointer_t p   = get_ptr();
    value_.value_ = pack_ptr(p, t);
  }

  /** smart pointer support  */
  /* @{ */
  auto operator*() const noexcept -> typename acl::detail::reference_type<class_t>::type
    requires(!std::is_same_v<class_t, void>)
  {
    return *get_ptr();
  }

  auto operator->() const noexcept -> pointer_t
    requires(std::is_class_v<class_t> || std::is_union_v<class_t>)
  {
    return get_ptr();
  }

  explicit operator bool() const noexcept
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
  T*    pointer_ = nullptr;
  tag_t tag_     = 0;

public:
  tagged_ptr(std::nullptr_t) noexcept {}
  tagged_ptr() noexcept = default;
  tagged_ptr(pointer_t val, tag_t tag_v) noexcept : pointer_(val), tag_(tag_v) {}

  void set(pointer_t p, tag_t t)
  {
    pointer_ = p;
    tag_     = t;
  }

  auto operator<=>(tagged_ptr const& other) const noexcept = default;

  auto get_ptr() const noexcept -> pointer_t
  {
    return pointer_;
  }

  void set_ptr(pointer_t p) noexcept
  {
    pointer_ = p;
  }

  [[nodiscard]] auto get_tag() const noexcept -> tag_t
  {
    return tag_;
  }

  auto unpack() const noexcept -> std::pair<pointer_t, tag_t>
  {
    return std::make_pair<pointer_t, tag_t>(get_ptr(), get_tag());
  }

  [[nodiscard]] auto get_next_tag() const noexcept -> tag_t
  {
    tag_t next = get_tag() + 1;
    return next;
  }

  void set_tag(tag_t t) noexcept
  {
    tag_ = t;
  }

  /** smart pointer support  */
  /* @{ */
  auto operator*() const noexcept -> typename acl::detail::reference_type<class_t>::type
    requires(!std::is_same_v<class_t, void>)
  {
    return *get_ptr();
  }

  auto operator->() const noexcept -> pointer_t
    requires(std::is_class_v<class_t> || std::is_union_v<class_t>)
  {
    return get_ptr();
  }

  explicit operator bool() const noexcept
  {
    return get_ptr() != 0;
  }
  /* @} */
};

} // namespace acl::detail
