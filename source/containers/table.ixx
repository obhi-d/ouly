#pragma once
#include "common.hpp"
#include "utils.hpp"
#include <acl/allocators/default_allocator.hpp>
#include <acl/containers/podvector.hpp>
#include <type_traits>
#include <vector>

namespace acl
{

template <DefaultConstructible T, bool IsPOD = std::is_trivial_v<T>>
class table
{
  struct free_idx
  {
    std::uint32_t unused = k_null_32;
    std::uint32_t valids = 0;
  };

  using vector   = std::conditional_t<IsPOD, podvector<T>, acl::vector<T>>;
  using freepool = std::conditional_t<IsPOD, free_idx, podvector<std::uint32_t>>;

public:
  table() noexcept        = default;
  table(table const&)     = default;
  table(table&&) noexcept = default;

  table& operator=(table const&)     = default;
  table& operator=(table&&) noexcept = default;

  template <typename... Args>
  std::uint32_t emplace(Args&&... args)
  {
    std::uint32_t index = 0;
    if constexpr (IsPOD)
    {
      if (free_pool.unused != k_null_32)
      {
        index            = free_pool.unused;
        free_pool.unused = reinterpret_cast<std::uint32_t&>(pool[free_pool.unused]);
      }
      else
      {
        index = static_cast<std::uint32_t>(pool.size());
        pool.resize(index + 1);
      }
      pool[index] = T(std::forward<Args>(args)...);
      free_pool.valids++;
    }
    else
    {
      if (!free_pool.empty())
      {
        index = free_pool.back();
        free_pool.pop_back();
        pool[index] = std::move(T(std::forward<Args>(args)...));
      }
      else
      {
        index = static_cast<std::uint32_t>(pool.size());
        pool.emplace_back(std::forward<Args>(args)...);
      }
    }
    return index;
  }

  void erase(std::uint32_t index)
  {
    if constexpr (IsPOD)
    {
      reinterpret_cast<std::uint32_t&>(pool[index]) = free_pool.unused;
      free_pool.unused                              = index;
      free_pool.valids--;
    }
    else
    {
      pool[index] = T();
      free_pool.emplace_back(index);
    }
  }

  T& operator[](std::uint32_t i)
  {
    return reinterpret_cast<T&>(pool[i]);
  }

  T const& operator[](std::uint32_t i) const
  {
    return reinterpret_cast<T const&>(pool[i]);
  }

  T& at(std::uint32_t i)
  {
    return reinterpret_cast<T&>(pool[i]);
  }

  T const& at(std::uint32_t i) const
  {
    return reinterpret_cast<T const&>(pool[i]);
  }

  std::uint32_t size() const
  {
    if constexpr (IsPOD)
      return free_pool.valids;
    else
      return static_cast<std::uint32_t>(pool.size() - free_pool.size());
  }

private:
  vector   pool;
  freepool free_pool;
};

} // namespace acl
