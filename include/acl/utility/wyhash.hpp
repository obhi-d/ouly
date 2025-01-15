#pragma once
#include "external/wyhash.h"
#define WYHASH_FINAL_VERSION_3
#include "external/wyhash32.h"
#include "external/wyhash32.hpp"
#include <compare>
#include <cstdint>

namespace acl
{
constexpr uint32_t wyhash32_default_prime_seed = 1337;
constexpr uint32_t wyhash64_default_prime_seed = 11579;
class wyhash32
{
public:
  constexpr wyhash32() noexcept = default;
  constexpr wyhash32(std::uint32_t initial) noexcept : value_(initial) {}

  constexpr auto operator()() const noexcept
  {
    return value_;
  }

  auto operator()(void const* key, std::size_t len) noexcept
  {
    return (value_ = ::wyhash32(key, len, value_));
  }

  template <cwyhash32::CharType T>
  static consteval auto make(T const* const key, std::size_t len,
                             std::uint32_t seed = wyhash32_default_prime_seed) noexcept
  {
    return cwyhash32::wyhash32(key, len, seed);
  }

  template <typename T>
  constexpr auto operator()(T const& key) noexcept
  {
    return (*this)(&key, sizeof(T));
  }

  auto operator<=>(wyhash32 const&) const noexcept = default;

private:
  std::uint32_t value_ = wyhash32_default_prime_seed;
};

class wyhash64
{
public:
  wyhash64() noexcept
  {
    ::make_secret(value_, static_cast<uint64_t*>(secret_));
  }
  wyhash64(std::uint64_t initial) noexcept : value_(initial)
  {
    ::make_secret(value_, static_cast<uint64_t*>(secret_));
  }

  auto operator()() const noexcept
  {
    return value_;
  }

  auto operator()(void const* key, std::size_t len) noexcept
  {
    return (value_ = ::wyhash(key, len, value_, static_cast<uint64_t*>(secret_)));
  }

  template <typename T>
  auto operator()(T const& key) noexcept
  {
    return (*this)(&key, sizeof(T));
  }

  auto operator<=>(wyhash64 const&) const noexcept = default;

private:
  std::uint64_t value_ = wyhash64_default_prime_seed;
  std::uint64_t secret_[4]{};
};

} // namespace acl