#pragma once
#include "podvector.hpp"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <type_traits>
#include <unordered_map>

namespace acl
{
template <typename T>
concept BlackboardDataType = std::is_pod_v<T>;

/// @brief Store data as name value pairs, value being POD type
///
/// Data is stored as a blob, names are stored seperately if required for lookup
/// Data can also be retrieved by index.
/// Data type should all be POD
/// Use @inventory if non-POD type data is required.
template <typename StringType = std::string_view>
class blackboard
{

public:
  struct alignas(64) atom_t
  {
    std::uint64_t first;
    std::uint64_t second;
  };

  template <typename T>
  static constexpr bool is_inlined = (sizeof(T) <= sizeof(atom_t));

  template <BlackboardDataType T>
  T const& at(StringType name) const noexcept
  {
    auto it = lookup.find(name);
    assert(it != lookup.end());
    return at<T>(static_cast<std::uint32_t>(it->second));
  }

  template <BlackboardDataType T>
  T const& at(std::uint32_t index) const noexcept
  {
    auto const& idx = offsets[index];
    if constexpr (sizeof(T) > sizeof(atom_t))
      return *reinterpret_cast<T const*>(&values[idx.first]);
    else
      return *reinterpret_cast<T const*>(&idx);
  }

  template <BlackboardDataType T, typename... Args>
  std::uint32_t emplace(StringType name, Args&&... args) noexcept
  {
    assert(lookup.find(name) == lookup.end());

    if constexpr (is_inlined<T>)
    {
      auto& entry = make_offset_entry();
      std::construct_at(reinterpret_cast<T*>(std::addressof(entry)), std::forward<Args>(args)...);
    }
    else
    {
      atom_t idx{.first = values.size(), .second = 0};
      values.resize(values.size() + atom_count(sizeof(T)));
      std::construct_at(reinterpret_cast<T*>(&values[idx.first]), std::forward<Args>(args)...);
      idx.second          = 1;
      make_offset_entry() = idx;
    }
    auto index   = static_cast<std::uint32_t>(offsets.size() - 1);
    lookup[name] = index;
    return index;
  }

  ///
  template <BlackboardDataType T, typename... Args>
  auto emplace_safe(StringType name, Args&&... args) noexcept -> std::uint32_t
  {
    auto existing = lookup.find(name);
    if (existing != lookup.end())
    {
      existing->second = std::abs(existing->second);
      return emplace_safe<T>(static_cast<std::uint32_t>(existing->second), std::forward<Args>(args)...);
    }
    else
    {
      return emplace<T>(name, std::forward<Args>(args)...);
    }
  }

  template <BlackboardDataType T, typename... Args>
  auto emplace_safe(std::uint32_t index, Args&&... args) noexcept -> std::uint32_t
  {
    if constexpr (is_inlined<T>)
    {
      std::construct_at(reinterpret_cast<T*>(&offsets[index]), std::forward<Args>(args)...);
    }
    else
    {
      std::construct_at(reinterpret_cast<T*>(&values[offsets[index].first]), std::forward<Args>(args)...);
    }
    return index;
  }

  template <BlackboardDataType T>
  void erase(StringType name) noexcept
  {
    auto it = lookup.find(name);
    if (it != lookup.end())
    {
      erase<T>(static_cast<std::uint32_t>(it->second));
      it->second = -it->second;
    }
  }

  template <BlackboardDataType T>
  void erase(std::uint32_t id) noexcept
  {
    if constexpr (is_inlined<T>)
      std::memset(&offsets[id], 0, sizeof(atom_t));
    else
    {
      offsets[id].second = 0;
    }
  }

  bool contains(StringType name) const noexcept
  {
    auto it = lookup.find(name);
    return (it != lookup.end() && it->second >= 0);
  }

private:
  constexpr inline auto& make_offset_entry()
  {
    offsets.emplace_back();
    return offsets.back();
  }

  static constexpr inline auto atom_count(auto size)
  {
    return ((size + sizeof(atom_t) - 1) / sizeof(atom_t));
  }

  using value_list     = podvector<atom_t>;
  using name_index_map = std::unordered_map<StringType, std::int32_t>;
  using index_list     = podvector<atom_t>;

  value_list     values;
  index_list     offsets;
  name_index_map lookup;
};
} // namespace acl