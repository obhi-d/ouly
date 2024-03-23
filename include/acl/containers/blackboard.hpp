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
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

namespace acl
{
template <typename T>
concept InventoryDataType = std::is_trivial_v<T> || std::is_move_constructible_v<T>;

struct blackboard_offset
{
  using dtor = void (*)(void*);

  void* data       = nullptr;
  dtor  destructor = nullptr;
};

template <typename T>
concept HashMap = requires {
  typename T::name_map_type;
  requires std::same_as<typename T::name_map_type::mapped_type, blackboard_offset>;
  typename T::name_map_type::key_type;
};

namespace detail
{

template <typename H>
struct name_index_map
{
  using type = std::unordered_map<std::string, blackboard_offset>;
};

template <HashMap H>
struct name_index_map<H>
{
  using type = typename H::name_map_type;
};
} // namespace detail

namespace opt
{
template <HashMap H>
struct name_val_map
{
  using name_map_type = H;
};

// lookup option
template <template <typename K, typename V> class H>
struct map
{
  using name_map_type = typename H<std::type_index, blackboard_offset>::type;
};

template <template <typename V> class H>
struct name_map
{
  using name_map_type = typename H<blackboard_offset>::type;
};

} // namespace opt

/**
 * @brief Store data as name value pairs, value can be any blob of data, has no free memory management
 *
 * @remark Data is stored as a blob, names are stored seperately if required for lookup
 *         Data can also be retrieved by index.
 *         There is no restriction on the data type that is supported (POD or non-POD both are supported).
 */
template <typename Options = acl::options<>>
class blackboard : public detail::custom_allocator_t<Options>
{
  using dtor = bool (*)(void*);

  using options                             = Options;
  static constexpr auto total_atoms_in_page = detail::pool_size_v<options>;
  using allocator                           = detail::custom_allocator_t<options>;
  using base_type                           = allocator;
  using name_index_map                      = typename detail::name_index_map<options>::type;

public:
  using link           = void*;
  using clink          = void const*;
  using key_type       = typename name_index_map::key_type;
  using iterator       = typename name_index_map::iterator;
  using const_iterator = typename name_index_map::const_iterator;

  static constexpr bool is_type_indexed = std::is_same_v<key_type, std::type_index>;

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
                    if (el.second.destructor)
                    {
                      el.second.destructor(el.second.data);
                      el.second.destructor = nullptr;
                    }
                  });
    auto h = head;
    while (h)
    {
      auto n = h->pnext;
      acl::deallocate(*this, h, h->size + sizeof(arena));
      h = n;
    }
    lookup.clear();
    head    = nullptr;
    current = nullptr;
  }

  template <InventoryDataType T>
  T const& get() const noexcept
    requires(is_type_indexed)
  {
    return const_cast<blackboard&>(*this).get<T>(std::type_index(typeid(T)));
  }

  template <InventoryDataType T>
  T const& get(key_type v) const noexcept
  {
    return const_cast<blackboard&>(*this).get<T>(v);
  }

  template <InventoryDataType T>
  T const* get_if() const noexcept
    requires(is_type_indexed)
  {
    return const_cast<blackboard&>(*this).get_if<T>(std::type_index(typeid(T)));
  }

  template <InventoryDataType T>
  T const* get_if(key_type v) const noexcept
  {
    return const_cast<blackboard&>(*this).get_if<T>(v);
  }

  template <InventoryDataType T>
  static constexpr T const& at(clink index) noexcept
  {
    return *reinterpret_cast<T const*>(index);
  }

  template <InventoryDataType T>
  static constexpr T& at(link index) noexcept
  {
    return *reinterpret_cast<T*>(index);
  }

  template <InventoryDataType T>
  T& get(key_type k) noexcept
  {
    auto it = lookup.find(k);
    ACL_ASSERT(it != lookup.end());
    return *reinterpret_cast<T*>(it->second.data);
  }

  template <InventoryDataType T>
  T* get_if(key_type k) noexcept
  {
    auto it = lookup.find(k);
    if (it == lookup.end())
      return nullptr;
    return reinterpret_cast<T*>(it->second.data);
  }

  /**
   *
   */
  template <InventoryDataType T, typename... Args>
  auto emplace(Args&&... args) noexcept -> link
    requires(is_type_indexed)
  {
    return emplace(std::type_index(typeid(T)), std::forward<Args>(args)...);
  }

  template <InventoryDataType T, typename... Args>
  auto emplace(key_type k, Args&&... args) noexcept -> link
  {
    auto& lookup_ent = lookup[k];
    if (lookup_ent.destructor && lookup_ent.data)
      lookup_ent.destructor(lookup_ent.data);
    if (!lookup_ent.data)
      lookup_ent.data = allocate_space(sizeof(T), alignof(T));

    std::construct_at(reinterpret_cast<T*>(lookup_ent.data), std::forward<Args>(args)...);
    lookup_ent.destructor = std::is_trivially_destructible_v<T> ? &do_nothing : &destroy_at<T>;
    return lookup_ent.data;
  }

  template <InventoryDataType T>
  void erase() noexcept
    requires(is_type_indexed)
  {
    erase(std::type_index(typeid(T)));
  }

  void erase(key_type index) noexcept
  {
    auto it = lookup.find(index);
    if (it != lookup.end())
    {
      auto& lookup_ent = it->second;
      if (lookup_ent.destructor && lookup_ent.data)
        lookup_ent.destructor(lookup_ent.data);
      lookup_ent.destructor = nullptr;
    }
  }

  template <InventoryDataType T>
  void contains() const noexcept
    requires(is_type_indexed)
  {
    contains(std::type_index(typeid(T)));
  }

  bool contains(key_type index) const noexcept
  {
    auto it = lookup.find(index);
    return it != lookup.end() && it->second.destructor != nullptr;
  }

private:
  static void do_nothing(void*) {}

  struct arena
  {
    arena*   pnext     = nullptr;
    uint32_t size      = 0;
    uint32_t remaining = 0;
  };

  template <typename T>
  static void destroy_at(void* s)
  {
    reinterpret_cast<T*>(s)->~T();
  }

  void* allocate_space(size_t size, size_t alignment)
  {
    uint32_t req = static_cast<uint32_t>(size + (alignment - 1));
    if (!current || current->remaining < req)
    {
      uint32_t page_size   = std::max<uint32_t>(total_atoms_in_page, req);
      auto     new_current = acl::allocate<arena>(*this, sizeof(arena) + page_size);
      if (current)
        current->pnext = new_current;
      new_current->pnext     = nullptr;
      new_current->size      = static_cast<uint32_t>(page_size);
      new_current->remaining = static_cast<uint32_t>(page_size - req);
      current                = new_current;
      if (!head)
        head = current;
      return acl::align(new_current + 1, alignment);
    }
    else
    {

      void* ptr = reinterpret_cast<std::byte*>(current + 1) + (current->size - current->remaining);
      current->remaining -= req;
      return acl::align(ptr, alignment);
    }
  }

  arena*         head    = nullptr;
  arena*         current = nullptr;
  name_index_map lookup;
};
} // namespace acl
