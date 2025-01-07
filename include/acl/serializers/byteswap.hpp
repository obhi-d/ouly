#include <algorithm>
#include <array>
#include <bit>
#include <concepts>

namespace acl
{
template <std::integral T>
constexpr auto byteswap(T value) noexcept -> T
{
  static_assert(std::has_unique_object_representations_v<T>, "T may not have padding bits");
  auto value_representation = std::bit_cast<std::array<std::byte, sizeof(T)>>(value);
  std::ranges::reverse(value_representation);
  return std::bit_cast<T>(value_representation);
}

} // namespace acl