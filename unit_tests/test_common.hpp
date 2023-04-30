
#pragma once
#include <compare>
#include <string>
#include <acl/type_traits.hpp>

template <typename IntTy>
inline IntTy range_rand(IntTy iBeg, IntTy iEnd)
{
  return static_cast<IntTy>(iBeg + (((double)rand() / (double)RAND_MAX) * (iEnd - iBeg)));
}

struct pod
{
  int a;
  int b;
  constexpr auto operator<=>(pod const&) const noexcept = default;
};


namespace acl
{
template <>
struct traits<std::string> : traits<>
{
  static constexpr std::uint32_t pool_size       = 2;
  static constexpr std::uint32_t index_pool_size = 2;
};

} // namespace acl


namespace helper
{
template <typename Cont>
static void insert(Cont& cont, std::uint32_t offset, std::uint32_t count)
{
  for (std::uint32_t i = 0; i < count; ++i)
  {
    cont.emplace(std::to_string(i + offset) + ".o");
  }
}
} // namespace helper
