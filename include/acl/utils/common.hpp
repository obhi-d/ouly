#pragma once

#include <acl/utils/type_traits.hpp>
#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <compare>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <new>
#include <random>
#include <sstream>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <variant>
#include <vector>

#if defined(_MSC_VER)
#include <intrin.h>
#define ACL_EXPORT      __declspec(dllexport)
#define ACL_IMPORT      __declspec(dllimport)
#define ACL_EMPTY_BASES __declspec(empty_bases)
#else
#define ACL_EXPORT __attribute__((visibility("default")))
#define ACL_IMPORT __attribute__((visibility("default")))
#define ACL_EMPTY_BASES
#endif

#ifdef ACL_DLL_IMPL
#ifdef ACL_EXPORT_SYMBOLS
#define ACL_API ACL_EXPORT
#else
#define ACL_API ACL_IMPORT
#endif
#else
#define ACL_API
#endif

#if _DEBUG
#define ACL_VALIDITY_CHECKS
#endif

#ifndef ACL_PRINT_DEBUG
#define ACL_PRINT_DEBUG acl::detail::print_debug_info
#endif

#define ACL_EXTERN extern "C"

namespace acl
{
constexpr std::uint32_t safety_offset = alignof(void*);

using uhandle = std::uint32_t;
using ihandle = std::uint32_t;

inline auto align(void* ptr, size_t alignment) -> void*
{
  auto off = static_cast<size_t>(reinterpret_cast<uintptr_t>(ptr) & (alignment - 1)); // NOLINT
  return static_cast<char*>(ptr) + off;
}

namespace detail
{

template <typename SizeType>
constexpr SizeType      k_null_sz  = std::numeric_limits<SizeType>::max();
constexpr std::uint32_t k_null_0   = 0;
constexpr std::uint32_t k_null_32  = std::numeric_limits<std::uint32_t>::max();
constexpr std::int32_t  k_null_i32 = std::numeric_limits<std::int32_t>::min();
constexpr std::uint64_t k_null_64  = std::numeric_limits<std::uint64_t>::max();
constexpr uhandle       k_null_uh  = std::numeric_limits<uhandle>::max();

template <class T>
struct optional_ref
{
  using type               = std::remove_reference_t<T>;
  constexpr optional_ref() = default;
  constexpr explicit optional_ref(type& iv) noexcept : value_(&iv) {}
  constexpr explicit optional_ref(type* iv) noexcept : value_(iv) {}

  constexpr operator bool() const noexcept
  {
    return value_ != nullptr;
  }

  constexpr auto operator*() const noexcept -> type&
  {
    assert(value_);
    return *value_;
  }

  constexpr auto get() const noexcept -> type&
  {
    assert(value_);
    return *value_;
  }

  constexpr auto operator->() const noexcept -> type*
  {
    return value_;
  }

  constexpr explicit operator type*() const noexcept
  {
    return value_;
  }

  constexpr explicit operator type&() const noexcept
  {
    assert(value_);
    return *value_;
  }

  [[nodiscard]] constexpr auto has_value() const noexcept -> bool
  {
    return value_ != nullptr;
  }

  constexpr void reset() const noexcept
  {
    value_ = nullptr;
  }

  constexpr auto release() const noexcept -> type*
  {
    auto r = value_;
    value_ = nullptr;
    return r;
  }

  constexpr auto operator<=>(const optional_ref& other) const noexcept = default;

  type* value_ = nullptr;
};

template <auto Nullv>
struct optional_val
{
  using vtype              = std::decay_t<decltype(Nullv)>;
  constexpr optional_val() = default;
  constexpr optional_val(vtype iv) noexcept : value_(iv) {}

  constexpr operator bool() const noexcept
  {
    return value_ != Nullv;
  }

  constexpr auto operator*() const noexcept -> vtype
  {
    return value_;
  }

  [[nodiscard]] constexpr auto get() const noexcept -> vtype
  {
    return value_;
  }

  constexpr explicit operator vtype() const noexcept
  {
    return value_;
  }

  [[nodiscard]] constexpr auto has_value() const noexcept -> bool
  {
    return value_ != Nullv;
  }

  constexpr void reset() const noexcept
  {
    value_ = Nullv;
  }

  [[nodiscard]] constexpr auto release() const noexcept -> vtype
  {
    auto r = value_;
    value_ = Nullv;
    return r;
  }

  constexpr auto operator<=>(const optional_val& other) const noexcept = default;

  vtype value_ = Nullv;
};

template <typename Var>
struct variant_result
{
  variant_result() = default;
  template <typename... Args>
  variant_result(Args&&... args) : res_(std::forward<Args>(args)...)
  {}

  auto operator*() const -> Var const&
  {
    return res_;
  }

  auto operator*() -> Var&
  {
    return res_;
  }

  explicit operator bool() const
  {
    return res_.index() != 0;
  }

  Var res_;
};

enum ordering_by : uint8_t
{
  e_size,
  e_offset,
  k_count
};

inline void print_debug_info(std::string const& s)
{
  std::cout << s;
}

} // namespace detail

enum class response : uint8_t
{
  e_ok,
  e_cancel,
  e_continue
};

} // namespace acl
