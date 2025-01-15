#pragma once

#include <acl/scheduler/awaiters.hpp>

namespace acl::detail
{
template <typename Awaiter>
concept HasMemberCoAwait = requires(Awaiter w) {
  { w.operator co_await() };
};

template <typename Awaiter>
concept HasFreeCoAwait = requires(Awaiter w) {
  { operator co_await(w) };
};

template <typename Awaiter>
concept HasCoAwait = HasFreeCoAwait<Awaiter> || HasFreeCoAwait<Awaiter>;

template <typename Awaiter>
using awaiter_result_t = decltype(std::declval<Awaiter&>().await_resume());

template <HasMemberCoAwait Awaitable>
auto get_awaiter(Awaitable&& awaitable) noexcept -> decltype(auto)
{
  return static_cast<Awaitable&&>(std::forward<Awaitable>(awaitable)).operator co_await();
}

template <HasFreeCoAwait Awaitable>
auto get_awaiter(Awaitable&& awaitable) noexcept -> decltype(auto)
{
  return operator co_await(static_cast<Awaitable&&>(std::forward<Awaitable>(awaitable)));
}

template <typename Awaitable>
using awaiter_t = decltype(get_awaiter(std::declval<Awaitable>()));
} // namespace acl::detail