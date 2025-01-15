#pragma once
#include <acl/containers/list_hook.hpp>
#include <iterator>
#include <type_traits>

namespace acl::detail
{

template <auto M>
struct intrusive_list_type_traits;

template <typename Ty, typename H, H Ty::* M>
struct intrusive_list_type_traits<M>
{
  using value_type               = Ty;
  using hook_type                = H;
  static constexpr bool is_dlist = std::is_same_v<list_hook, H>;

  static auto hook(Ty& t) noexcept -> H&
  {
    return t.*M;
  }

  static auto hook(Ty const& t) noexcept -> H const&
  {
    return t.*M;
  }

  static auto next(Ty const& t) noexcept -> Ty*
  {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<Ty*>(hook(t).pnext_);
  }

  static void next(Ty& t, Ty* next) noexcept
  {
    hook(t).pnext_ = next;
  }

  static auto prev(Ty const& t) noexcept -> Ty* requires(is_dlist) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<Ty*>(hook(t).pprev_);
  }

  static void prev(Ty& t, Ty* next) noexcept
    requires(is_dlist)
  {
    hook(t).pprev_ = next;
  }
};

template <typename SizeType, bool CacheSize = false>
struct size_counter
{
  void added() noexcept {}
  void added(SizeType /*unused*/) noexcept {}
  void erased() noexcept {}
  template <typename L>
  auto count(L const& list) const noexcept -> SizeType
  {
    SizeType nb = 0;
    for (auto i = std::begin(list), e = std::end(list); i != e; ++i, ++nb)
    {
      ;
    }
    return nb;
  }
  void clear() noexcept {}
};

template <typename SizeType>
struct size_counter<SizeType, true>
{
  void added() noexcept
  {
    count_++;
  }

  void erased() noexcept
  {
    count_--;
  }

  void added(SizeType a) noexcept
  {
    count_ += a;
  }

  template <typename L>
  auto count(L const& /*unused*/) const noexcept -> SizeType
  {
    return count_;
  }

  void clear() noexcept
  {
    count_ = 0;
  }

  SizeType count_ = 0;
};

template <typename Ty, auto M, typename B, bool CacheTail>
struct list_data : B
{
  Ty* head_ = nullptr;
};

template <typename Ty, auto M, typename B>
struct list_data<Ty, M, B, true> : B
{
  Ty* head_ = nullptr;
  Ty* tail_ = nullptr;
};
} // namespace acl::detail