// SPDX-License-Identifier: MIT

#include "ouly/scheduler/scheduler.hpp"
#include "ouly/scheduler/task.hpp"
#include <latch>
#if defined(_MSC_VER)
#include <intrin.h>                            // _mm_pause / __yield / YieldProcessor
#elif defined(__i386__) || defined(__x86_64__) // 32- & 64-bit x86
#include <immintrin.h>                         // x86 GCC/Clang
#elif defined(__arm__) || defined(__aarch64__)
#include <arm_acle.h> // __builtin_arm_yield (GCC/Clang ≥ 9)
#endif

namespace ouly
{

// Always-inline to keep the call zero-overhead in tight loops
#ifndef _MSC_VER
[[gnu::always_inline]]
#endif
inline void pause_exec() noexcept
{
#if defined(__i386__) || defined(__x86_64__) // 32- & 64-bit x86
#if defined(_MSC_VER)
  _mm_pause(); // MSVC / Intel C++ :contentReference[oaicite:0]{index=0}
#else
  __builtin_ia32_pause(); // GCC / Clang
#endif

#elif defined(__aarch64__) || defined(__arm__) // Arm & AArch64
#if defined(_MSC_VER)
  __yield(); // MSVC intrinsic
#else
  __builtin_arm_yield(); // ACLE 8.4 intrinsic
#endif

#elif defined(__riscv) // RISC-V Zihintpause
  __builtin_riscv_pause(); // GCC/Clang ≥ 13

#elif defined(YieldProcessor) // any Windows target
  YieldProcessor(); // expands appropriately

#else // last-ditch fallback
  std::this_thread::yield();
#endif
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
thread_local ouly::detail::worker const* g_worker = nullptr;
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
thread_local ouly::worker_id g_worker_id = {};
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
thread_local uint32_t g_random_seed = 0;

// LCG constants for fast PRNG
static constexpr uint32_t lcg_multiplier    = 1664525U;
static constexpr uint32_t lcg_increment     = 1013904223U;
static constexpr uint32_t initial_seed_mask = 0xAAAAAAAAU;

auto update_seed() -> uint32_t
{
  return (g_random_seed = (g_random_seed * lcg_multiplier + lcg_increment) & initial_seed_mask);
}

auto worker_context::get(workgroup_id group) noexcept -> worker_context const&
{
  return g_worker->contexts_[group.get_index()];
}

auto worker_id::this_worker::get_id() noexcept -> worker_id const&
{
  return g_worker_id;
}

struct scheduler::worker_synchronizer
{
  std::latch job_board_freeze_ack_;
  std::latch job_board_exhausted_ack_;

  worker_synchronizer(ptrdiff_t worker_count) noexcept
      : job_board_freeze_ack_(worker_count), job_board_exhausted_ack_(worker_count)
  {}
};

scheduler::~scheduler() noexcept
{
  if (!stop_.load())
  {
    end_execution();
  }
}

// NOLINTNEXTLINE
inline void scheduler::do_work(worker_id thread, ouly::detail::work_item& work) noexcept
{
  work(memory_block_.workers_[thread.get_index()]
        .get()
        .contexts_[work.get_compressed_data<ouly::workgroup_id>().get_index()]);
}

void scheduler::busy_work(worker_id thread) noexcept
{
  // Optimized work stealing with limited attempts
  constexpr uint32_t max_busy_attempts = 3;
  for (uint32_t attempt = 0; attempt < max_busy_attempts; ++attempt)
  {
    if (work(thread))
    {
      return; // Found and executed work
    }

    // Brief pause between attempts to reduce CPU usage
    pause_exec();
  }
}

void scheduler::run(worker_id thread)
{
  g_worker    = &memory_block_.workers_[thread.get_index()].get();
  g_worker_id = thread;

  // LCG constants for fast PRNG

  // Random seed for randomized stealing - use thread index and a simple counter for variety
  g_random_seed = thread.get_index() ^ initial_seed_mask;

  entry_fn_(worker_desc(thread, memory_block_.group_ranges_[thread.get_index()].mask_));

  while (true)
  {
    while (work(thread))
    {
      ;
    }

    if (stop_.load(std::memory_order_seq_cst))
    {
      break;
    }

    memory_block_.wake_data_[thread.get_index()].get().status_.store(false, std::memory_order_relaxed);
    memory_block_.wake_data_[thread.get_index()].get().event_.wait();
  }

  if (synchronizer_ != nullptr)
  {
    synchronizer_->job_board_freeze_ack_.count_down();
    synchronizer_->job_board_freeze_ack_.wait();
  }

  finalize_worker(thread);
}

inline void scheduler::finalize_worker(worker_id thread) noexcept
{
  while (work(thread))
  {
  }

  if (synchronizer_ != nullptr)
  {
    synchronizer_->job_board_exhausted_ack_.count_down();
    synchronizer_->job_board_exhausted_ack_.wait();
  }

  g_worker    = nullptr;
  g_worker_id = {};
}

inline auto scheduler::work(worker_id thread) noexcept -> bool
{
  ouly::detail::work_item wrk;
  if (!get_work(thread, wrk))
  {
    return false;
  }
  OULY_ASSERT(&memory_block_.workers_[thread.get_index()]
                .get()
                .contexts_[wrk.get_compressed_data<workgroup_id>().get_index()]
                .get_scheduler() == this);
  do_work(thread, wrk);
  return true;
}

namespace detail
{
// Adaptive work stealing strategy to reduce contention
class adaptive_work_stealer
{
private:
  thread_local static uint32_t                              recent_failures;
  thread_local static uint32_t                              success_streak;
  thread_local static std::chrono::steady_clock::time_point last_success;

public:
  static void record_success() noexcept
  {
    success_streak++;
    recent_failures = std::max(0U, recent_failures - 2); // Decay failure count
    last_success    = std::chrono::steady_clock::now();
  }

  static void record_failure() noexcept
  {
    recent_failures++;
    success_streak = 0;
  }

  static auto get_adaptive_delay() noexcept -> uint32_t
  {
    constexpr uint32_t max_base_delay  = 64U;
    constexpr uint32_t max_total_delay = 256U;

    // Exponential backoff based on recent failures, capped at reasonable maximum
    uint32_t base_delay = std::min(recent_failures * 2, max_base_delay);

    // If we haven't had success recently, increase delay more aggressively
    auto now                = std::chrono::steady_clock::now();
    auto time_since_success = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_success);

    constexpr auto threshold = std::chrono::milliseconds(10);
    if (time_since_success > threshold)
    {
      base_delay *= 2;
    }

    return std::min(base_delay, max_total_delay);
  }

  static auto should_yield() noexcept -> bool
  {
    constexpr uint32_t high_failure_threshold   = 10;
    constexpr uint32_t medium_failure_threshold = 5;

    return recent_failures > high_failure_threshold ||
           (recent_failures > medium_failure_threshold && success_streak == 0);
  }
};

thread_local uint32_t                              adaptive_work_stealer::recent_failures = 0;
thread_local uint32_t                              adaptive_work_stealer::success_streak  = 0;
thread_local std::chrono::steady_clock::time_point adaptive_work_stealer::last_success =
 std::chrono::steady_clock::now();
} // namespace detail

// Work stealing implementation
auto scheduler::try_steal_work(worker_id thread, ouly::detail::work_item& work) noexcept -> bool
{
  uint32_t start_index  = update_seed() % worker_count_;
  uint32_t thread_index = thread.get_index();

  // Try to steal from other workers' local queues
  for (uint32_t attempt = 0; attempt < worker_count_ - 1; ++attempt)
  {
    uint32_t target_index = (start_index + attempt) % worker_count_;

    // Skip our own thread
    if (target_index == thread_index)
    {
      continue;
    }

    auto& target_worker = memory_block_.workers_[target_index].get();

    // Try to steal from target worker's local queue
    if (target_worker.local_work_queue_.pop(work))
    {
      return true;
    }
  }

  // Try to steal from workgroup queues that this thread doesn't belong to
  auto& range = memory_block_.group_ranges_[thread_index];
  for (uint32_t group_idx = 0; group_idx < workgroups_.size(); ++group_idx)
  {
    // Skip workgroups this thread already checked
    if ((range.mask_ & (1U << group_idx)) != 0)
    {
      continue;
    }

    auto& workgroup = workgroups_[group_idx];
    if (workgroup.pop_item(work))
    {
      return true;
    }
  }

  return false;
}

// NOLINTNEXTLINE
auto scheduler::get_work(worker_id thread, ouly::detail::work_item& work) noexcept -> bool
{
  auto& worker = memory_block_.workers_[thread.get_index()].get();
  auto& range  = memory_block_.group_ranges_[thread.get_index()];

  // Priority 1: Check local worker queue first (exclusive work items have highest priority)
  if (worker.pop_item(work))
  {
    detail::adaptive_work_stealer::record_success();
    return true;
  }

  // Priority 2: Check workgroups in priority order
  for (uint32_t i = 0; i < range.count_; ++i)
  {
    uint32_t group_idx = range.priority_order_[i];
    auto&    workgroup = workgroups_[group_idx];

    if (workgroup.pop_item(work))
    {
      detail::adaptive_work_stealer::record_success();
      return true;
    }
  }

  // Priority 3: Work stealing from other threads
  // Try to steal from other workers' local queues
  if (try_steal_work(thread, work))
  {
    detail::adaptive_work_stealer::record_success();
    return true;
  }

  detail::adaptive_work_stealer::record_failure();

  // Apply adaptive backoff if work stealing is failing frequently
  if (detail::adaptive_work_stealer::should_yield())
  {
    std::this_thread::yield();
  }
  else
  {
    uint32_t delay = detail::adaptive_work_stealer::get_adaptive_delay();
    for (uint32_t i = 0; i < delay; ++i)
    {
      pause_exec();
    }
  }

  return false; // Empty work item
}

// NOLINTNEXTLINE
void scheduler::wake_up(worker_id thread) noexcept
{
  auto& wake = memory_block_.wake_data_[thread.get_index()].get();
  if (!wake.status_.exchange(true, std::memory_order_acq_rel))
  {
    wake.event_.notify();
  }
}

void scheduler::begin_execution(scheduler_worker_entry&& entry, void* user_context)
{
  memory_block_.workers_      = std::make_unique<aligned_worker[]>(worker_count_);
  memory_block_.group_ranges_ = std::make_unique<ouly::detail::group_range[]>(worker_count_);
  memory_block_.wake_data_    = std::make_unique<aligned_wake_data[]>(worker_count_);

  threads_.reserve(worker_count_ - 1);

  auto wgroup_count = static_cast<uint32_t>(workgroups_.size());

  for (uint32_t group = 0; group < wgroup_count; ++group)
  {
    auto const& g = workgroups_[group];

    for (uint32_t i = g.start_thread_idx_, end = g.thread_count_ + i; i < end; ++i)
    {
      auto& range = memory_block_.group_ranges_[i];
      range.mask_ |= 1U << group;
      range.priority_order_[range.count_++] = static_cast<uint8_t>(group);
    }
  }

  for (uint32_t w = 0; w < worker_count_; ++w)
  {
    auto& worker = memory_block_.workers_[w].get();
    worker.id_   = worker_id(w);
    auto& range  = memory_block_.group_ranges_[w];

    std::sort(range.priority_order_.data(), range.priority_order_.data() + range.count_,
              [&](uint8_t first, uint8_t second)
              {
                return workgroups_[first].priority_ == workgroups_[second].priority_
                        ? first < second
                        : workgroups_[first].priority_ > workgroups_[second].priority_;
              });

    worker.contexts_ = std::make_unique<worker_context[]>(wgroup_count);
    for (uint32_t g = 0; g < wgroup_count; ++g)
    {
      worker.contexts_[g] = worker_context(*this, user_context, worker_id(w), workgroup_id(g),
                                           memory_block_.group_ranges_[w].mask_, w - workgroups_[g].start_thread_idx_);
    }
    memory_block_.wake_data_[w].get().status_.store(true, std::memory_order_relaxed);
  }

  stop_              = false;
  auto start_counter = std::latch(worker_count_);

  entry_fn_ = [cust_entry = std::move(entry), &start_counter](worker_desc worker)
  {
    if (cust_entry)
    {
      cust_entry(worker);
    }
    start_counter.count_down();
  };

  entry_fn_(worker_desc(worker_id(0), memory_block_.group_ranges_[0].mask_));

  for (uint32_t thread = 1; thread < worker_count_; ++thread)
  {
    threads_.emplace_back(&scheduler::run, this, worker_id(thread));
  }

  g_worker    = &memory_block_.workers_[0].get();
  g_worker_id = worker_id(0);
  start_counter.wait();
  entry_fn_ = {};
}

// NOLINTNEXTLINE
void scheduler::take_ownership() noexcept
{
  g_worker    = &memory_block_.workers_[0].get();
  g_worker_id = worker_id(0);
}

// NOLINTNEXTLINE
void scheduler::finish_pending_tasks() noexcept
{
  constexpr uint32_t yield_interval  = 10; // Yield every N iterations
  uint32_t           iteration_count = 0;

  synchronizer_ = std::make_shared<worker_synchronizer>(worker_count_);
  stop_         = true;

  synchronizer_->job_board_freeze_ack_.count_down();
  while (!synchronizer_->job_board_freeze_ack_.try_wait())
  {
    for (uint32_t thread = 1; thread < worker_count_; ++thread)
    {
      // Wake up each worker to finish their tasks
      wake_up(worker_id(thread));
    }

    // Wait for all workers to acknowledge freeze
    pause_exec();
  }

  bool retry = true;
  while (retry)
  {
    retry         = false;
    bool has_work = false;

    // Check workgroup queues efficiently
    for (auto& workgroup : workgroups_)
    {
      if (workgroup.thread_count_ == 0)
      {
        continue;
      }

      // Check if workgroup has pending work
      if (!workgroup.local_work_queue_.empty())
      {
        // Wake up all threads in this group
        for (uint32_t w = workgroup.start_thread_idx_, end = w + workgroup.thread_count_; w < end; ++w)
        {
          wake_up(worker_id(w));
        }
        has_work = true;
      }
    }

    if (has_work)
    {
      busy_work(worker_id(0)); // Process local work first
    }

    // Check exclusive worker items efficiently
    for (uint32_t w = 0; w < worker_count_; ++w)
    {
      auto& worker = memory_block_.workers_[w].get();

      // Check if worker has pending work in local queue or exclusive items
      bool worker_has_work = !worker.local_work_queue_.empty();

      if (!worker_has_work)
      {
        // Check exclusive items queue (protected by lock)
        if (worker.items_.first.try_lock())
        {
          worker_has_work = !worker.items_.second.empty();
          worker.items_.first.unlock();
        }
      }

      if (worker_has_work)
      {
        wake_up(worker_id(w));
        has_work = true;
        retry    = true; // Retry if we found work in any worker queue
      }
    }

    if (!has_work)
    {
      break;
    }

    ++iteration_count;

    // Small delay between iterations to allow workers to process
    if (iteration_count % yield_interval == 0)
    {
      std::this_thread::yield();
    }
  }

  synchronizer_->job_board_exhausted_ack_.count_down();
  synchronizer_->job_board_exhausted_ack_.wait();
}

void scheduler::end_execution()
{
  finish_pending_tasks();
  for (uint32_t thread = 1; thread < worker_count_; ++thread)
  {
    threads_[thread - 1].join();
  }
  threads_.clear();
}

void scheduler::submit(worker_id src, worker_id dst, ouly::detail::work_item work)
{
  if (src == dst)
  {
    do_work(src, work);
  }
  else
  {
    auto& target_worker = memory_block_.workers_[dst.get_index()].get();

    // Try to push to target worker's local queue first
    if (!target_worker.push_item(std::move(work)))
    {
      // If local queue is full, fall back to doing the work on current thread
      do_work(src, work);
    }
    else
    {
      // Wake up the target worker
      wake_up(dst);
    }
  }
}

void scheduler::submit([[maybe_unused]] worker_id src, workgroup_id dst, ouly::detail::work_item work)
{
  auto& wg = workgroups_[dst.get_index()];

  // Load balancing: Try to distribute work among threads in the workgroup
  // Use round-robin assignment to balance load
  static thread_local std::atomic<uint32_t> round_robin_counter{0};
  uint32_t start_offset = round_robin_counter.fetch_add(1, std::memory_order_relaxed) % wg.thread_count_;

  // First, try to find an idle thread in the workgroup
  for (uint32_t attempt = 0; attempt < wg.thread_count_; ++attempt)
  {
    uint32_t worker_offset = (start_offset + attempt) % wg.thread_count_;
    uint32_t worker_index  = wg.start_thread_idx_ + worker_offset;

    auto& wake_data = memory_block_.wake_data_[worker_index].get();

    // If worker is sleeping, try to assign work directly to it
    if (!wake_data.status_.load(std::memory_order_acquire))
    {
      auto& target_worker = memory_block_.workers_[worker_index].get();
      if (target_worker.push_item(std::move(work)))
      {
        wake_up(worker_id(worker_index));
        return;
      }
    }
  }

  // If no idle threads found, push to workgroup shared queue
  if (!wg.push_item(std::move(work)))
  {
    // If workgroup queue is full, wake up all threads in the workgroup
    for (uint32_t i = wg.start_thread_idx_, end = i + wg.thread_count_; i < end; ++i)
    {
      wake_up(worker_id(i));
    }
  }
}

void scheduler::create_group(workgroup_id group, uint32_t thread_offset, uint32_t thread_count, uint32_t priority)
{
  if (group.get_index() >= workgroups_.size())
  {
    workgroups_.resize(group.get_index() + 1);
  }
  // thread_count = ouly::next_pow2(thread_count);
  worker_count_ =
   std::max(workgroups_[group.get_index()].create_group(thread_offset, thread_count, priority), worker_count_);
}

auto scheduler::create_group(uint32_t thread_offset, uint32_t thread_count, uint32_t priority) -> workgroup_id
{
  workgroups_.emplace_back();
  // thread_count = ouly::next_pow2(thread_count);
  worker_count_ = std::max(workgroups_.back().create_group(thread_offset, thread_count, priority), worker_count_);
  // no empty group found
  return workgroup_id(static_cast<uint32_t>(workgroups_.size() - 1));
}

void scheduler::clear_group(workgroup_id group)
{
  auto& wg = workgroups_[group.get_index()];

  // Safely clear the workgroup by ensuring all operations are complete
  wg.start_thread_idx_ = 0;
  wg.thread_count_     = 0;
}

} // namespace ouly
