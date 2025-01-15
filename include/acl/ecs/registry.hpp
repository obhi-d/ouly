
#pragma once

#include <acl/ecs/detail/registry_defs.hpp>
#include <acl/ecs/entity.hpp>
#include <algorithm>
#include <cassert>
#include <span>

namespace acl::ecs
{

/**
 * @brief This class stores a list of reusable links, and allows for vector allocating objects on a seperate container
 * based on the links. This class also stores link revisions that are updated when links are deleted, and allows for
 * verification if links are still alive.
 * @tparam Ty
 * @tparam SizeType
 */
template <typename EntityTy, template <typename S> class CounterType = acl::detail::counter>
class basic_registry : acl::detail::revision_table<typename EntityTy::revision_type>
{
  using base = acl::detail::revision_table<typename EntityTy::revision_type>;

public:
  using size_type     = typename EntityTy::size_type;
  using ssize_type    = std::make_signed_t<size_type>;
  using revision_type = typename EntityTy::revision_type;
  using type          = EntityTy;

  /**
   * @brief Creates a new entity in the registry
   *
   * Attempts to reuse a previously freed slot if available.
   * If no freed slots are available, creates a new entity by incrementing the max size.
   *
   * @return A new entity of the specified type
   *
   * @note This operation is thread-safe due to atomic operations depending on the CounterType
   */
  auto emplace() -> type
  {
    auto i = free_slot_.fetch_sub(1);
    if (i > 0)
    {
      return type(free_[i - 1]);
    }

    return type(max_size_.fetch_add(1));
  }

  /**
   * @brief Erases a slot and manages its revision for reuse
   *
   * This method handles the erasure of a slot and prepares it for reuse by:
   * 1. Adding the revised slot to the free list
   * 2. Updating the free slot counter
   * 3. Incrementing the revision number (if revision tracking is enabled)
   * 4. Marking the container as unsorted
   *
   * @param l The slot to be erased
   *
   * @note If revision tracking is enabled, the revision number for the slot will be incremented
   * @note The operation will resize the revisions array if needed
   * @note Sets the sorted flag to false as the operation may affect sorting
   * @note This method is not thread safe
   */
  void erase(type l)
  {
    auto count = free_slot_.load();
    if (count > 0)
    {
      free_.resize(static_cast<size_t>(count));
      free_.emplace_back(l.revised());
    }
    else
    {
      free_.clear();
      free_.emplace_back(l.revised());
    }

    free_slot_.store((ssize_type)free_.size(), std::memory_order_relaxed);

    if constexpr (!std::is_same_v<revision_type, void>)
    {
      auto idx = l.get();
      if (idx >= base::revisions_.size())
      {
        base::revisions_.resize(idx + 1);
      }
      assert(l.revision() == base::revisions_[idx]);
      base::revisions_[idx]++;
    }

    sorted_ = false;
  }

  /**
   * @brief Erases a list of elements from the registry
   *
   * This function handles the erasure of multiple elements by:
   * 1. Adding their indices to the free list for reuse
   * 2. Incrementing the revision counter for each erased element (if revision tracking is enabled)
   * 3. Marking the free list as unsorted
   *
   * @param ls Span of elements to erase
   *
   * @note The function maintains a free list of slots that can be reused when creating new elements
   * @note If revision tracking is enabled, the revision counter for each erased element is incremented
   * @note The sorted state is set to false after erasure
   * @note This method is not thread safe
   */
  void erase(std::span<type const> ls)
  {
    auto count = free_slot_.load();
    if (count > 0)
    {
      free_.resize(static_cast<size_t>(count));
      free_.reserve(static_cast<size_t>(count) + ls.size());
      for (auto const& l : ls)
      {
        free_.emplace_back(l.revised());
      }
    }
    else
    {
      free_.clear();
      free_.reserve(ls.size());
      for (auto const& l : ls)
      {
        free_.emplace_back(l.revised());
      }
    }

    free_slot_.store((ssize_type)free_.size(), std::memory_order_relaxed);

    if constexpr (!std::is_same_v<revision_type, void>)
    {
      for (auto const& l : ls)
      {
        auto idx = l.get();
        if (idx >= base::revisions_.size())
        {
          base::revisions_.resize(idx + 1);
        }
        assert(l.revision() == base::revisions_[idx]);
        base::revisions_[idx]++;
      }
    }

    sorted_ = false;
  }

  auto is_valid(type l) const noexcept -> bool
    requires(!std::is_same_v<revision_type, void>)
  {
    return static_cast<revision_type>(l.revision()) == base::revisions_[l.get()];
  }

  /**
   * @brief Gets the revision of a type identifier or component.
   * @tparam type The type parameter being used to query the revision.
   * @param l The type identifier or component to get the revision for.
   * @return The revision value associated with the type.
   * @note This overload only participates in overload resolution when revision_type is not void.
   */
  auto get_revision(type l) const noexcept
    requires(!std::is_same_v<revision_type, void>)
  {
    return get_revision(l.get());
  }

  /** @see get_revision */
  auto get_revision(size_type l) const noexcept
    requires(!std::is_same_v<revision_type, void>)
  {
    return l < base::revisions_.size() ? base::revisions_[l] : 0;
  }

  /**
   * @brief Iterates through all entity indices in the registry
   *
   * If the free list is not sorted, sorts it before iteration.
   * Calls the provided lambda for each valid entity index.
   *
   * @tparam Lambda Callable type that accepts an entity index parameter
   * @param l Lambda function to execute for each index, must accept a size_type parameter
   *
   * @note Ensures the free list is sorted before iteration
   * @see sort_free()
   */
  template <typename Lambda>
  void for_each_index(Lambda&& l)
  {
    if (!sorted_)
    {
      sort_free();
    }
    internal_for_each(std::forward<Lambda>(l), free_, max_size_.load());
  }

  /**
   * @brief Iterates through indices in the registry, optionally sorting them
   * @tparam Lambda Function type that accepts index parameters
   * @param l Lambda function to execute for each valid index
   *
   * If the registry is marked as sorted, iterates through indices directly.
   * Otherwise, creates a sorted copy of indices before iteration.
   * Sorting is done based on the type of each index.
   */
  template <typename Lambda>
  void for_each_index(Lambda&& l) const
  {
    if (sorted_)
    {
      internal_for_each(std::forward<Lambda>(l), free_, max_size_.load());
    }
    else
    {
      auto copy = free_;
      std::ranges::sort(copy,
                        [](size_type first, size_type second)
                        {
                          return type(first).get() < type(second).get();
                        });
      internal_for_each(std::forward<Lambda>(l), copy, max_size_.load());
    }
  }

  /**
   * @brief Gets the maximum size limit of the registry
   * @return The maximum number of entities that can be stored in the registry
   * @note This is a thread-safe operation as it accesses an atomic value
   */
  [[nodiscard]] auto max_size() const -> uint32_t
  {
    return max_size_.load();
  }

  /**
   * @brief Sorts the free list based on entity types.
   *
   * This function sorts the internal free list of entities in ascending order based on their types.
   * After sorting, the sorted_ flag is set to true to indicate the list is in sorted state.
   *
   * @note The sorting is performed using std::ranges::sort with a comparison function that
   *       compares entity types.
   */
  void sort_free()
  {
    std::ranges::sort(free_,
                      [](size_type first, size_type second)
                      {
                        return type(first).get() < type(second).get();
                      });
    sorted_ = true;
  }

  /**
   * @brief This function is not threadsafe
   */
  void shrink() noexcept
  {
    free_.resize(free_slot_.load());
  }

private:
  template <typename Lambda>
  static void internal_for_each(Lambda lambda, auto const& copy, size_type max_size)
  {
    for (size_type i = 1, fi = 0; i < max_size; ++i)
    {
      if (fi < copy.size() && type(copy[fi]).get() == i)
      {
        fi++;
      }
      else
      {
        lambda(i);
      }
    }
  }

  std::vector<size_type>  free_;
  CounterType<size_type>  max_size_  = {1};
  CounterType<ssize_type> free_slot_ = {0};
  bool                    sorted_    = false;
};

template <typename T = std::true_type>
using registry = basic_registry<entity<T>>;

template <typename T = std::true_type>
using rxregistry = basic_registry<rxentity<T>>;

} // namespace acl::ecs
