
#pragma once

#include <acl/containers/blackboard_offset.hpp>
#include <algorithm>
#include <concepts>
#include <string>
#include <unordered_map>

namespace acl::detail
{
template <typename T>
concept BlackboardHashMap = requires(T& obj, T const& cobj) {
  typename T::key_type;
  requires std::same_as<typename T::mapped_type, blackboard_offset>;
  typename T::iterator;
  typename T::const_iterator;
  { obj.find(std::declval<typename T::key_type>()) } -> std::same_as<typename T::iterator>;
  { cobj.find(std::declval<typename T::key_type>()) } -> std::same_as<typename T::const_iterator>;

  obj.erase(obj.find(std::declval<typename T::key_type>()));
  { obj[std::declval<typename T::key_type>()] } -> std::same_as<typename T::mapped_type&>;
  requires std::same_as<typename T::mapped_type, blackboard_offset>;
};

template <typename T>
concept HashMapDeclTraits = requires {
  typename T::name_map_type;
  requires BlackboardHashMap<typename T::name_map_type>;
};

template <typename H>
struct name_index_map
{
  using type = std::unordered_map<std::string, blackboard_offset>;
};

template <HashMapDeclTraits H>
struct name_index_map<H>
{
  using type = typename H::name_map_type;
};
} // namespace acl::detail
