module;

#ifdef _MSC_VER
#include <malloc.h>
#endif

#ifndef ACL_CUSTOM_MALLOC_NS
#include <acl/utility/malloc_ns.hpp>
#define ACL_CUSTOM_MALLOC_NS acl::detail
#else
namespace ACL_CUSTOM_MALLOC_NS
{
extern "C++" void* malloc(std::size_t s);
extern "C++" void* zmalloc(std::size_t s);
extern "C++" void  free(void* v);
extern "C++" void* aligned_alloc(std::size_t alignment, std::size_t size);
extern "C++" void  aligned_free(void* ptr);
} // namespace ACL_CUSTOM_MALLOC_NS
#endif

export module acl.utils:malloc_ns;

import <cstdlib>;

namespace acl::detail
{
inline void* malloc(std::size_t s)
{
  return std::malloc(s);
}

inline void* zmalloc(std::size_t s)
{
  auto z = std::malloc(s);
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

inline void* aligned_zmalloc(std::size_t alignment, std::size_t size)
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

export namespace acl
{

inline void* malloc(std::size_t s)
{
  return ACL_CUSTOM_MALLOC_NS::malloc(s);
}
inline void* zmalloc(std::size_t s)
{
  return ACL_CUSTOM_MALLOC_NS::zmalloc(s);
}
inline void free(void* f)
{
  return ACL_CUSTOM_MALLOC_NS::free(f);
}
inline void* aligned_alloc(std::size_t alignment, std::size_t size)
{
  ACL_ASSERT(alignment > 0);
  ACL_ASSERT((alignment & (alignment - 1)) == 0);
  return ACL_CUSTOM_MALLOC_NS::aligned_alloc(alignment, size);
}
inline void* aligned_zmalloc(std::size_t alignment, std::size_t size)
{
  ACL_ASSERT(alignment > 0);
  ACL_ASSERT((alignment & (alignment - 1)) == 0);
  return ACL_CUSTOM_MALLOC_NS::aligned_zmalloc(alignment, size);
}
inline void aligned_free(void* ptr)
{
  return ACL_CUSTOM_MALLOC_NS::aligned_free(ptr);
}

} // namespace acl
