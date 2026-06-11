// SPDX-License-Identifier: MIT

#include "ouly/scheduler/v2/scheduler.hpp"
#include "ouly/scheduler/detail/pause.hpp"
#include "ouly/scheduler/detail/v2/worker.hpp"
#include "ouly/scheduler/detail/v2/workgroup.hpp"
#include "ouly/scheduler/v2/task_context.hpp"
#include "ouly/scheduler/worker_structs.hpp"
#include <atomic>
#include <barrier>
#include <chrono>
#include <cstdint>
#include <latch>
#include <ranges>
#include <thread>

namespace ouly::inline v2
{

// Bring type aliases into scope
using workgroup_type = ouly::detail::v2::workgroup;
using work_item_type = ouly::detail::v2::work_item;
using worker_type    = ouly::detail::v2::worker;

// Thread-local storage for worker information
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
thread_local detail::v2::worker const* g_worker = nullptr;
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
thread_local ouly::worker_id g_worker_id = {};
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
thread_local uint32_t g_random_seed = 0;

// LCG constants for fast PRNG
static constexpr uint32_t lcg_multiplier    = 1664525U;
static constexpr uint32_t lcg_increment     = 1013904223U;
static constexpr uint32_t initial_seed_mask = 0xAAAAAAAAU;

static auto update_seed() -> uint32_t;

static auto update_seed() -> uint32_t
{
  return (g_random_seed = ((g_random_seed * lcg_multiplier) + lcg_increment) & initial_seed_mask);
}

auto task_context::this_context::get() noexcept -> task_context const&
{
  return g_worker->get_context();
}

auto task_context::this_context::get_worker_id() noexcept -> worker_id
{
  return g_worker_id;
}

struct workgroup_desc
{
  uint32_t start_        = 0;
  uint32_t thread_count_ = 0;
  uint32_t priority_     = 0; // Priority of the workgroup
};

struct scheduler::worker_initializer
{
  std::array<workgroup_desc, detail::v2::max_workgroup> workgroup_descriptions_;
};

scheduler::~scheduler() noexcept
{
  if (!stop_.load())
  {
    end_execution();
  }
}

void scheduler::submit_internal([[maybe_unused]] task_context const& current, workgroup_id dst,
                                ::ouly::detail::v2::work_item const& work)
{
  auto& target_workgroup = ouly::detail::vector_access(workgroups_, dst.get_index());

  pending_.fetch_add(1, std::memory_order_relaxed);

  // The per-worker Chase-Lev queues are single-producer: only the worker that currently *owns* a
  // slot may push to that slot's queue. We must therefore identify the calling worker by its live
  // thread-local identity (g_worker), NOT by the `current` task_context that was passed in: callers
  // routinely cache a task_context (e.g. `auto ctx = this_context::get();`) and reuse it across many
  // submits, but a worker's group/offset changes whenever it migrates between workgroups during a
  // cooperative wait. Using a stale offset here would push into a queue owned by a *different*
  // thread, corrupting the deque (single-producer invariant violated) and silently losing the task,
  // which manifests as a phantom "work available" count and a hang. The live worker always owns the
  // slot reported by its own current context, so pushing there is safe.
  detail::v2::worker const* self = g_worker;
  if (self != nullptr && &self->get_context().get_scheduler() != this)
  {
    self = nullptr; // worker thread belongs to a different scheduler instance
  }

  if (self != nullptr && self->get_workgroup() == dst)
  {
    if (target_workgroup.push_work_to_worker(self->get_group_offset(), work))
    {
      // Work was successfully pushed, workgroup will advertise availability
      wake_up_workers(1); // Wake up one worker
      return;
    }
  }

  // Either we are not running on a worker that owns a slot in `dst`, or that worker's queue is full:
  // route through the multi-producer mailbox, which is safe to push to from any thread.
  while (!target_workgroup.submit_to_mailbox(work))
  {
    // Mailbox is full; recruit everyone, then help drain when we are a worker of this
    // scheduler. A foreign thread must not call busy_work: it would pop from a deque it
    // does not own and break the single-consumer invariant.
    wake_up_workers(worker_count_);
    if (self != nullptr)
    {
      busy_work(self->get_worker_id());
    }
    else
    {
      std::this_thread::yield();
    }
  }

  // Add to needy workgroups list for better work distribution
  wake_up_workers(1); // Wake up workers to handle the new work
}

void scheduler::wake_up_workers(uint32_t count) noexcept
{
  // The epoch bump must be ordered before the sleeping_ read (seq_cst RMW) so that it
  // forms a total order with a parking worker's (sleeping_ increment, work re-check):
  // either we observe the sleeper and release a token, or the sleeper observes the
  // advertised work / bumped epoch and aborts the park.
  park_epoch_.fetch_add(1, std::memory_order_seq_cst);
  auto sleeping_count = sleeping_.load(std::memory_order_seq_cst);
  if (sleeping_count <= 0)
  {
    return;
  }
  auto wake_count = std::min(count, static_cast<uint32_t>(sleeping_count));

  if (wake_count > 0)
  {
    wake_tokens_.release(static_cast<int32_t>(wake_count));
  }
}

void scheduler::run_worker(worker_id wid)
{
  // Set thread-local storage for this worker
  g_worker_id = wid;
  g_worker    = &ouly::detail::vector_access(workers_, wid.get_index());

  // Execute the entry function if provided
  if (entry_fn_)
  {
    entry_fn_(wid);
  }

  // Main worker loop
  while (!stop_.load(std::memory_order_relaxed))
  {
    const auto park_token = park_epoch_.load(std::memory_order_seq_cst);

    if (!find_work_for_worker(wid))
    {
      // Enter sleep state (seq_cst: pairs with the producer's epoch bump + sleeping_ read
      // in wake_up_workers so one side always observes the other).
      sleeping_.fetch_add(1, std::memory_order_seq_cst);

      if (find_work_for_worker(wid))
      {
        // Acquired (and executed) work while arming sleep: disarm and retry.
        sleeping_.fetch_sub(1, std::memory_order_relaxed);
        continue;
      }

      // Note: this epoch check is what makes parking race-free. A producer bumps the
      // epoch (seq_cst) after advertising work and before reading sleeping_, so either
      // we observe the bump here and abort the park, or the producer observes our
      // sleeping_ increment and releases a wake token.
      if (park_token != park_epoch_.load(std::memory_order_seq_cst))
      {
        sleeping_.fetch_sub(1, std::memory_order_relaxed);
        // If the epoch has changed, we need to exit the context
        // This means we have new work or a stop signal
        continue;
      }

      // exit context
      auto& worker = ouly::detail::vector_access(workers_, wid.get_index());
      if (worker.get_workgroup())
      {
        auto& workgroup = ouly::detail::vector_access(workgroups_, worker.get_workgroup().get_index());
        workgroup.exit(static_cast<int>(worker.get_group_offset()));
        worker.set_workgroup_info(0, workgroup_id{});
      }

      wake_tokens_.acquire();

      if (stop_.load(std::memory_order_relaxed))
      {
        break;
      }

      // Exit sleep state
      sleeping_.fetch_sub(1, std::memory_order_relaxed);
    }
  }

  g_worker    = nullptr;
  g_worker_id = {};

  finished_.fetch_add(1, std::memory_order_release);
}

auto scheduler::enter_context(worker_id wid, workgroup_id needy_wg) noexcept -> bool
{
  auto& worker = ouly::detail::vector_access(workers_, wid.get_index());

  if (worker.get_workgroup() != needy_wg)
  {
    auto& needy_workgroup = ouly::detail::vector_access(workgroups_, needy_wg.get_index());
    int   enter_ctx       = needy_workgroup.enter();
    if (enter_ctx < 0)
    {
      return false;
    }

    if (worker.get_workgroup())
    {
      auto& current_group = ouly::detail::vector_access(workgroups_, worker.get_workgroup().get_index());
      current_group.exit(static_cast<int>(worker.get_group_offset()));
    }

    worker.set_workgroup_info(static_cast<uint32_t>(enter_ctx), needy_wg);
  }
  return true;
}

// NOLINTNEXTLINE
auto scheduler::find_work_for_worker(worker_id wid) noexcept -> bool
{
  // Try to drain work from the worker's own workgroup first
  auto&                 worker        = ouly::detail::vector_access(workers_, wid.get_index());
  thread_local uint32_t random_victim = update_seed();

  if (worker.get_workgroup())
  {
    auto& workgroup = ouly::detail::vector_access(workgroups_, worker.get_workgroup().get_index());

    if (workgroup.has_work())
    {
      detail::v2::work_item work{detail::v2::work_item::noinit};

      if (workgroup.pop_work_from_worker(work, worker.get_group_offset()))
      {
        on_work_taken(workgroup);
        execute_work(wid, work);
        return true;
      }

      // No work in mailbox, try stealing from this workgroup
      if (workgroup.steal_work(work, random_victim))
      {
        on_work_taken(workgroup);
        execute_work(wid, work);
        return true;
      }

      if (workgroup.receive_from_mailbox(work))
      {
        on_work_taken(workgroup);
        execute_work(wid, work);
        return true;
      }

      random_victim = update_seed();
    }
  }

  // Priority 3: Work stealing from any workgroup with priority-based selection
  // Start with higher priority workgroups and overloaded workgroups
  uint32_t steal_start_idx = update_seed();

  for (uint32_t attempt = 0; attempt < workgroup_count_; ++attempt)
  {
    uint32_t i         = (steal_start_idx + attempt) % workgroup_count_;
    auto&    workgroup = ouly::detail::vector_access(workgroups_, i);

    if (!workgroup.has_work())
    {
      continue;
    }

    if (!enter_context(wid, workgroup_id(i)))
    {
      continue;
    }

    detail::v2::work_item work{detail::v2::work_item::noinit};
    if (workgroup.steal_work(work, steal_start_idx))
    {
      on_work_taken(workgroup);
      execute_work(wid, work);
      return true;
    }

    if (workgroup.receive_from_mailbox(work))
    {
      on_work_taken(workgroup);
      execute_work(wid, work);
      return true;
    }
  }

  // Report failure when no item could be acquired. Returning "retry" here just because a
  // group still advertised work made every idle worker spin for the whole duration of any
  // executing task (advertise was only balanced after execution). With dequeue-time
  // accounting, queues being empty means there is genuinely nothing to acquire; parking is
  // safe because every push wakes a worker and takers chain wakes while items remain.
  return false;
}

void scheduler::on_work_taken(detail::v2::workgroup& src_group) noexcept
{
  // Balance the advertise that accompanied the push of the item just dequeued. Done at
  // dequeue time (not after execution) so has_work() reflects queued items only.
  src_group.sink_one_work();

  // Wake chaining: if more items remain queued, recruit one more sleeper. This covers
  // bursts where submit-side wakes were capped by the number of sleepers at the time.
  if (src_group.has_work())
  {
    wake_up_workers(1);
  }
}

void scheduler::execute_work(worker_id wid, detail::v2::work_item& work) noexcept
{
  auto& worker = ouly::detail::vector_access(workers_, wid.get_index());
  // Create a copy since work_item expects mutable reference
  auto const& current_context = worker.get_context();
  work(current_context);

  pending_.fetch_sub(1, std::memory_order_acq_rel);
}

void scheduler::begin_execution(scheduler_worker_entry&& entry, void* user_context)
{
  OULY_ASSERT(initializer_ != nullptr); // create_group() must be called before begin_execution()
  if (!initializer_)
  {
    return;
  }

  auto& workgroup_descs = initializer_->workgroup_descriptions_;
  // Initialize worker contexts
  for (uint32_t i = 0; i < detail::v2::max_workgroup; ++i)
  {
    if (ouly::detail::vector_access(workgroup_descs, i).thread_count_ > 0)
    {
      workgroup_count_ = std::max(workgroup_count_, i + 1);
      worker_count_    = std::max(worker_count_, ouly::detail::vector_access(workgroup_descs, i).start_ +
                                                  ouly::detail::vector_access(workgroup_descs, i).thread_count_);
    }
  }

  workgroups_ = std::make_unique<detail::v2::workgroup[]>(workgroup_count_);

  for (uint32_t i = 0; i < workgroup_count_; ++i)
  {
    if (ouly::detail::vector_access(workgroup_descs, i).thread_count_ > 0)
    {
      ouly::detail::vector_access(workgroups_, i)
       .create_group(ouly::detail::vector_access(workgroup_descs, i).start_,
                     ouly::detail::vector_access(workgroup_descs, i).thread_count_,
                     ouly::detail::vector_access(workgroup_descs, i).priority_);
    }
  }

  // Initialize workers and workgroups
  workers_ = std::make_unique<detail::v2::worker[]>(worker_count_);

  // Per-worker backoff lives inside each worker

  stop_.store(false, std::memory_order_relaxed);
  finished_.store(0, std::memory_order_relaxed);
  sleeping_.store(0, std::memory_order_relaxed);
  pending_.store(0, std::memory_order_relaxed);
  // Drain any tokens left over from a previous run so workers don't wake spuriously.
  while (wake_tokens_.try_acquire())
  {
  }

  auto start_counter = std::latch(worker_count_);

  entry_fn_ = [cust_entry = std::move(entry), &start_counter](ouly::worker_id worker) -> void
  {
    if (cust_entry)
    {
      cust_entry(worker);
    }
    start_counter.count_down();
  };

  // Create worker threads
  threads_.reserve(worker_count_);

  for (uint32_t thread = 1; thread < worker_count_; ++thread)
  {
    ouly::detail::vector_access(workers_, thread).current_context_.init(*this, user_context, 0, worker_id(thread));
    threads_.emplace_back(&scheduler::run_worker, this, worker_id(thread));
  }

  ouly::detail::vector_access(workers_, 0).current_context_.init(*this, user_context, 0, worker_id(0));
  enter_context(worker_id(0), workgroup_id(0));

  g_worker    = &ouly::detail::vector_access(workers_, 0);
  g_worker_id = worker_id(0);

  entry_fn_(worker_id(0));
  start_counter.wait();
  entry_fn_ = {}; // Clear entry function after execution starts

  initializer_ = nullptr;
}

void scheduler::end_execution()
{
  if (!workers_)
  {
    // begin_execution() was never called; nothing to wind down.
    stop_.store(true, std::memory_order_relaxed);
    return;
  }

  finish_pending_tasks();

  // Wait for all threads to finish
  for (auto& thread : threads_)
  {
    if (thread.joinable())
    {
      thread.join();
    }
  }

  threads_.clear();
}

auto scheduler::has_work() const -> bool
{
  return std::ranges::any_of(std::span(workgroups_.get(), workgroup_count_),
                             [](const auto& group) -> bool
                             {
                               return group.has_work_strong();
                             });
}

void scheduler::wait_for_tasks()
{
  // Wait until every submitted task has finished (queued + in-flight), helping where
  // possible. Releasing wake tokens unconditionally here used to leak tokens (workers
  // consume one per park), causing spurious wakeups long after the wait.
  while (pending_.load(std::memory_order_acquire) > 0)
  {
    if (busy_work(worker_id(0)))
    {
      continue;
    }
    if (has_work())
    {
      // Queued items we could not take (e.g. group has no free slot): recruit workers.
      wake_up_workers(worker_count_);
    }
    // Only in-flight tasks remain; don't burn this core while they finish.
    std::this_thread::yield();
  }
}

void scheduler::finish_pending_tasks()
{
  wait_for_tasks();

  // First: Signal stop to prevent new task submissions
  stop_.store(true, std::memory_order_seq_cst);

  auto thread_count = static_cast<uint32_t>(worker_count_ - 1);
  while (finished_.load(std::memory_order_acquire) < thread_count)
  {
    // Bounded wake: release at most one token per parked worker per round. The previous
    // unconditional release in a tight loop could exceed the semaphore's max count (UB)
    // and burned a core while workers wound down.
    park_epoch_.fetch_add(1, std::memory_order_seq_cst);
    auto sleeping_count = sleeping_.load(std::memory_order_seq_cst);
    if (sleeping_count > 0)
    {
      wake_tokens_.release(sleeping_count);
    }
    std::this_thread::yield();
  }
}

void scheduler::create_group(workgroup_id group, uint32_t start_thread_idx, uint32_t thread_count, uint32_t priority)
{
  if (group.get_index() >= detail::v2::max_workgroup || thread_count == 0)
  {
    return;
  }

  if (!initializer_)
  {
    initializer_ = std::make_shared<worker_initializer>();
  }

  if (group.get_index() >= workgroup_count_)
  {
    workgroup_count_ = group.get_index() + 1;
  }

  auto& wg         = ouly::detail::vector_access(initializer_->workgroup_descriptions_, group.get_index());
  wg.start_        = start_thread_idx;
  wg.thread_count_ = thread_count;
  wg.priority_     = priority;
  worker_count_    = std::max(worker_count_, wg.start_ + wg.thread_count_);
}

auto scheduler::create_group(uint32_t start_thread_idx, uint32_t thread_count, uint32_t priority) -> workgroup_id
{
  // Find next available group ID
  auto& workgroup_descs = initializer_->workgroup_descriptions_;
  for (uint32_t i = 0; i < detail::v2::max_workgroup; ++i)
  {
    if (ouly::detail::vector_access(workgroup_descs, i).thread_count_ == 0)
    {
      workgroup_id new_group{i};
      create_group(new_group, start_thread_idx, thread_count, priority);
      return new_group;
    }
  }

  return workgroup_id{0}; // Fallback to group 0 if all are used
}

void scheduler::clear_group(workgroup_id group)
{
  if (group.get_index() < detail::v2::max_workgroup)
  {
    ouly::detail::vector_access(workgroups_, group.get_index()).clear();
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
  auto worker_count = get_worker_count(g);
  return worker_count > 0 ? worker_count : 1;
}

void scheduler::take_ownership() noexcept
{
  // Implementation for taking ownership when multiple schedulers exist
  // This is typically used when multiple scheduler instances exist
  enter_context(worker_id(0), workgroup_id(0));
}

void scheduler::reset_to_workgroup(worker_id thread, workgroup_id group)
{
  if (!group)
  {
    // The worker had no workgroup (e.g. parked between contexts); nothing to restore.
    return;
  }

  auto& worker = ouly::detail::vector_access(workers_, thread.get_index());
  if (worker.get_workgroup() != group)
  {
    constexpr uint32_t max_context_enter_attempts = 1000;
    uint32_t           enter_attempts             = 0;
    while (worker.get_workgroup() != group && enter_attempts < max_context_enter_attempts)
    {
      if (enter_context(thread, group))
      {
        break; // Successfully entered the preserved group context
      };

      ++enter_attempts;
      ouly::detail::pause_exec();
    }
  }
}

auto scheduler::busy_work(worker_id thread) noexcept -> bool
{
  auto& worker = ouly::detail::vector_access(workers_, thread.get_index());
  // We need to preserve the worker's context and group information and restore it after busy work
  auto preserve_group = worker.get_workgroup();

  // Try to find work for the worker during busy wait
  // Optimized work stealing with adaptive attempts
  constexpr uint32_t min_attempts      = 1;
  constexpr uint32_t max_attempts      = 3;
  constexpr uint32_t failure_threshold = 5;

  // Use thread_local failure tracking for better performance
  auto&    local_recent_failures = ouly::detail::vector_access(workers_, thread.get_index()).busy_backoff_;
  uint32_t attempts              = (local_recent_failures > failure_threshold) ? min_attempts : max_attempts;

  for (uint32_t attempt = 0; attempt < attempts; ++attempt)
  {
    if (find_work_for_worker(thread)) [[likely]]
    {
      local_recent_failures = std::max(0U, local_recent_failures - 1);
      reset_to_workgroup(thread, preserve_group);
      return true; // Found and executed work
    }

    // Shorter pause for first attempts, longer for subsequent ones
    if (attempt < attempts - 1)
    {
      ouly::detail::pause_exec();
    }
  }

  local_recent_failures++;
  reset_to_workgroup(thread, preserve_group);
  return false;
}

} // namespace ouly::inline v2

void ouly::v2::task_context::cooperative_wait(std::binary_semaphore& event) const
{
  using namespace std::chrono_literals;
  constexpr uint32_t spin_limit = 64;

  uint32_t idle_spins = 0;
  while (!event.try_acquire())
  {
    // Help execute queued work while waiting.
    if (owner_->busy_work(index_))
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
    // spinning, so a waiting thread does not pin a core.
    if (event.try_acquire_for(100us))
    {
      return;
    }
  }
}
