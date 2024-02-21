#pragma once
#include "podvector.hpp"
#include <acl/allocators/allocator.hpp>
#include <acl/allocators/default_allocator.hpp>
#include <acl/utils/config.hpp>
#include <acl/utils/link.hpp>
#include <acl/utils/utils.hpp>
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

template <typename T>
concept HashMap = requires {
  typename T::name_map_type;
  requires std::same_as<typename T::name_map_type::mapped_type, acl::vlink>;
  typename T::name_map_type::key_type;
};
namespace detail
{
template <typename H>
struct name_index_map
{
  using type = std::unordered_map<std::string, vlink>;
};

template <HashMap H>
struct name_index_map<H>
{
  using type = typename H::name_map_type;
};
} // namespace detail

namespace opt
{
// lookup option
template <typename N>
struct name_map
{
  using name_map_type = N;
};

} // namespace opt

/**
 * @brief Store data as name value pairs, value can be any blob of data
 *
 * @remark Data is stored as a blob, names are stored seperately if required for lookup
 *         Data can also be retrieved by index.
 *         There is no restriction on the data type that is supported (POD or non-POD both are supported).
 */
template <typename Options = acl::options<>>
class blackboard : public detail::custom_allocator_t<Options>
{
  using dtor = bool (*)(void*);

  struct atom_t
  {
    void* data;
    dtor  dtor_fn;
  };

  using options                                    = Options;
  static constexpr std::size_t total_atoms_in_page = detail::pool_size_v<options>;
  using allocator                                  = detail::custom_allocator_t<options>;
  using base_type                                  = allocator;
  using name_index_map                             = typename detail::name_index_map<options>::type;

  static constexpr std::uint64_t inlined_mask_v = 0x8000000000000000;
  static constexpr std::uint64_t deleted_mask_v = 0x4000000000000000;

public:
  using link           = vlink;
  using key_type       = typename name_index_map::key_type;
  using iterator       = typename name_index_map::iterator;
  using const_iterator = typename name_index_map::const_iterator;

  template <typename T>
  static constexpr bool is_inlined = (sizeof(T) <= sizeof(atom_t)) && std::is_trivial_v<T>;

  inline blackboard() noexcept {}
  inline blackboard(allocator&& alloc) noexcept : base_type(std::move<allocator>(alloc)) {}
  inline blackboard(allocator const& alloc) noexcept : base_type(alloc) {}
  inline blackboard(blackboard&& other) noexcept          = default;
  inline blackboard(blackboard const& other) noexcept     = delete;
  blackboard& operator=(blackboard&& other) noexcept      = default;
  blackboard& operator=(blackboard const& other) noexcept = delete;
  ~blackboard()
  {
    clear();
  }

  void clear()
  {
    std::for_each(lookup.begin(), lookup.end(),
                  [this](auto& el)
                  {
                    if (!el.second.has_mask(inlined_mask_v | deleted_mask_v))
                    {
                      auto& atom = offsets[el.second.value()];
                      atom.dtor_fn(atom.data);
                    }
                  });
    std::for_each(managed_data.begin(), managed_data.end(),
                  [this](auto entry)
                  {
                    acl::deallocate(*this, entry, total_atoms_in_page * sizeof(atom_t), 16);
                  });

    lookup.clear();
    managed_data.clear();
    offsets.clear();
    availabile_atoms = 0;
  }

  template <InventoryDataType T>
  T const& get(key_type name) const noexcept
  {
    return const_cast<blackboard&>(*this).get<T>(name);
  }

  template <InventoryDataType T>
  T const* get_if(key_type name) const noexcept
  {
    return const_cast<blackboard&>(*this).get_if<T>(name);
  }

  template <InventoryDataType T>
  T const& at(link index) const noexcept
  {
    return const_cast<blackboard&>(*this).at<T>(index);
  }

  template <InventoryDataType T>
  T& get(key_type name) noexcept
  {
    auto it = lookup.find(name);
    ACL_ASSERT(it != lookup.end());
    return at<T>(it->second);
  }

  template <InventoryDataType T>
  T& at(vlink index) noexcept
  {
    if constexpr (is_inlined<T>)
      return *reinterpret_cast<T*>(&offsets[index.unmasked()]);
    else
    {
      auto& idx = offsets[index.value()];
      return *std::launder(reinterpret_cast<T*>(idx.data));
    }
  }

  template <InventoryDataType T>
  T* get_if(key_type name) noexcept
  {
    auto it = lookup.find(name);
    if (it == lookup.end())
      return nullptr;
    return &at<T>(it->second);
  }

  /**
   *
   */
  template <InventoryDataType T, typename... Args>
  auto emplace(key_type name, Args&&... args) noexcept -> link
  {
    auto existing = lookup.find(name);
    if (existing != lookup.end())
    {
      auto& lookup_ent = existing->second;
      lookup_ent.unmask();
      if constexpr (is_inlined<T>)
      {
        std::construct_at(reinterpret_cast<T*>(&offsets[lookup_ent.unmasked()]), std::forward<Args>(args)...);
        lookup_ent.mask(inlined_mask_v);
      }
      else
      {
        auto& atom = offsets[lookup_ent.value()];
        if constexpr (acl::detail::debug)
        {
          ACL_ASSERT(atom.dtor_fn == reinterpret_cast<dtor>(&destroy_at<T>));
        }

        T* data = reinterpret_cast<T*>(atom.data);
        atom.dtor_fn(atom.data);
        std::construct_at(data, std::forward<Args>(args)...);
      }
      return lookup_ent;
    }
    else
    {
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

      auto index   = is_inlined<T> ? vlink((offsets.size() - 1) | inlined_mask_v) : vlink(offsets.size() - 1);
      lookup[name] = index;
      return index;
    }
  }

  void erase(key_type name) noexcept
  {
    auto it = lookup.find(name);
    if (it != lookup.end())
      erase(it->second);
  }

  bool contains(key_type name) const noexcept
  {
    auto it = lookup.find(name);
    return (it != lookup.end() && !it->second.has_mask(deleted_mask_v));
  }

private:
  static void do_nothing(void*) {}

  template <typename T>
  static void destroy_at(void* s)
  {
    reinterpret_cast<T*>(s)->~T();
  }

  void erase(vlink& id) noexcept
  {
    auto& atom = offsets[id.unmasked()];
    if (!id.has_mask(inlined_mask_v))
    {
      atom.dtor_fn(atom.data);
      atom.dtor_fn = reinterpret_cast<dtor>(&do_nothing);
    }
    id.mask(deleted_mask_v);
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
    constexpr auto atoms_req       = atom_count(sizeof(T) + (alignof(T) - 1));
    auto           num_total_atoms = total_atoms_in_page;
    if (availabile_atoms < atoms_req)
    {
      static_assert(total_atoms_in_page >= atoms_req, "T is too big, increaase pool size");
      constexpr auto page_size = std::max(total_atoms_in_page, atoms_req);
      availabile_atoms = num_total_atoms = page_size;
      managed_data.emplace_back(acl::allocate<atom_t>(*this, sizeof(atom_t) * page_size, 16));
    }
    void* ret = managed_data.back() + (num_total_atoms - availabile_atoms);
    availabile_atoms -= atoms_req;
    std::size_t size = sizeof(T) + (alignof(T) - 1);
    return atom_t{.data    = std::align(alignof(T), sizeof(T), ret, size),
                  .dtor_fn = reinterpret_cast<dtor>(&destroy_at<T>)};
  }

  using index_list = podvector<atom_t, acl::options<opt::basic_size_type<std::size_t>>>;

  podvector<atom_t*> managed_data;
  index_list         offsets;
  name_index_map     lookup;
  std::size_t        availabile_atoms = 0;
};
} // namespace acl
