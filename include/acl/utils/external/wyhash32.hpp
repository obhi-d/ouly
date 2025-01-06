#pragma once
// NOLINTBEGIN
// Author: Wang Yi <godspeed_china@yeah.net>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <tuple>
#include <utility>

namespace cwyhash32
{

template <typename T>
concept CharType =
 std::same_as<T, char> || std::same_as<T, std::byte> || std::same_as<T, char8_t> || std::same_as<T, unsigned char>;

template <CharType T>
static consteval inline uint32_t _wyr32(const T* const p)
{
#ifndef WYHASH32_BIG_ENDIAN
  constexpr int byte_order[4] = {3, 2, 1, 0};
#else
  constexpr int byte_order[4] = {0, 1, 2, 3};
#endif
  return (uint32_t)p[byte_order[0]] << 24 | (uint32_t)p[byte_order[1]] << 16 | (uint32_t)p[byte_order[2]] << 8 |
         (uint32_t)p[byte_order[3]];
}

template <CharType T>
static consteval inline uint32_t _wyr24(const T* const p, uint32_t k)
{
  return (((uint32_t)p[0]) << 16) | (((uint32_t)p[k >> 1]) << 8) | p[k - 1];
}

static consteval inline auto _wymix32(uint32_t A, uint32_t B)
{
  uint64_t c = A ^ 0x53c5ca59u;
  c *= B ^ 0x74743c1bu;
  return std::make_pair((uint32_t)c, (uint32_t)(c >> 32));
}
// This version is vulnerable when used with a few bad seeds, which should be skipped beforehand:
// 0x429dacdd, 0xd637dbf3
template <CharType T>
static consteval inline uint32_t wyhash32(const T* const key, uint64_t len, uint32_t seed)
{
  const T* p    = key;
  uint64_t i    = len;
  uint32_t see1 = (uint32_t)len;
  seed ^= (uint32_t)(len >> 32);
  std::tie(seed, see1) = _wymix32(seed, see1);
  for (; i > 8; i -= 8, p += 8)
  {
    seed ^= _wyr32(p);
    see1 ^= _wyr32(p + 4);
    std::tie(seed, see1) = _wymix32(seed, see1);
  }
  if (i >= 4)
  {
    seed ^= _wyr32(p);
    see1 ^= _wyr32(p + i - 4);
  }
  else if (i)
    seed ^= _wyr24(p, (uint32_t)i);
  std::tie(seed, see1) = _wymix32(seed, see1);
  std::tie(seed, see1) = _wymix32(seed, see1);
  return seed ^ see1;
}

} // namespace cwyhash32
// NOLINTEND