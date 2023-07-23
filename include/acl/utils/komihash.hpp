
#pragma once
#include "detail/komihash.h"
#include <compare>
#include <cstdint>

namespace acl
{
struct komihash64
{
public:
  inline constexpr komihash64() noexcept = default;
  inline constexpr komihash64(std::uint64_t initial) noexcept : value(initial) {}

  inline constexpr auto operator()() const noexcept
  {
    return value;
  }

  inline auto operator()(void const* key, std::size_t len) noexcept
  {
    return (value = ::komihash(key, len, value));
  }

  template <typename T>
  inline auto operator()(T const& key) noexcept
  {
    return (*this)(&key, sizeof(T));
  }

  inline auto operator<=>(komihash64 const&) const noexcept = default;

private:
  std::uint64_t value = 1337;
};

struct komihash64_stream
{
public:
  inline komihash64_stream() noexcept
  {
    komihash_stream_init(&ctx, 11579);
  }

  inline komihash64_stream(std::uint64_t initial) noexcept
  {
    komihash_stream_init(&ctx, initial);
  }

  inline auto operator()() noexcept
  {
    return komihash_stream_final(&ctx);
  }

  inline auto operator()(void const* key, std::size_t len) noexcept
  {
    return (::komihash_stream_update(&ctx, key, len));
  }

  template <typename T>
  inline auto operator()(T const& key) noexcept
  {
    return (*this)(&key, sizeof(T));
  }

  inline auto operator<=>(komihash64_stream const&) const noexcept = default;

private:
  ::komihash_stream_t ctx;
};

} // namespace acl