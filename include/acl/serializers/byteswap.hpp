#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <concepts>

namespace acl
{
template <typename T>
constexpr auto byteswap(T value) noexcept -> T
{
  if constexpr (sizeof(T) == sizeof(std::byte))
  {
    return value;
  }
  // static_assert(std::has_unique_object_representations_v<T>, "T may not have padding bits");
  auto value_representation = std::bit_cast<std::array<std::byte, sizeof(T)>>(value);
  std::ranges::reverse(value_representation);
  return std::bit_cast<T>(value_representation);
}

} // namespace acl