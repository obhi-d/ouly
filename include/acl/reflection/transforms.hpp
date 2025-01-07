#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace acl
{

// Type to string conversion customization
template <typename T>
void from_string(T& ref, std::string_view) = delete;

template <typename T>
auto to_string(T const& ref) -> std::string = delete;

template <typename T>
auto to_string_view(T const& ref) -> std::string_view = delete;

// Variant type transform
template <typename T>
auto to_variant_index(std::string_view ref) -> uint32_t = delete;
template <typename T>
auto from_variant_index(std::size_t ref) -> std::string_view = delete;

template <>
inline void from_string<std::string>(std::string& ref, std::string_view v)
{
  ref = std::string(v);
}

template <>
inline auto to_string_view<std::string>(std::string const& ref) -> std::string_view
{
  return ref;
}

} // namespace acl