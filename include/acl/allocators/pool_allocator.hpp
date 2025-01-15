#pragma once

#include <acl/allocators/default_allocator.hpp>
#include <acl/allocators/detail/custom_allocator.hpp>
#include <acl/allocators/detail/memory_stats.hpp>
#include <acl/allocators/detail/pool_defs.hpp>

namespace acl
{

struct pool_allocator_tag
{};

template <typename Config = acl::config<>>
class pool_allocator
    : acl::detail::statistics<pool_allocator_tag, acl::config<Config, cfg::base_stats<acl::detail::padding_stats>>>
{
public:
  using tag = pool_allocator_tag;
  using statistics =
   acl::detail::statistics<pool_allocator_tag, acl::config<Config, cfg::base_stats<acl::detail::padding_stats>>>;
  static constexpr auto default_atom_size  = acl::detail::atom_size<Config>::value;
  static constexpr auto default_atom_count = acl::detail::atom_count<Config>::value;
  using underlying_allocator               = acl::detail::underlying_allocator_t<Config>;
  using size_type                          = typename underlying_allocator::size_type;
  using address                            = typename underlying_allocator::address;

  pool_allocator() noexcept
      : k_atom_size_(static_cast<size_type>(default_atom_size)),
        k_atom_count_(static_cast<size_type>(default_atom_count))
  {}

  template <typename... Args>
  pool_allocator(size_type i_atom_size, size_type i_atom_count, Args&&... i_args)
      : k_atom_size_(i_atom_size), k_atom_count_(i_atom_count), statistics(std::forward<Args>(i_args)...)
  {}

  pool_allocator(pool_allocator const& i_other) = delete;
  pool_allocator(pool_allocator&& i_other) noexcept
      //  array_arena arrays;
      //  solo_arena      solo;
      //  const size_type k_atom_count;
      //  const size_type k_atom_size;
      //  arena_linker    linked_arenas;
      : arrays_(std::move(i_other.arrays_)), solo_(std::move(i_other.solo_)), k_atom_count_(i_other.k_atom_count_),
        k_atom_size_(i_other.k_atom_size_), linked_arenas_(std::move(i_other.linked_arenas_))
  {}

  auto operator=(pool_allocator const& i_other) -> pool_allocator& = delete;
  auto operator=(pool_allocator&& i_other) noexcept -> pool_allocator&
  {
    arrays_ = (std::move(i_other.arrays_));
    solo_   = std::move(i_other.solo_);
    assert(k_atom_count_ == i_other.k_atom_count_);
    assert(k_atom_size_ == i_other.k_atom_size_);
    linked_arenas_ = std::move(i_other.linked_arenas_);
    return *this;
  }

  constexpr static auto null() -> address
  {
    return underlying_allocator::null();
  }

  template <typename Alignment = alignment<>>
  [[nodiscard]] auto allocate(size_type size_value, Alignment alignment = {}) -> address
  {
    constexpr auto alignment_value = (size_t)alignment;
    auto           fixup           = alignment_value - 1;
    if (alignment_value && ((k_atom_size_ < alignment_value) || (k_atom_size_ & fixup)))
    {
      size_value += alignment_value + 4;
    }

    size_type i_count = (size_value + k_atom_size_ - 1) / k_atom_size_;

    if constexpr (acl::detail::HasComputeStats<Config>)
    {
      if (alignment_value && ((k_atom_size_ < alignment_value) || (k_atom_size_ & fixup)))
      {
        // Account for the missing atoms
        auto      real_size = size_value - alignment_value - 4;
        size_type count     = (real_size + k_atom_size_ - 1) / k_atom_size_;
        this->statistics::pad_atoms(static_cast<std::uint32_t>(i_count - count));
      }
    }

    if (i_count > k_atom_count_)
    {
      return underlying_allocator::allocate(size_value, alignment);
    }

    address ret_value;
    auto    measure = statistics::report_allocate(size_value);

    if (i_count == 1)
    {
      ret_value = solo_ ? consume() : consume(1);
    }
    else
    {
      ret_value = consume(i_count);
    }

    if (alignment_value && ((k_atom_size_ < alignment_value) || (k_atom_size_ & fixup)))
    {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      auto pointer = reinterpret_cast<std::uintptr_t>(ret_value);
      auto ret     = ((pointer + 4 + static_cast<std::uintptr_t>(fixup)) & ~static_cast<std::uintptr_t>(fixup));
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
      *(reinterpret_cast<std::uint32_t*>(ret) - 1) = static_cast<std::uint32_t>(ret - pointer);
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      return reinterpret_cast<address>(ret);
    }
    return ret_value;
  }

  template <typename Alignment = alignment<>>
  void deallocate(address i_ptr, size_type size_value, Alignment alignment = {})
  {
    constexpr auto alignment_value = (size_t)alignment;
    auto           fixup           = alignment_value - 1;
    address        orig_ptr        = i_ptr;
    if (alignment_value && ((k_atom_size_ < alignment_value) || (k_atom_size_ & fixup)))
    {
      size_value += alignment_value + 4;
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      std::uint32_t off_by = *(reinterpret_cast<std::uint32_t*>(i_ptr) - 1);
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      i_ptr = reinterpret_cast<address>(reinterpret_cast<std::uint8_t*>(i_ptr) - off_by);
    }

    size_type i_count = (size_value + k_atom_size_ - 1) / k_atom_size_;

    if constexpr (acl::detail::HasComputeStats<Config>)
    {
      if (alignment_value && ((k_atom_size_ < alignment_value) || (k_atom_size_ & fixup)))
      {
        // Account for the missing atoms
        auto      real_size = size_value - alignment_value - 4;
        size_type count     = (real_size + k_atom_size_ - 1) / k_atom_size_;
        this->statistics::unpad_atoms(static_cast<std::uint32_t>(i_count - count));
      }
    }

    if (i_count > k_atom_count_)
    {
      underlying_allocator::deallocate(orig_ptr, size_value, alignment);
    }
    else
    {
      auto measure = statistics::report_deallocate(size_value);
      if (i_count == 1)
      {
        release(i_ptr);
      }
      else
      {
        release(i_ptr, i_count);
      }
    }
  }

private:
  struct array_arena
  {

    array_arena() : ppvalue_(nullptr) {}
    array_arena(array_arena const& other) : ppvalue_(other.ppvalue_) {}
    array_arena(array_arena&& other) noexcept : ppvalue_(other.ppvalue_)
    {
      other.ppvalue_ = nullptr;
    }
    array_arena(void* i_pdata) : pvalue_(i_pdata) {}
    ~array_arena() noexcept = default;
    auto operator=(array_arena&& other) noexcept -> array_arena&
    {
      ppvalue_       = other.ppvalue_;
      other.ppvalue_ = nullptr;
      return *this;
    }
    auto operator=(array_arena const& other) noexcept -> array_arena&
    {
      pvalue_ = other.pvalue_;
      return *this;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    explicit array_arena(address i_addr, size_type i_count) : ivalue_(reinterpret_cast<std::uintptr_t>(i_addr) | 0x1)
    {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      *reinterpret_cast<size_type*>(i_addr) = i_count;
    }

    auto length() const -> size_type
    {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      return *reinterpret_cast<size_type*>(ivalue_ & ~0x1);
    }

    void set_length(size_type i_length)
    {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      *reinterpret_cast<size_type*>(ivalue_ & ~0x1) = i_length;
    }

    auto get_next() const -> array_arena
    {
      // NOLINTNEXTLINE
      return *(reinterpret_cast<void**>(ivalue_ & ~0x1) + 1);
    }

    void set_next(array_arena next)
    {
      // NOLINTNEXTLINE
      *(reinterpret_cast<void**>(ivalue_ & ~0x1) + 1) = next.value_;
    }

    void clear_flag()
    {
      ivalue_ &= (~0x1);
    }

    [[nodiscard]] auto get_value() const -> std::uint8_t*
    {
      // NOLINTNEXTLINE
      return reinterpret_cast<std::uint8_t*>(ivalue_ & ~0x1);
    }
    auto update(size_type i_count) -> void*
    {
      return get_value() + i_count;
    }

    explicit operator bool() const
    {
      return ppvalue_ != nullptr;
    }
    union
    {
      address        addr_;
      void*          pvalue_;
      void**         ppvalue_;
      std::uint8_t*  value_;
      std::uintptr_t ivalue_;
    };
  };

  struct solo_arena
  {
    solo_arena() : ppvalue_(nullptr) {}
    solo_arena(solo_arena const&& other) noexcept : ppvalue_(other.ppvalue_) {}
    solo_arena(void* i_pdata) : pvalue_(i_pdata) {}
    solo_arena(solo_arena&& other) noexcept : ppvalue_(other.ppvalue_)
    {
      other.ppvalue_ = nullptr;
    }
    solo_arena(solo_arena const& other) : pvalue_(other.pvalue_) {}
    solo_arena(array_arena const& other) : pvalue_(other.get_value()) {}
    ~solo_arena() noexcept = default;
    auto operator=(solo_arena&& other) noexcept -> solo_arena&
    {
      ppvalue_       = other.ppvalue_;
      other.ppvalue_ = nullptr;
      return *this;
    }
    auto operator=(array_arena const& other) -> solo_arena&
    {
      pvalue_ = other.get_value();
      return *this;
    }
    auto operator=(solo_arena const& other) -> solo_arena&
    {
      pvalue_ = other.pvalue_;
      return *this;
    }

    [[nodiscard]] auto get_value() const -> void*
    {
      return pvalue_;
    }

    auto get_next() const -> solo_arena
    {
      return *(ppvalue_);
    }
    void set_next(solo_arena next)
    {
      *(ppvalue_) = next.value_;
    }

    explicit operator bool() const
    {
      return ppvalue_ != nullptr;
    }
    union
    {
      address        addr_;
      void*          pvalue_;
      void**         ppvalue_;
      std::uint8_t*  value_;
      std::uintptr_t ivalue_;
    };
  };

  struct arena_linker
  {
    arena_linker()                                       = default;
    auto operator=(const arena_linker&) -> arena_linker& = delete;
    auto operator=(arena_linker&&) -> arena_linker&      = delete;
    explicit arena_linker(const arena_linker& i_other)   = default;
    arena_linker(arena_linker&& i_other) noexcept : first_(i_other.first_)
    {
      i_other.first_ = nullptr;
    }
    ~arena_linker() noexcept = default;

    enum : size_type
    {
      k_header_size = sizeof(void*)
    };

    void link_with(address arena, size_type size)
    {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      void** loc = reinterpret_cast<void**>(static_cast<std::uint8_t*>(arena) + size);
      *loc       = first_;
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      first_ = reinterpret_cast<void*>(loc);
    }

    template <typename Lambda>
    void for_each(Lambda i_deleter, size_type size)
    {
      size_type real_size = size + k_header_size;
      void*     it        = first_;
      while (it)
      {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        void* next = *reinterpret_cast<void**>(it);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        i_deleter(reinterpret_cast<address>(reinterpret_cast<std::uint8_t*>(it) - size), real_size);
        it = next;
      }
    }

    explicit operator bool() const
    {
      return first_ != nullptr;
    }
    void* first_ = nullptr;
  };

public:
  ~pool_allocator()
  {
    size_type size = k_atom_count_ * k_atom_size_;
    linked_arenas_.for_each(
     [=](address i_value, size_type size_value)
     {
       underlying_allocator::deallocate(i_value, size_value);
     },
     size);
  }

private:
  auto consume(size_type i_count) -> address
  {
    size_type len = arrays_ ? arrays_.length() : 0;
    if (!arrays_ || len < i_count)
    {
      allocate_arena();
      len = arrays_.length();
    }

    assert(len >= i_count);
    std::uint8_t* ptr       = arrays_.get_value();
    std::uint8_t* head      = ptr + (i_count * k_atom_size_);
    size_type     left_over = len - i_count;
    switch (left_over)
    {
    case 0:
      arrays_ = arrays_.get_next();
      break;
    case 1:
    {
      solo_arena new_solo(head);
      arrays_ = arrays_.get_next();
      new_solo.set_next(solo_);
      solo_ = new_solo;
    }
    break;
    default:
    {
      // reorder arrays to sort them from big to small
      array_arena cur = arrays_.get_next();
      array_arena save(head, left_over);
      if (cur && cur.length() > left_over)
      {
        arrays_          = cur;
        array_arena prev = save;
        while (true)
        {
          if (!cur || cur.length() <= left_over)
          {
            prev.set_next(save);
            save.set_next(cur);
            break;
          }
          prev = cur;
          cur  = cur.get_next();
        }
      }
      else
      {
        save.set_next(cur);
        arrays_ = save;
      }
    }
    break;
    }

    return ptr;
  }

  auto consume() -> address
  {
    address ptr = solo_.get_value();
    solo_       = solo_.get_next();
    return ptr;
  }

  void release(address i_only, size_type i_count)
  {
    array_arena new_arena(i_only, i_count);
    array_arena cur = arrays_;
    if (cur.length() > i_count)
    {
      array_arena prev = new_arena;
      while (true)
      {
        if (!cur || cur.length() <= i_count)
        {
          prev.set_next(new_arena);
          new_arena.set_next(cur);
          break;
        }
        prev = cur;
        cur  = cur.get_next();
      }
    }
    else
    {
      new_arena.set_next(cur);
      arrays_ = new_arena;
    }
  }

  void release(address i_only)
  {
    solo_arena arena(i_only);
    arena.set_next(std::move(solo_));
    solo_ = arena;
  }

  void allocate_arena()
  {
    size_type   size       = k_atom_count_ * k_atom_size_;
    address     arena_data = underlying_allocator::allocate(size + arena_linker::k_header_size);
    array_arena new_arena(arena_data, k_atom_count_);
    linked_arenas_.link_with(arena_data, size);
    new_arena.set_next(arrays_);
    arrays_ = new_arena;
    statistics::report_new_arena();
  }

  [[nodiscard]] auto get_total_free_count() const -> std::uint32_t
  {
    std::uint32_t count   = 0;
    auto          a_first = arrays_;
    while (a_first)
    {
      count += static_cast<std::uint32_t>(a_first.length());
      a_first = a_first.get_next();
    }

    auto s_first = solo_;
    while (s_first)
    {
      count++;
      s_first = s_first.get_next();
    }
    return count;
  }

  [[nodiscard]] auto get_missing_atoms() const -> std::uint32_t
  {
    if constexpr (acl::detail::HasComputeStats<Config>)
    {
      return this->statistics::padding_atoms_count();
    }
    return 0;
  }

  [[nodiscard]] auto get_total_arena_count() const -> std::uint32_t
  {
    std::uint32_t count = 0;
    arena_linker  a_first(linked_arenas_);
    a_first.for_each(
     [&](address i_value, size_type size_value)
     {
       count++;
     },
     k_atom_size_ * k_atom_count_);
    return count;
  }

  array_arena  arrays_;
  solo_arena   solo_;
  size_type    k_atom_count_ = {};
  size_type    k_atom_size_  = {};
  arena_linker linked_arenas_;

public:
  template <typename RecordTy>
  auto validate(RecordTy const& records) -> bool
  {
    std::uint32_t rec_count = 0;
    for (auto& rec : records)
    {
      if (rec.count <= k_atom_count_)
      {
        rec_count += rec.count;
      }
    }

    std::uint32_t arena_count = get_total_arena_count();
    if (rec_count + get_total_free_count() + get_missing_atoms() != arena_count * k_atom_count_)
    {
      return false;
    }

    if (arena_count != this->statistics::get_arenas_allocated())
    {
      return false;
    }
    return true;
  }
};
} // namespace acl