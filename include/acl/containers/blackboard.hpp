#pragma once

#include <acl/allocators/allocator.hpp>
#include <acl/allocators/default_allocator.hpp>
#include <acl/allocators/detail/custom_allocator.hpp>
#include <acl/containers/config.hpp>
#include <acl/containers/detail/blackboard_defs.hpp>
#include <acl/utility/config.hpp>
#include <acl/utility/utils.hpp>
#include <cassert>
#include <cstdint>
#include <memory>

namespace acl
{

/**
 * @brief Store data as name value pairs, value can be any blob of data, has no free memory management
 *
 * @remark Data is stored as a blob, names are stored seperately if required for lookup
 *         Data can also be retrieved by index.
 *         There is no restriction on the data type that is supported (POD or non-POD both are supported).
 */
template <typename Config = acl::config<>>
class blackboard : public acl::detail::custom_allocator_t<Config>
{
  using dtor = bool (*)(void*);

  using config                              = Config;
  static constexpr auto total_atoms_in_page = acl::detail::pool_size_v<config>;
  using allocator                           = acl::detail::custom_allocator_t<config>;
  using base_type                           = allocator;
  using name_index_map                      = typename acl::detail::name_index_map<config>::type;

public:
  using link           = void*;
  using clink          = void const*;
  using key_type       = typename name_index_map::key_type;
  using iterator       = typename name_index_map::iterator;
  using const_iterator = typename name_index_map::const_iterator;

  static constexpr bool is_type_indexed = std::is_same_v<key_type, std::type_index>;

  blackboard() noexcept = default;
  blackboard(allocator&& alloc) noexcept : base_type(std::move<allocator>(alloc)) {}
  blackboard(allocator const& alloc) noexcept : base_type(alloc) {}
  blackboard(blackboard&& other) noexcept                         = default;
  blackboard(blackboard const& other) noexcept                    = delete;
  auto operator=(blackboard&& other) noexcept -> blackboard&      = default;
  auto operator=(blackboard const& other) noexcept -> blackboard& = delete;
  ~blackboard()
  {
    clear();
  }

  void clear()
  {
    std::for_each(lookup_.begin(), lookup_.end(),
                  [this](auto& el)
                  {
                    if (el.second.destructor_)
                    {
                      el.second.destructor_(el.second.data_);
                      el.second.destructor_ = nullptr;
                    }
                  });
    auto h = head_;
    while (h)
    {
      auto n = h->pnext_;
      acl::deallocate(*this, h, h->size_ + sizeof(arena));
      h = n;
    }
    lookup_.clear();
    head_    = nullptr;
    current_ = nullptr;
  }

  template <typename T>
  auto get() const noexcept -> T const&
    requires(is_type_indexed)
  {
    return get<T>(std::type_index(typeid(T)));
  }

  template <typename T>
  auto get() noexcept -> T& requires(is_type_indexed) { return get<T>(std::type_index(typeid(T))); }

  template <typename T>
  auto get(key_type v) const noexcept -> T const&
  {
    // NOLINTNEXTLINE
    return const_cast<blackboard&>(*this).get<T>(v);
  }

  template <typename T>
  auto get(key_type k) noexcept -> T&
  {
    auto it = lookup_.find(k);
    assert(it != lookup_.end());
    // NOLINTNEXTLINE
    return *reinterpret_cast<T*>(it->second.data_);
  }

  template <typename T>
  auto get_if() const noexcept -> T const* requires(is_type_indexed) { return get_if<T>(std::type_index(typeid(T))); }

  template <typename T>
  auto get_if() noexcept -> T* requires(is_type_indexed) { return get_if<T>(std::type_index(typeid(T))); }

  template <typename T>
  auto get_if(key_type v) const noexcept -> T const*
  {
    // NOLINTNEXTLINE
    return const_cast<blackboard&>(*this).get_if<T>(v);
  }

  template <typename T>
  auto get_if(key_type k) noexcept -> T*
  {
    auto it = lookup_.find(k);
    if (it == lookup_.end())
    {
      return nullptr;
    }
    // NOLINTNEXTLINE
    return reinterpret_cast<T*>(it->second.data_);
  }

  /**
   *
   */
  template <typename T, typename... Args>
  auto emplace(Args&&... args) noexcept
   -> T& requires(is_type_indexed) { return emplace<T>(std::type_index(typeid(T)), std::forward<Args>(args)...); }

  template <typename T, typename... Args>
  auto emplace(key_type k, Args&&... args) noexcept -> T&
  {
    auto& lookup_ent = lookup_[k];
    if (lookup_ent.destructor_ && lookup_ent.data_)
    {
      lookup_ent.destructor_(lookup_ent.data_);
    }
    if (!lookup_ent.data_)
    {
      lookup_ent.data_ = allocate_space(sizeof(T), alignof(T));
    }

    // NOLINTNEXTLINE
    std::construct_at(reinterpret_cast<T*>(lookup_ent.data_), std::forward<Args>(args)...);
    lookup_ent.destructor_ = std::is_trivially_destructible_v<T> ? &do_nothing : &destroy_at<T>;
    // NOLINTNEXTLINE
    return *reinterpret_cast<T*>(lookup_ent.data_);
  }

  template <typename T>
  void erase() noexcept
    requires(is_type_indexed)
  {
    erase(std::type_index(typeid(T)));
  }

  void erase(key_type index) noexcept
  {
    auto it = lookup_.find(index);
    if (it != lookup_.end())
    {
      auto& lookup_ent = it->second;
      if (lookup_ent.destructor_ && lookup_ent.data_)
      {
        lookup_ent.destructor_(lookup_ent.data_);
      }
      lookup_ent.destructor_ = nullptr;
    }
  }

  template <typename T>
  void contains() const noexcept
    requires(is_type_indexed)
  {
    contains(std::type_index(typeid(T)));
  }

  auto contains(key_type index) const noexcept -> bool
  {
    auto it = lookup_.find(index);
    return it != lookup_.end() && it->second.destructor_ != nullptr;
  }

private:
  static void do_nothing(void* /*unused*/) {}

  struct arena
  {
    arena*   pnext_     = nullptr;
    uint32_t size_      = 0;
    uint32_t remaining_ = 0;
  };

  template <typename T>
  static void destroy_at(void* s)
  {
    // NOLINTNEXTLINE
    reinterpret_cast<T*>(s)->~T();
  }

  auto allocate_space(size_t size_, size_t alignment) -> void*
  {
    auto req = static_cast<uint32_t>(size_ + (alignment - 1));
    if ((current_ == nullptr) || current_->remaining_ < req)
    {
      uint32_t page_size   = std::max<uint32_t>(total_atoms_in_page, req);
      auto     new_current = acl::allocate<arena>(*this, sizeof(arena) + page_size);
      if (current_)
      {
        current_->pnext_ = new_current;
      }
      new_current->pnext_     = nullptr;
      new_current->size_      = page_size;
      new_current->remaining_ = (page_size - req);
      current_                = new_current;
      if (head_ == nullptr)
      {
        head_ = current_;
      }
      return acl::align(new_current + 1, alignment);
    }
    // NOLINTNEXTLINE
    void* ptr = reinterpret_cast<std::byte*>(current_ + 1) + (current_->size_ - current_->remaining_);
    current_->remaining_ -= req;
    return acl::align(ptr, alignment);
  }

  arena*         head_    = nullptr;
  arena*         current_ = nullptr;
  name_index_map lookup_;
};
} // namespace acl
