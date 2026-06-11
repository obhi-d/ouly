// SPDX-License-Identifier: MIT

#include "ouly/scheduler/v3/scheduler.hpp"
#include "ouly/scheduler/detail/pause.hpp"
#include "ouly/scheduler/detail/v3/worker.hpp"
#include "ouly/scheduler/detail/v3/workgroup.hpp"
#include "ouly/scheduler/v3/task_context.hpp"
#include "ouly/scheduler/worker_structs.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <latch>
#include <thread>

namespace ouly::v3
{

using worker_type    = ouly::detail::v3::worker;
using workgroup_type = ouly::detail::v3::workgroup;
using work_item_type = ouly::detail::v3::work_item;

// Thread-local identity of the current worker thread.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
thread_local worker_type const* g_worker = nullptr;
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
thread_local ouly::worker_id g_worker_id = {};
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
thread_local uint32_t g_random_seed = 0;

// LCG constants for fast PRNG used to randomize steal victims
static constexpr uint32_t lcg_multiplier = 1664525U;
static constexpr uint32_t lcg_increment  = 1013904223U;

static auto update_seed() -> uint32_t
{
  return (g_random_seed = (g_random_seed * lcg_multiplier) + lcg_increment);
}

auto task_context::this_context::get() noexcept -> task_context const&
{
  OULY_ASSERT(g_worker != nullptr);
  return g_worker->get_context();
}

auto task_context::this_context::get_worker_id() noexcept -> worker_id
{
  return g_worker_id;
}

scheduler::~scheduler() noexcept
{
  if (workers_ && !stop_.load(std::memory_order_relaxed))
  {
    end_execution();
  }
}

void scheduler::notify_workers(uint32_t count) noexcept
{
  // Bump the epoch first: a worker about to park rechecks the epoch (via the futex value
  // check inside atomic wait), so a bump that lands between its work recheck and the wait
  // call prevents it from sleeping through this notification.
  wake_epoch_.get().fetch_add(1, std::memory_order_seq_cst);
  auto sleeper_count = sleepers_.get().load(std::memory_order_seq_cst);
  if (sleeper_count == 0)
  {
    return;
  }
  if (count > 1 && sleeper_count > 1)
  {
    wake_epoch_.get().notify_all();
  }
  else
  {
    wake_epoch_.get().notify_one();
  }
}

void scheduler::finish_task() noexcept
{
  if (pending_.get().fetch_sub(1, std::memory_order_acq_rel) == 1)
  {
    if (pending_waiters_.load(std::memory_order_acquire) != 0)
    {
      pending_.get().notify_all();
    }
  }
}

void scheduler::execute_work(worker_type& wkr, uint32_t group_index, work_item_type& work) noexcept
{
  auto& ctx        = wkr.context_;
  auto  prev_group = ctx.group_id_;
  auto  prev_off   = ctx.offset_;

  auto& group   = ouly::detail::vector_access(workgroups_, group_index);
  ctx.group_id_ = workgroup_id(group_index);
  ctx.offset_   = group.get_offset(ctx.get_worker().get_index());

  work(ctx);

  // Restore so a context observed through this_context::get() stays valid after nested
  // helping (cooperative waits) regardless of which group's task we just ran.
  ctx.group_id_ = prev_group;
  ctx.offset_   = prev_off;

  finish_task();
}

auto scheduler::try_execute_one(worker_id wid) noexcept -> bool
{
  auto& wkr = ouly::detail::vector_access(workers_, wid.get_index());

  for (uint32_t i = 0; i < wkr.group_count_; ++i)
  {
    uint32_t group_index = ouly::detail::vector_access(wkr.group_order_, i);
    auto&    group       = ouly::detail::vector_access(workgroups_, group_index);

    if (!group.has_queued())
    {
      continue;
    }

    work_item_type work{work_item_type::noinit};
    if (group.take(work, group.get_offset(wid.get_index()), update_seed()))
    {
      // Wake chaining: if this group still has queued items, recruit one more sleeper so
      // bursts fan out exponentially without broadcasting on every submit.
      if (group.has_queued())
      {
        notify_workers(1);
      }
      execute_work(wkr, group_index, work);
      return true;
    }
  }
  return false;
}

auto scheduler::has_queued_work(worker_type const& wkr) const noexcept -> bool
{
  for (uint32_t i = 0; i < wkr.group_count_; ++i)
  {
    if (ouly::detail::vector_access(workgroups_, ouly::detail::vector_access(wkr.group_order_, i)).has_queued())
    {
      return true;
    }
  }
  return false;
}

void scheduler::run_worker(worker_id wid)
{
  auto& wkr   = ouly::detail::vector_access(workers_, wid.get_index());
  g_worker    = &wkr;
  g_worker_id = wid;
  g_random_seed ^= wid.get_index() * lcg_multiplier;

  if (entry_fn_)
  {
    entry_fn_(wid);
  }

  uint32_t idle_spins = 0;

  while (!stop_.load(std::memory_order_relaxed))
  {
    if (try_execute_one(wid))
    {
      idle_spins = 0;
      continue;
    }

    if (++idle_spins < idle_spin_limit_)
    {
      ouly::detail::pause_exec();
      constexpr uint32_t yield_interval = 16;
      if ((idle_spins % yield_interval) == 0)
      {
        std::this_thread::yield();
      }
      continue;
    }
    idle_spins = 0;

    // Park: eventcount protocol. The seq_cst ordering of (sleepers_ increment, queued
    // recheck) here against (queued increment, epoch bump, sleepers_ read) on the
    // producer side guarantees the producer either sees us sleeping (and notifies) or we
    // see the queued item / bumped epoch (and skip the wait).
    auto epoch = wake_epoch_.get().load(std::memory_order_seq_cst);
    sleepers_.get().fetch_add(1, std::memory_order_seq_cst);

    if (has_queued_work(wkr) || stop_.load(std::memory_order_seq_cst))
    {
      sleepers_.get().fetch_sub(1, std::memory_order_relaxed);
      continue;
    }

    wake_epoch_.get().wait(epoch);
    sleepers_.get().fetch_sub(1, std::memory_order_relaxed);
  }

  g_worker    = nullptr;
  g_worker_id = {};
}

void scheduler::submit_internal([[maybe_unused]] task_context const& current, workgroup_id dst,
                                work_item_type const& work)
{
  OULY_ASSERT(workgroups_ != nullptr);
  OULY_ASSERT(dst && dst.get_index() < workgroup_count_);

  auto& group = ouly::detail::vector_access(workgroups_, dst.get_index());

  pending_.get().fetch_add(1, std::memory_order_relaxed);

  // Identify the calling thread by its live thread-local identity; a cached task_context
  // may be stale, and only the owning thread may push to a Chase-Lev deque.
  worker_type const* self = g_worker;
  if (self != nullptr && &self->get_context().get_scheduler() != this)
  {
    self = nullptr; // worker thread of a different scheduler instance
  }

  bool pushed = false;
  if (self != nullptr && group.contains(self->get_worker_id().get_index()))
  {
    pushed = group.push_local(group.get_offset(self->get_worker_id().get_index()), work);
  }

  while (!pushed && !group.push_mailbox(work))
  {
    // Mailbox full: wake everyone, then help drain if we are a worker of this scheduler.
    notify_workers(worker_count_);
    if (self != nullptr)
    {
      busy_work(self->get_worker_id());
    }
    else
    {
      std::this_thread::yield();
    }
  }

  notify_workers(1);

  // A waiter inside wait_for_tasks() may be parked on `pending_`; new work must wake it
  // so it can help drain groups it is the only member of (e.g. a main-thread group).
  if (pending_waiters_.load(std::memory_order_acquire) != 0)
  {
    pending_.get().notify_all();
  }
}

void scheduler::busy_work(worker_id thread) noexcept
{
  constexpr uint32_t attempts = 2;
  for (uint32_t i = 0; i < attempts; ++i)
  {
    if (try_execute_one(thread))
    {
      return;
    }
    ouly::detail::pause_exec();
  }
}

void scheduler::wait_for_tasks()
{
  auto main_thread = g_worker_id;

  while (pending_.get().load(std::memory_order_acquire) != 0)
  {
    if (try_execute_one(main_thread))
    {
      continue;
    }

    auto snapshot = pending_.get().load(std::memory_order_acquire);
    if (snapshot == 0)
    {
      break;
    }

    if (worker_count_ <= 1)
    {
      // No other workers exist; the remaining tasks can only be ours after a retry.
      std::this_thread::yield();
      continue;
    }

    pending_waiters_.fetch_add(1, std::memory_order_seq_cst);
    // Recheck after announcing ourselves; submit/finish paths notify when waiters != 0.
    if (pending_.get().load(std::memory_order_seq_cst) == snapshot &&
        !has_queued_work(ouly::detail::vector_access(workers_, main_thread.get_index())))
    {
      pending_.get().wait(snapshot);
    }
    pending_waiters_.fetch_sub(1, std::memory_order_relaxed);
  }
}

void scheduler::begin_execution(scheduler_worker_entry&& entry, void* user_context)
{
  // Compute worker / workgroup counts from the recorded descriptors.
  workgroup_count_ = 0;
  worker_count_    = 0;
  for (uint32_t i = 0; i < detail::v3::max_workgroup; ++i)
  {
    auto const& desc = ouly::detail::vector_access(workgroup_descs_, i);
    if (desc.thread_count_ > 0)
    {
      workgroup_count_ = std::max(workgroup_count_, i + 1);
      worker_count_    = std::max(worker_count_, desc.start_ + desc.thread_count_);
    }
  }

  OULY_ASSERT(workgroup_count_ > 0 && worker_count_ > 0);

  workgroups_ = std::make_unique<workgroup_type[]>(workgroup_count_);
  for (uint32_t i = 0; i < workgroup_count_; ++i)
  {
    auto const& desc = ouly::detail::vector_access(workgroup_descs_, i);
    if (desc.thread_count_ > 0)
    {
      ouly::detail::vector_access(workgroups_, i).create_group(desc.start_, desc.thread_count_, desc.priority_);
    }
  }

  workers_ = std::make_unique<worker_type[]>(worker_count_);
  for (uint32_t w = 0; w < worker_count_; ++w)
  {
    auto& wkr = ouly::detail::vector_access(workers_, w);
    wkr.context_.init(*this, user_context, 0, worker_id(w));

    // Fixed membership: collect groups containing this worker, sorted by priority desc.
    wkr.group_count_ = 0;
    for (uint32_t i = 0; i < workgroup_count_; ++i)
    {
      if (ouly::detail::vector_access(workgroups_, i).contains(w))
      {
        ouly::detail::vector_access(wkr.group_order_, wkr.group_count_++) = static_cast<uint8_t>(i);
      }
    }
    std::sort(wkr.group_order_.begin(), wkr.group_order_.begin() + wkr.group_count_,
              [this](uint8_t lhs, uint8_t rhs) -> bool
              {
                auto const& lg = ouly::detail::vector_access(workgroups_, lhs);
                auto const& rg = ouly::detail::vector_access(workgroups_, rhs);
                return lg.get_priority() == rg.get_priority() ? lhs < rhs : lg.get_priority() > rg.get_priority();
              });

    // Default context group: the highest-priority group this worker belongs to.
    if (wkr.group_count_ > 0)
    {
      uint32_t first_group   = ouly::detail::vector_access(wkr.group_order_, 0);
      wkr.context_.group_id_ = workgroup_id(first_group);
      wkr.context_.offset_   = ouly::detail::vector_access(workgroups_, first_group).get_offset(w);
    }
  }

  stop_.store(false, std::memory_order_relaxed);
  pending_.get().store(0, std::memory_order_relaxed);

  auto start_counter = std::latch(worker_count_);

  entry_fn_ = [cust_entry = std::move(entry), &start_counter](ouly::worker_id worker) -> void
  {
    if (cust_entry)
    {
      cust_entry(worker);
    }
    start_counter.count_down();
  };

  threads_.reserve(worker_count_ - 1);
  for (uint32_t thread = 1; thread < worker_count_; ++thread)
  {
    threads_.emplace_back(&scheduler::run_worker, this, worker_id(thread));
  }

  g_worker    = &ouly::detail::vector_access(workers_, 0);
  g_worker_id = worker_id(0);

  entry_fn_(worker_id(0));
  start_counter.wait();
  entry_fn_ = {};
}

void scheduler::end_execution()
{
  wait_for_tasks();

  stop_.store(true, std::memory_order_seq_cst);
  wake_epoch_.get().fetch_add(1, std::memory_order_seq_cst);
  wake_epoch_.get().notify_all();

  for (auto& thread : threads_)
  {
    if (thread.joinable())
    {
      thread.join();
    }
  }
  threads_.clear();
}

void scheduler::create_group(workgroup_id group, uint32_t start_thread_idx, uint32_t thread_count, uint32_t priority)
{
  if (group.get_index() >= detail::v3::max_workgroup || thread_count == 0)
  {
    return;
  }

  auto& desc         = ouly::detail::vector_access(workgroup_descs_, group.get_index());
  desc.start_        = start_thread_idx;
  desc.thread_count_ = thread_count;
  desc.priority_     = priority;

  workgroup_count_ = std::max(workgroup_count_, group.get_index() + 1);
  worker_count_    = std::max(worker_count_, start_thread_idx + thread_count);
}

auto scheduler::create_group(uint32_t start_thread_idx, uint32_t thread_count, uint32_t priority) -> workgroup_id
{
  for (uint32_t i = 0; i < detail::v3::max_workgroup; ++i)
  {
    if (ouly::detail::vector_access(workgroup_descs_, i).thread_count_ == 0)
    {
      workgroup_id new_group{i};
      create_group(new_group, start_thread_idx, thread_count, priority);
      return new_group;
    }
  }
  return workgroup_id{0};
}

void scheduler::clear_group(workgroup_id group)
{
  if (workgroups_ && group.get_index() < workgroup_count_)
  {
    // Dropped items never execute, so they must be balanced against pending_ here or
    // wait_for_tasks() would hang. take_any() only uses thief-safe operations, so this
    // is safe to call from any thread.
    auto&          wg = ouly::detail::vector_access(workgroups_, group.get_index());
    work_item_type work{work_item_type::noinit};
    while (wg.take_any(work))
    {
      finish_task();
    }
  }
}

auto scheduler::get_worker_count(workgroup_id g) const noexcept -> uint32_t
{
  return ouly::detail::vector_access(workgroups_, g.get_index()).get_thread_count();
}

auto scheduler::get_worker_start_idx(workgroup_id g) const noexcept -> uint32_t
{
  return ouly::detail::vector_access(workgroups_, g.get_index()).get_start_thread_idx();
}

auto scheduler::get_logical_divisor(workgroup_id g) const noexcept -> uint32_t
{
  auto count = get_worker_count(g);
  return count > 0 ? count : 1;
}

void scheduler::take_ownership() noexcept
{
  if (workers_)
  {
    g_worker    = &ouly::detail::vector_access(workers_, 0);
    g_worker_id = worker_id(0);
  }
}

} // namespace ouly::v3

void ouly::v3::task_context::cooperative_wait(std::binary_semaphore& event) const
{
  using namespace std::chrono_literals;
  auto&    sched      = *owner_;
  uint32_t idle_spins = 0;

  constexpr uint32_t spin_limit = 64;

  while (!event.try_acquire())
  {
    if (sched.try_execute_one(index_))
    {
      idle_spins = 0;
      continue;
    }

    if (++idle_spins < spin_limit)
    {
      ouly::detail::pause_exec();
      continue;
    }

    // No work and the event is not signaled: block with a short timeout instead of
    // spinning. The timeout bounds added latency for work that arrives while blocked.
    if (event.try_acquire_for(100us))
    {
      return;
    }
  }
}
