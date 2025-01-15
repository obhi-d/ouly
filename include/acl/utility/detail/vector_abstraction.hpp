#pragma once
#include <acl/utility/type_traits.hpp>
#include <type_traits>

namespace acl::detail
{

template <typename Class>
concept IsSparseVector = Class::is_sparse_vector;

template <typename Class>
concept IsNormalVector = !IsSparseVector<Class>;

template <IsSparseVector V, typename I, typename... Args>
auto emplace_at(V& vector, I i, Args&&... args) -> auto&
{
  return vector.emplace_at(i, std::forward<Args>(args)...);
}

template <IsNormalVector V, typename I, typename... Args>
auto emplace_at(V& vector, I i, Args&&... args) -> auto&
{
  if (i >= vector.size())
  {
    vector.resize(i + 1);
  }
  return vector[i] = typename V::value_type(std::forward<Args>(args)...);
}

template <IsSparseVector V, typename I, typename Args>
auto replace_at(V& vector, I i, Args&& args) -> auto&
{
  if (vector.contains(i))
  {
    return vector[i] = std::forward<Args>(args);
  }
  return emplace_at(vector, i, std::forward<Args>(args));
}

template <IsNormalVector V, typename I, typename Args>
auto replace_at(V& vector, I i, Args&& args) -> auto&
{
  if (i < vector.size())
  {
    return vector[i] = std::forward<Args>(args);
  }
  return emplace_at(vector, i, std::forward<Args>(args));
}

template <IsSparseVector V, typename I>
auto ensure_at(V& vector, I i) -> auto&
{
  if (vector.contains(i))
  {
    return vector[i];
  }
  return emplace_at(vector, i);
}

template <IsNormalVector V, typename I, typename Args>
auto ensure_at(V& vector, I i, Args&& args) -> auto&
{
  if (i >= vector.size())
  {
    vector.resize(i + 1, std::forward<Args>(args));
  }
  return vector[i];
}

template <IsSparseVector V, typename I>
auto get_if(V& vector, I i) -> auto*
{
  return vector.get_if(i);
}

template <IsNormalVector V, typename I>
auto get_if(V& vector, I i)
 -> std::conditional_t<std::is_const_v<V>, typename V::value_type const*, typename V::value_type*>
{
  if (i < vector.size())
  {
    return &vector[i];
  }
  return nullptr;
}

template <IsSparseVector V, typename I, typename Val>
auto get_or(V& vector, I i, Val value)
{
  return vector.get_or(i, value);
}

template <IsNormalVector V, typename I, typename Val>
auto get_or(V& vector, I i, Val value)
{
  if (i < vector.size())
  {
    return vector[i];
  }
  return value;
}

} // namespace acl::detail
