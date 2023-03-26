#include <compare>
#include <cstdint>

namespace acl
{
template <typename T>
class tagged_ptr
{

public:
  using tag_t = int8_t;

private:
  union pack
  {
    uintptr_t value;
    int8_t    parts[8];
  };

  static constexpr uintptr_t TagIndex = 7;
  static constexpr uintptr_t TagMask  = 0xffffffffffffffull;
  static constexpr uintptr_t PtrMask  = ~TagMask;

  static T* extract_ptr(tagged_ptr const& i)
  {
    return reinterpret_cast<T*>(i.value.value & TagMask);
  }

  static tag_t extract_tag(tagged_ptr const& i)
  {
    return i.value.parts[TagIndex];
  }

  static auto pack_ptr(T* ptr, tag_t tag)
  {
    pack packer;
    packer.value = reinterpret_cast<uintptr_t>(ptr);
    packer.parts[TagIndex] = tag;
    return packer.value;
  }

public:
  tagged_ptr() noexcept = default;
  tagged_ptr(T* val, tag_t tag) noexcept : value(pack_ptr(val, tag)) {}

  void set(T* p, tag_t t)
  {
    value.value = pack_ptr(p, t);
  }

  inline auto operator==(tagged_ptr const& p) const noexcept
  {
    return value.value == p.value.value;
  }

  inline auto operator!=(tagged_ptr const& p) const noexcept
  {
    return value.value != p.value.value;
  }

  inline auto operator<=>(tagged_ptr const& p) const noexcept
  {
    return value.value <=> p.value.value;
  }

  T* get_ptr() const
  {
    return extract_ptr(*this);
  }

  void set_ptr(T* p)
  {
    tag_t tag = get_tag();
    value.value = pack_ptr(p, tag);
  }

  tag_t get_tag() const
  {
    return extract_tag(*this);
  }

  tag_t get_next_tag() const
  {
    tag_t next = (get_tag() + 1) & (std::numeric_limits<tag_t>::max)();
    return next;
  }

  void set_tag(tag_t t)
  {
    T* p  = get_ptr();
    value.value = pack_ptr(p, t);
  }

  /** smart pointer support  */
  /* @{ */
  T& operator*() const
  {
    return *get_ptr();
  }

  T* operator->() const
  {
    return get_ptr();
  }

  operator bool(void) const
  {
    return get_ptr() != 0;
  }
  /* @} */

protected:
  pack value = 0;
};
} // namespace acl