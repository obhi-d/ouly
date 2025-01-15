#pragma once

#include <cstdint>

namespace acl::cfg
{
struct mutate_enums
{
  using mutate_enums_type = void;
};

#ifdef ACL_USE_LARGE_SIZE_TYPE
using container_size_type = uint64_t;
#else
using container_size_type = uint32_t;
#endif

constexpr uint8_t null_sentinel     = '?';
constexpr uint8_t not_null_sentinel = '!';

/** This is a helper magic number that can be specialized for types to aid in binary serialization with magic number
 * entries */
template <typename T>
constexpr uint32_t magic_type_header = 0;

constexpr uint32_t default_lite_yml_parser_buffer_size = 8096;
} // namespace acl::cfg