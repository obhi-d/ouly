#pragma once
#include "allocator.hpp"
#include "default_allocator.hpp"
#include "detail/config.hpp"
#include "detail/utils.hpp"
#include "podvector.hpp"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <type_traits>
#include <unordered_map>

namespace acl
{
template <typename T>
concept InventoryDataType = std::is_trivial_v<T> || std::is_move_constructible_v<T>;

/// @brief Store data as name value pairs, value being POD type
///
/// Data is stored as a blob, names are stored seperately if required for lookup
/// Data can also be retrieved by index.
/// There is no restriction on the data type that is supported.
template <typename Allocator = default_allocator<>, typename StringType = std::string_view,
          std::size_t const PoolSize = 1024>
class greenboard : public Allocator
{
  using dtor = bool (*)(void*);

  struct atom_t
  {
    void* data;
    dtor  dtor_fn;
  };

  enum class state
  {
    external_alive,
    destroyed,
    inlined_alive
  };

  using storage                             = detail::aligned_storage<sizeof(atom_t), alignof(atom_t)>;
  static constexpr auto total_atoms_in_page = PoolSize;
  using allocator                           = Allocator;
  using lookup_entry                        = std::pair<std::uint32_t, state>;
  using base_type                           = Allocator;

public:
  template <typename T>
  static constexpr bool is_inlined = (sizeof(T) <= sizeof(atom_t)) && std::is_trivial_v<T>;

  inline greenboard() noexcept {}
  inline greenboard(Allocator&& alloc) noexcept : base_type(std::move<Allocator>(alloc)) {}
  inline greenboard(Allocator const& alloc) noexcept : base_type(alloc) {}
  inline greenboard(greenboard&& other) noexcept      = default;
  inline greenboard(greenboard const& other) noexcept = delete;
  greenboard& operator=(greenboard&& other) noexcept = default;
  greenboard& operator=(greenboard const& other) noexcept = delete;
  ~greenboard()
  {
    clear();
  }

  void clear()
  {
    std::for_each(lookup.begin(), lookup.end(),
                  [this](auto& el)
                  {
                    if (el.second.second == state::external_alive)
                    {
                      auto& atom = offsets[el.second.first];
                      atom.dtor_fn(atom.data);
                    }
                  });
    std::for_each(managed_data.begin(), managed_data.end(),
                  [this](auto entry)
                  {
                    acl::deallocate(*this, entry, total_atoms_in_page * sizeof(atom_t));
                  });

    lookup.clear();
    managed_data.clear();
    offsets.clear();
    availabile_atoms = 0;
  }

  template <InventoryDataType T>
  T const& at(StringType name) const noexcept
  {
    auto it = lookup.find(name);
    assert(it != lookup.end());
    return at<T>(it->second.first);
  }

  template <InventoryDataType T>
  T const& at(std::uint32_t index) const noexcept
  {
    if constexpr (is_inlined<T>)
      return *reinterpret_cast<T const*>(&offsets[index]);
    else
    {
      auto const& idx = offsets[index];
      return *reinterpret_cast<T const*>(idx.data);
    }
  }

  template <InventoryDataType T, typename... Args>
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
      atom_t atom = ensure<T>();
      std::construct_at(reinterpret_cast<T*>(atom.data), std::forward<Args>(args)...);
      make_offset_entry() = atom;
    }
    auto index   = static_cast<std::uint32_t>(offsets.size() - 1);
    lookup[name] = lookup_entry(index, is_inlined<T> ? state::inlined_alive : state::external_alive);
    return index;
  }

  ///
  template <InventoryDataType T, typename... Args>
  auto emplace_safe(StringType name, Args&&... args) noexcept -> std::uint32_t
  {
    auto existing = lookup.find(name);
    if (existing != lookup.end())
    {
      auto& lookup_ent = existing->second;
      if constexpr (is_inlined<T>)
      {
        std::construct_at(reinterpret_cast<T*>(&offsets[lookup_ent.first]), std::forward<Args>(args)...);
        lookup_ent.second = state::inlined_alive;
      }
      else
      {
        auto& atom = offsets[lookup_ent.first];
        if constexpr (acl::detail::debug)
        {
          assert(atom.dtor_fn == reinterpret_cast<dtor>(&destroy_at<T>));
        }
        T* data = reinterpret_cast<T*>(atom.data);
        if (lookup_ent.second == state::destroyed)
        {
          std::construct_at(data, std::forward<Args>(args)...);
          lookup_ent.second = state::external_alive;
        }
        else
        {
          if constexpr (std::is_move_assignable_v<T>)
          {
            *data = T(std::forward<Args>(args)...);
          }
          else
          {
            std::destroy_at(data);
            std::construct_at(data, std::forward<Args>(args)...);
          }
        }
      }
      return lookup_ent.first;
    }
    else
    {
      return emplace<T>(name, std::forward<Args>(args)...);
    }
  }

  template <typename T>
  void erase(StringType name) noexcept
  {
    auto it = lookup.find(name);
    if (it != lookup.end() && it->second.second != state::destroyed)
    {
      erase<T>(it->second.first);
      it->second.second = state::destroyed;
    }
  }

  bool contains(StringType name) const noexcept
  {
    auto it = lookup.find(name);
    return (it != lookup.end() && it->second.second != state::destroyed);
  }

private:
  template <typename T>
  static void destroy_at(void* s)
  {
    reinterpret_cast<T*>(s)->~T();
  }

  template <typename T>
  void erase(std::uint32_t id) noexcept
  {
    if constexpr (!is_inlined<T>)
    {
      std::destroy_at(reinterpret_cast<T*>(offsets[id].data));
    }
  }

  constexpr inline auto& make_offset_entry()
  {
    offsets.emplace_back();
    return offsets.back();
  }

  static constexpr inline auto atom_count(auto size)
  {
    return ((size + sizeof(atom_t) - 1) / sizeof(atom_t));
  }

  template <typename T>
  inline atom_t ensure() noexcept
  {
    constexpr auto atoms_req       = atom_count(sizeof(T));
    auto           num_total_atoms = total_atoms_in_page;
    if (availabile_atoms < atoms_req)
    {
      static_assert(total_atoms_in_page >= atoms_req, "T is too big, increaase pool size");
      constexpr auto page_size = std::max(total_atoms_in_page, atoms_req);
      num_total_atoms          = page_size;
      managed_data.emplace_back(acl::allocate<storage>(*this, sizeof(storage) * page_size));
    }
    auto ret = managed_data.back() + (num_total_atoms - availabile_atoms);
    availabile_atoms -= atoms_req;

    return atom_t{.data = ret, .dtor_fn = reinterpret_cast<dtor>(&destroy_at<T>)};
  }

  using name_index_map = std::unordered_map<StringType, std::pair<std::uint32_t, state>>;
  using index_list     = podvector<atom_t>;

  podvector<storage*> managed_data;
  index_list          offsets;
  name_index_map      lookup;
  std::size_t         availabile_atoms = 0;
};
} // namespace acl