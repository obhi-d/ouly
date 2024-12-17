#pragma once

#include <cstdlib>
#ifdef _MSC_VER
#include <malloc.h>
#endif

namespace acl::detail
{
inline void* malloc(std::size_t s)
{
  return std::malloc(s);
}

inline void* zmalloc(std::size_t s)
{
  auto z = std::malloc(s);
  assert(z);
  std::memset(z, 0, s);
  return z;
}

inline void free(void* v)
{
  std::free(v);
}

inline void* aligned_alloc(std::size_t alignment, std::size_t size)
{
#ifdef _MSC_VER
  return ::_aligned_malloc(size, alignment);
#else
  return std::aligned_alloc(alignment, size);
#endif
}

inline void* aligned_zalloc(std::size_t alignment, std::size_t size)
{
  auto z = aligned_alloc(alignment, size);
  std::memset(z, 0, size);
  return z;
}

inline void aligned_free(void* ptr)
{
#ifdef _MSC_VER
  return _aligned_free(ptr);
#else
  return std::free(ptr);
#endif
}

} // namespace acl::detail