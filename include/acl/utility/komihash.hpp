
#pragma once
#include "external/komihash.h"
#include <compare>
#include <cstdint>

namespace acl
{
constexpr uint32_t komihash_default_seed        = 1337;
constexpr uint32_t komihash_default_stream_init = 11579;

struct komihash64
{
public:
  constexpr komihash64() noexcept = default;
  constexpr komihash64(std::uint64_t initial) noexcept : value_(initial) {}

  constexpr auto operator()() const noexcept
  {
    return value_;
  }

  auto operator()(void const* key, std::size_t len) noexcept
  {
    return (value_ = ::komihash(key, len, value_));
  }

  template <typename T>
  auto operator()(T const& key) noexcept
  {
    return (*this)(&key, sizeof(T));
  }

  auto operator<=>(komihash64 const&) const noexcept = default;

private:
  std::uint64_t value_ = komihash_default_seed;
};

struct komihash64_stream
{
public:
  komihash64_stream() noexcept
  {
    komihash_stream_init(&ctx_, komihash_default_stream_init);
  }

  komihash64_stream(std::uint64_t initial) noexcept
  {
    komihash_stream_init(&ctx_, initial);
  }

  auto operator()() noexcept
  {
    return komihash_stream_final(&ctx_);
  }

  auto operator()(void const* key, std::size_t len) noexcept
  {
    (::komihash_stream_update(&ctx_, key, len));
  }

  template <typename T>
  auto operator()(T const& key) noexcept
  {
    return (*this)(&key, sizeof(T));
  }

private:
  ::komihash_stream_t ctx_{};
};

} // namespace acl