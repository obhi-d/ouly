
#pragma once

#include <acl/reflection/detail/deduced_types.hpp>

namespace acl::detail
{

template <typename Cont, typename... Args>
static inline void emplace(Cont& container, size_t index, Args&&... args)

{
  if constexpr (requires { container.emplace(std::forward<Args>(args)...); })
  {
    container.emplace(std::forward<Args>(args)...);
  }
  else if constexpr (requires { container.emplace_back(std::forward<Args>(args)...); })
  {
    container.emplace_back(std::forward<Args>(args)...);
  }
  else if constexpr (requires { container.push_back(std::forward<Args>(args)...); })
  {
    container.push_back(std::forward<Args>(args)...);
  }
  else if constexpr (requires { container[index] = typename Cont::value_type(std::forward<Args>(args)...); })
  {
    if (index < std::size(container))
    {
      container[index] = typename Cont::value_type(std::forward<Args>(args)...);
    }
  }
}

template <typename Cont, typename SizeType = std::size_t>
static inline void reserve(Cont& container, SizeType sz)
{
  if constexpr (requires { container.reserve(sz); })
  {
    container.reserve(sz);
  }
}

template <typename Cont, typename SizeType = std::size_t>
static inline void resize(Cont& container, SizeType sz)
{
  if constexpr (requires { container.resize(sz); })
  {
    container.resize(sz);
  }
}

} // namespace acl::detail