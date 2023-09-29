#pragma once
#include "detail/wyhash.h"
#define wyhash_final_version_3
#include "detail/wyhash32.h"
#include "detail/wyhash32.hpp"
#include <compare>
#include <cstdint>

namespace acl
{    

class wyhash32
{
public:
  inline constexpr wyhash32() noexcept = default;
  inline constexpr wyhash32(std::uint32_t initial) noexcept : value(initial) {}

  inline constexpr auto operator()() const noexcept
  {
    return value;
  }

  inline auto operator()(void const* key, std::size_t len) noexcept
  {
    return (value = ::wyhash32(key, len, value));
  }
    
  template <cwyhash32::CharType T>
  static inline consteval auto make(T const* const key, std::size_t len, std::uint32_t seed = 1337) noexcept
  {
    return cwyhash32::wyhash32(key, len, seed);
  }

  template <typename T>
  constexpr auto operator()(T const& key) noexcept
  {
    return (*this)(&key, sizeof(T));
  }

  inline auto operator<=>(wyhash32 const&) const noexcept = default;

private:
  std::uint32_t value = 1337;
};

class wyhash64
{
public:
  inline wyhash64() noexcept
  {
    ::make_secret(value, secret);
  }
  inline wyhash64(std::uint64_t initial) noexcept : value(initial)
  {
    ::make_secret(value, secret);
  }

  inline auto operator()() const noexcept
  {
    return value;
  }

  inline auto operator()(void const* key, std::size_t len) noexcept
  {
    return (value = ::wyhash(key, len, value, secret));
  }

  template <typename T>
  inline auto operator()(T const& key) noexcept
  {
    return (*this)(&key, sizeof(T));
  }

  inline auto operator<=>(wyhash64 const&) const noexcept = default;

private:
  std::uint64_t value = 11579;
  std::uint64_t secret[4];
};

} // namespace acl