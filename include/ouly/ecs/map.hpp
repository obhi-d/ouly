// SPDX-License-Identifier: MIT
#pragma once

#include "ouly/containers/detail/indirection.hpp"
#include "ouly/ecs/entity.hpp"
#include "ouly/utility/detail/vector_abstraction.hpp"
#include "ouly/utility/optional_ref.hpp"

namespace ouly::ecs
{

template <typename EntityTy = ouly::ecs::entity<>, typename Config = ouly::default_config<EntityTy>>
class map
{
public:
  using entity_type    = EntityTy;
  using config         = Config;
  using size_type      = typename entity_type::size_type;
  using ssize_type     = std::make_signed_t<size_type>;
  using revision_type  = typename EntityTy::revision_type;
  using allocator_type = ouly::detail::custom_allocator_t<Config>;

private:
  static constexpr size_type tombstone = std::numeric_limits<size_type>::max();

  using this_type = map<entity_type, config>;

  struct default_index_pool_size
  {
    static constexpr uint32_t self_index_pool_size_v  = 1024;
    static constexpr bool     self_use_sparse_index_v = true;
    static constexpr uint32_t keys_index_pool_size_v  = 4096;
    static constexpr bool     keys_use_sparse_index_v = false;
  };

  struct self_index_traits
  {
    using size_type                        = ouly::detail::choose_size_t<uint32_t, Config>;
    static constexpr size_type pool_size_v = std::conditional_t<ouly::detail::HasSelfIndexPoolSize<config>, config,
                                                                default_index_pool_size>::self_index_pool_size_v;
    static constexpr bool      use_sparse_index_v =
     std::conditional_t<ouly::detail::HasSelfUseSparseIndexAttrib<config>, config,
                        default_index_pool_size>::self_use_sparse_index_v;
    static constexpr size_type null_v       = std::numeric_limits<size_type>::max();
    static constexpr bool      assume_pod_v = true;
  };

  struct key_index_traits
  {
    using size_type                       = ouly::detail::choose_size_t<uint32_t, Config>;
    static constexpr uint32_t pool_size_v = std::conditional_t<ouly::detail::HasKeysIndexPoolSize<config>, config,
                                                               default_index_pool_size>::keys_index_pool_size_v;
    static constexpr bool     use_sparse_index_v =
     std::conditional_t<ouly::detail::HasKeysUseSparseIndexAttrib<config>, config,
                        default_index_pool_size>::keys_use_sparse_index_v;
    static constexpr size_type null_v       = tombstone;
    static constexpr bool      assume_pod_v = true;
  };

  using self_index = ouly::detail::self_index_type<self_index_traits>;
  using key_index  = ouly::detail::indirection_type<key_index_traits>;

public:
  map() noexcept = default;
  map(allocator_type&& alloc) noexcept : allocator_type(std::move<allocator_type>(alloc)) {}
  map(allocator_type const& alloc) noexcept : allocator_type(alloc) {}
  map(map&& other) noexcept
  {
    *this = std::move(other);
  }
  map(map const& other) noexcept
  {
    *this = other;
  }
  ~map()
  {
    clear();
    shrink_to_fit();
  }

  auto operator=(map&& other) noexcept -> map&
  {
    if (this == &other)
    {
      return *this;
    }
    clear();
    shrink_to_fit();

    keys_ = std::move(other.keys_);
    self_ = std::move(other.self_);
    return *this;
  }

  auto operator=(map const& other) noexcept -> map&
  {
    if (this == &other)
    {
      return *this;
    }
    clear();
    shrink_to_fit();

    keys_ = other.keys_;
    self_ = other.self_;
    return *this;
  }

  /**
   * @brief Returns size of packed array
   */
  auto size() const noexcept -> size_type
  {
    return self_.size();
  }

  auto emplace(entity_type point) noexcept -> size_type
  {
    auto& k = keys_.ensure_at(point.get());
    k       = self_.size();
    self_.push_back(point.value());
    return k;
  }

  /**
   * @brief Erase a single element at `l`.
   * @return The index which needs to be swapped with the back of the values.
   * @note The final operation would require values to move the last element to returned index followed by a pop_back.
   */
  auto erase(entity_type l) noexcept -> size_type
  {
    if constexpr (ouly::debug)
    {
      validate(l);
    }
    return erase_at(l);
  }

  /**
   * @brief For indirectly mapped map, returns the key (index) associated with the entity
   * @return The key associated with the entity or `tombstone` if not found (which is
   * std::numeric_limits<uint32_t>::max())
   */
  auto key(entity_type point) const noexcept -> size_type
  {
    return keys_.get_if(point.get());
  }

  /**
   * @brief Returns a const reference to the internal keys container
   * @details This function is only available when the class does not use direct mapping
   * @return Const reference to the container holding the keys
   * @requires !has_direct_mapping
   */
  auto keys() const noexcept -> auto const&
  {
    return keys_;
  }

  /**
   * @brief Drop unused pages
   */
  void shrink_to_fit() noexcept
  {
    keys_.shrink_to_fit();
    self_.shrink_to_fit();
  }

  /**
   * @brief Set size to 0, memory is not released, objects are destroyed
   */
  void clear() noexcept
  {
    keys_.clear();
    self_.clear();
  }

  auto at(entity_type l) const noexcept -> size_type
  {
    if constexpr (ouly::debug)
    {
      validate(l);
    }
    return key(l.get());
  }

  auto operator[](entity_type l) const noexcept -> size_type
  {
    return at(l);
  }

  auto contains(entity_type l) const noexcept -> bool
  {
    auto idx = l.get();
    if (keys_.contains(idx))
    {
      auto val = keys_.get(idx);
      return self_.get(val) == l.value();
    }
    return false;
  }

  [[nodiscard]] auto empty() const noexcept -> bool
  {
    return self_.empty();
  }

  void validate_integrity() const
  {
    for (size_type first = 0, last = size(); first < last; ++first)
    {
      OULY_ASSERT(keys_.get(entity_type(get_ref_at_idx(first)).get()) == first);
    }

    for (size_type i = 0; i < keys_.size(); ++i)
    {
      if (keys_.contains(i))
      {
        OULY_ASSERT(keys_.get(i) < self_.size());
      }
    }
  }

  /**
   * @brief Sets the maximum size for the component storage
   *
   * If the storage has direct mapping, ensures the internal storage
   * can accommodate at least (size-1) elements. This operation is
   * a no-op if the storage doesn't use direct mapping.
   *
   * @param size The maximum number of elements to accommodate
   */
  void set_max(size_type size)
  {
    if (size)
    {
      keys_.resize(size, tombstone);
    }
  }

private:
  void validate(entity_type l) const noexcept
  {
    auto lnk  = l.get();
    auto idx  = keys_.get(lnk);
    auto self = self_.get(idx);
    OULY_ASSERT(self == l.value());
  }

  auto erase_at(entity_type l) noexcept -> size_type
  {
    auto lnk                                                = l.get();
    auto item_id                                            = keys_.get(lnk);
    keys_.get(lnk)                                          = tombstone;
    keys_.get(entity_type(self_.best_erase(item_id)).get()) = item_id;
    return item_id;
  }

  key_index  keys_;
  self_index self_;
};

} // namespace ouly::ecs
