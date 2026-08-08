// SPDX-License-Identifier: MIT
#pragma once

#include "ouly/scheduler/worker_structs.hpp"

#include <atomic>
#include <coroutine>
#include <utility>

namespace ouly::detail
{

inline auto completed_sentinel() noexcept -> std::coroutine_handle<>
{
  return std::noop_coroutine();
}

struct coro_state
{
  using continuation_dispatch = auto (*)(std::coroutine_handle<>) noexcept -> std::coroutine_handle<>;

  std::atomic<std::coroutine_handle<>> continuation_{nullptr};
  std::atomic_bool                     started_{false};
  std::atomic<continuation_dispatch>   continuation_dispatch_{nullptr};
  workgroup_id                        resume_group_;
  bool                                detached_ = false;
};

template <TaskContext WC>
struct coroutine_context_slot
{
  inline static thread_local WC const* current_ = nullptr;
};

template <TaskContext WC>
class coroutine_context_guard
{
public:
  explicit coroutine_context_guard(WC const& ctx) noexcept
      : previous_(std::exchange(coroutine_context_slot<WC>::current_, &ctx))
  {}

  ~coroutine_context_guard() noexcept
  {
    coroutine_context_slot<WC>::current_ = previous_;
  }

private:
  WC const* previous_ = nullptr;
};
} // namespace ouly::detail
