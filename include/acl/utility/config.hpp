
#pragma once

#include <cassert>
#include <cstdint>

#if !defined(NDEBUG) || defined(_DEBUG)
#define ACL_DEBUG
#endif

namespace acl
{
#ifdef ACL_DEBUG
inline static constexpr bool debug = true;
#else
inline static constexpr bool debug = false;
#endif
} // namespace acl

#ifdef _MSC_VER
#define ACL_POTENTIAL_EMPTY_MEMBER [[msvc::no_unique_address]]
#else
#define ACL_POTENTIAL_EMPTY_MEMBER [[no_unique_address]]
#endif

#include <cassert>
#include <cstdio>
#include <cstdlib>

#ifdef _MSC_VER
#if defined(_M_X64) || defined(_M_IA64)
#define ACL_PACK_TAGGED_POINTER
#endif // defined(_M_X64) || defined(_M_IA64)
#endif // _MSC_VER

#ifdef __GNUC__
#if defined(__x86_64__)
#define ACL_PACK_TAGGED_POINTER
#endif // __GNUC__
#endif // defined(__x86_64__)

/**
 * Before including any config.hpp, if ACL_API is defined from a dll to exp
 */
#ifndef ACL_API
#define ACL_API
#endif

namespace acl
{

template <typename... Config>
struct config : public Config...
{};

template <>
struct config<>
{};

template <typename T>
struct default_config
{};
} // namespace acl

namespace acl::cfg
{

constexpr uint32_t default_pool_size = 4096;
/**
 * @brief Config to provide member pointer
 * @tparam M
 */
template <auto M>
struct member;

/**
 * @brief Specialization that provided class_type
 * @tparam T class_type for the member
 * @tparam M member_type for the member
 * @tparam MPtr Pointer to member
 */
template <typename T, typename M, M T::* MPtr>
struct member<MPtr>
{
  using class_type  = T;
  using member_type = M;
  using self_index  = cfg::member<MPtr>;

  static auto get(class_type& to) noexcept -> member_type&
  {
    return to.*MPtr;
  }

  static auto get(class_type const& to) noexcept -> member_type const&
  {
    return to.*MPtr;
  }
};

/**
 * @brief Config to control underlying pool size
 */
template <uint32_t PoolSize = default_pool_size>
struct pool_size
{
  static constexpr uint32_t pool_size_v = PoolSize;
};

/**
 * @brief Config to control the pool size of index maps used by container
 */
template <uint32_t PoolSize = default_pool_size>
struct index_pool_size
{
  static constexpr uint32_t index_pool_size_v = PoolSize;
};

/**
 * @brief Self index pool size controls the pool size for back references
 */
template <uint32_t PoolSize = default_pool_size>
struct self_index_pool_size
{
  static constexpr uint32_t self_index_pool_size_v = PoolSize;
};

/**
 * @brief Key index pool size controls the pool size for key indexes in tables
 */
template <uint32_t PoolSize = default_pool_size>
struct keys_index_pool_size
{
  static constexpr uint32_t keys_index_pool_size_v = PoolSize;
};

/**
 * @brief Null value, only applicable to constexpr types
 */
template <auto NullValue>
struct null_value
{
  static constexpr auto null_v = NullValue;
};

/**
 * @brief Indexes will have this as their size type
 */
template <typename T = uint32_t>
struct basic_size_type
{
  using size_type = T;
};

/**
 * @brief This option is used to indicated the projected member of a class to be used to store its own index in a sparse
 * or packed container. This will allow the container to do its book keeping for item deletion and addition.
 */
template <auto M>
using self_index_member = member<M>;

template <typename T = void>
struct basic_link_type
{
  using link_type = T;
};

struct assume_pod
{
  static constexpr bool assume_pod_v = true;
};

struct no_fill
{
  static constexpr bool no_fill_v = true;
};

struct trivially_destroyed_on_move
{
  static constexpr bool trivially_destroyed_on_move_v = true;
};

struct use_sparse
{
  static constexpr bool use_sparse_v = true;
};

struct use_sparse_index
{
  static constexpr bool use_sparse_index_v = true;
};

struct self_use_sparse_index
{
  static constexpr bool self_use_sparse_index_v = true;
};

struct keys_use_sparse_index
{
  static constexpr bool keys_use_sparse_index_v = true;
};

struct zero_out_memory
{
  static constexpr bool zero_out_memory_v = true;
};

struct disable_pool_tracking
{
  static constexpr bool disable_pool_tracking_v = true;
};

struct use_direct_mapping
{
  static constexpr bool use_direct_mapping_v = true;
};

// custom vector
template <typename T>
struct custom_vector
{
  using custom_vector_t = T;
};
} // namespace acl::cfg