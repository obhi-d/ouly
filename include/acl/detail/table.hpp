#pragma once
#include "common.hpp"
#include "utils.hpp"
#include <acl/default_allocator.hpp>
#include <acl/podvector.hpp>
#include <type_traits>

namespace acl::detail
{
template <typename T>
using podvector_wrapper = acl::podvector<T>;

template <typename T, template <typename> typename VectorType = podvector_wrapper>
class table
{
public:
  template <typename... Args>
  std::uint32_t emplace(Args&&... args)
  {
    std::uint32_t index = 0;
    if (unused != k_null_32)
    {
      index  = unused;
      unused = reinterpret_cast<std::uint32_t&>(pool[unused]);
    }
    else
    {
      index = static_cast<std::uint32_t>(pool.size());
      pool.resize(index + 1);
    }
    new (&pool[index]) T(std::forward<Args>(args)...);
    valids++;
    return index;
  }

  void erase(std::uint32_t index)
  {
    auto& t = reinterpret_cast<T&>(pool[index]);
    t.~T();
    reinterpret_cast<std::uint32_t&>(pool[index]) = unused;
    unused                                        = index;
    valids--;
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
    return valids;
  }

private:
  using storage = detail::aligned_storage<sizeof(T), alignof(T)>;
  VectorType<storage> pool;
  std::uint32_t       unused = k_null_32;
  std::uint32_t       valids = 0;
};

} // namespace acl::detail
