// SPDX-License-Identifier: MIT

#include "ouly/scheduler/scheduler.hpp"
#include "ouly/scheduler/task.hpp"
#include "ouly/scheduler/worker_context.hpp"
#include "ouly/utility/common.hpp"
#include <atomic>
#include <barrier>
#include <cstdint>
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
  _mm_pause(); // MSVC / Intel C++
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
namespace detail
{
auto update_seed() -> uint32_t;
}

auto detail::update_seed() -> uint32_t
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

struct scheduler::tally_publisher
{
  scheduler* owner_ = nullptr;

  void operator()() const noexcept;
};

struct scheduler::worker_synchronizer
{
  std::latch                    job_board_freeze_ack_;
  std::barrier<tally_publisher> published_tally_;
  std::atomic_int64_t           tally_{0};

  worker_synchronizer(ptrdiff_t worker_count, scheduler* owner) noexcept
      : job_board_freeze_ack_(worker_count), published_tally_(worker_count, tally_publisher{owner})
  {}
};

void scheduler::tally_publisher::operator()() const noexcept
{
  // Cache-friendly sequential access with prefetching
  int64_t            total_remaining_tasks = 0;
  constexpr uint32_t prefetch_distance     = 2;

  for (uint32_t thread = 0; thread < owner_->worker_count_; ++thread)
  {
    // Prefetch next worker data to improve cache performance
    if (thread + prefetch_distance < owner_->worker_count_)
    {
      prefetch_for_read(&owner_->memory_block_.workers_[thread + prefetch_distance]);
    }

    total_remaining_tasks += owner_->memory_block_.workers_[thread].get().tally_;
  }
  owner_->synchronizer_->tally_.store(total_remaining_tasks, std::memory_order_release);
}

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
  auto& worker = memory_block_.workers_[thread.get_index()].get();
  worker.tally_--;
  work(worker.contexts_[work.get_compressed_data<ouly::workgroup_id>().get_index()]);
}

void scheduler::busy_work(worker_id thread) noexcept
{
  // Optimized work stealing with adaptive attempts
  constexpr uint32_t min_attempts      = 1;
  constexpr uint32_t max_attempts      = 3;
  constexpr uint32_t failure_threshold = 5;

  // Use thread_local failure tracking for better performance
  thread_local uint32_t local_recent_failures = 0;
  uint32_t              attempts = (local_recent_failures > failure_threshold) ? min_attempts : max_attempts;

  for (uint32_t attempt = 0; attempt < attempts; ++attempt)
  {
    if (work(thread)) [[likely]]
    {
      local_recent_failures = std::max(0U, local_recent_failures - 1);
      return; // Found and executed work
    }

    // Shorter pause for first attempts, longer for subsequent ones
    if (attempt < attempts - 1)
    {
      pause_exec();
    }
  }

  // No work found, increment failure count
  local_recent_failures++;
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

    if (stop_.load(std::memory_order_acquire)) [[unlikely]]
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
  if (synchronizer_ != nullptr)
  {

    auto old_tally = memory_block_.workers_[thread.get_index()].get().tally_;
    synchronizer_->tally_.fetch_add(old_tally, std::memory_order_acq_rel);

    synchronizer_->published_tally_.arrive_and_wait();

    while (synchronizer_->tally_.load(std::memory_order_acquire) > 0)
    {
      while (work(thread))
      {
      }

      synchronizer_->published_tally_.arrive_and_wait();
    }

    // Wait to take stock of the remaning work
  }

  g_worker    = nullptr;
  g_worker_id = {};
}

inline auto scheduler::work(worker_id thread) noexcept -> bool
{
  ouly::detail::work_item available_work;
  if (!get_work(thread, available_work))
  {
    return false;
  }
  OULY_ASSERT(&memory_block_.workers_[thread.get_index()]
                .get()
                .contexts_[available_work.get_compressed_data<workgroup_id>().get_index()]
                .get_scheduler() == this);
  OULY_ASSERT(available_work);
  do_work(thread, available_work);
  return true;
}

namespace detail
{
// Adaptive work stealing strategy to reduce contention
class adaptive_work_stealer
{
private:
  thread_local static uint32_t recent_failures;
  thread_local static uint32_t success_streak;

public:
  static void record_success() noexcept
  {
    success_streak++;
    recent_failures = std::max(0U, recent_failures - 2); // Decay failure count
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

thread_local uint32_t adaptive_work_stealer::recent_failures = 0;
thread_local uint32_t adaptive_work_stealer::success_streak  = 0;
} // namespace detail

// NOLINTNEXTLINE
auto scheduler::get_work(worker_id thread, ouly::detail::work_item& work) noexcept -> bool
{
  auto& range = memory_block_.group_ranges_[thread.get_index()];

  // Check workgroups in priority order - each worker's queue within each workgroup
  for (uint32_t i = 0; i < range.count_; ++i)
  {
    uint32_t group_idx = range.priority_order_[i];
    auto&    workgroup = workgroups_[group_idx];

    // Calculate this worker's offset within the workgroup
    uint32_t worker_offset = thread.get_index() - workgroup.start_thread_idx_;

    // First, try to get work from this worker's own queue in this workgroup
    if (workgroup.pop_item_from_worker(worker_offset, work)) [[likely]]
    {
      detail::adaptive_work_stealer::record_success();
      return true;
    }

    // Then try to steal from other workers' queues within this workgroup
    // Start with adjacent workers for better cache locality
    uint32_t start_steal_offset = detail::update_seed() % workgroup.thread_count_;

    // First try nearby workers (cache-friendly)
    for (uint32_t distance = 1; distance <= workgroup.thread_count_ / 2; ++distance)
    {
      // Try both directions from starting point
      for (int32_t direction = -1; direction <= 1; direction += 2)
      {
        auto offset_calc = static_cast<int32_t>(start_steal_offset) + (direction * static_cast<int32_t>(distance));
        auto target_worker_offset = static_cast<uint32_t>(
         (offset_calc + static_cast<int32_t>(workgroup.thread_count_)) % static_cast<int32_t>(workgroup.thread_count_));

        // Don't steal from ourselves
        if (target_worker_offset == worker_offset)
        {
          continue;
        }

        if (workgroup.pop_item_from_worker(target_worker_offset, work)) [[likely]]
        {
          detail::adaptive_work_stealer::record_success();
          return true;
        }
      }
    }

    // If nearby stealing failed, try remaining workers
    for (uint32_t steal_attempt = 0; steal_attempt < workgroup.thread_count_; ++steal_attempt)
    {
      uint32_t target_worker_offset = (start_steal_offset + steal_attempt) % workgroup.thread_count_;

      // Skip if already checked or is self
      if (target_worker_offset == worker_offset || (steal_attempt <= workgroup.thread_count_ / 2))
      {
        continue;
      }

      if (workgroup.pop_item_from_worker(target_worker_offset, work)) [[unlikely]]
      {
        detail::adaptive_work_stealer::record_success();
        return true;
      }
    }
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

  return false; // No work found
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

void scheduler::assign_priority_order()
{

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
}

auto scheduler::compute_group_range(uint32_t worker_index) -> bool
{
  auto& worker = memory_block_.workers_[worker_index].get();
  worker.id_   = worker_id(worker_index);
  auto& range  = memory_block_.group_ranges_[worker_index];

  std::sort(range.priority_order_.data(), range.priority_order_.data() + range.count_,
            [&](uint8_t first, uint8_t second)
            {
              return workgroups_[first].priority_ == workgroups_[second].priority_
                      ? first < second
                      : workgroups_[first].priority_ > workgroups_[second].priority_;
            });

  // No need for complex steal range calculation with new design
  return true;
}

void scheduler::compute_steal_mask([[maybe_unused]] uint32_t worker_index) const
{
  // No longer needed with the new per-workgroup-per-worker queue design
  // Work stealing is naturally bounded by workgroup membership
}

void scheduler::begin_execution(scheduler_worker_entry&& entry, void* user_context)
{
  memory_block_.workers_      = std::make_unique<aligned_worker[]>(worker_count_);
  memory_block_.group_ranges_ = std::make_unique<ouly::detail::group_range[]>(worker_count_);
  memory_block_.wake_data_    = std::make_unique<aligned_wake_data[]>(worker_count_);

  threads_.reserve(worker_count_ - 1);

  assign_priority_order();

  for (uint32_t worker_index = 0; worker_index < worker_count_; ++worker_index)
  {
    auto& worker = memory_block_.workers_[worker_index].get();
    worker.id_   = worker_id(worker_index);

    compute_group_range(worker_index);

    auto wgroup_count = static_cast<uint32_t>(workgroups_.size());
    worker.contexts_  = std::make_unique<worker_context[]>(wgroup_count);
    for (uint32_t g = 0; g < wgroup_count; ++g)
    {
      worker.contexts_[g] =
       worker_context(*this, user_context, worker_id(worker_index), workgroup_id(g),
                      memory_block_.group_ranges_[worker_index].mask_, worker_index - workgroups_[g].start_thread_idx_);
    }
    memory_block_.wake_data_[worker_index].get().status_.store(true, std::memory_order_relaxed);
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
  synchronizer_ = std::make_shared<worker_synchronizer>(worker_count_, this);

  // First: Signal stop to prevent new task submissions
  stop_.store(true, std::memory_order_seq_cst);

  // Second: Wake all workers to process remaining tasks
  for (uint32_t thread = 1; thread < worker_count_; ++thread)
  {
    wake_up(worker_id(thread));
  }

  // Third: Wait for all workers to acknowledge the stop signal
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

  finalize_worker(worker_id(0));
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

void scheduler::submit_internal([[maybe_unused]] worker_id src, workgroup_id dst, ouly::detail::work_item const& work)
{
  memory_block_.workers_[src.get_index()].get().tally_++;

  auto& wg = workgroups_[dst.get_index()];

  // Load balancing: Try to distribute work among threads in the workgroup
  // Use round-robin assignment to balance load
  uint32_t start_offset = detail::update_seed();

  // First, try to find an idle thread in the workgroup and push directly to its queue
  for (uint32_t attempt = 0; attempt < wg.thread_count_; ++attempt)
  {
    uint32_t worker_offset = (start_offset + attempt) % wg.thread_count_;
    uint32_t worker_index  = wg.start_thread_idx_ + worker_offset;

    auto& worker_wake_data = memory_block_.wake_data_[worker_index].get();

    // If worker is sleeping, try to assign work directly to its workgroup queue
    if (!worker_wake_data.status_.load(std::memory_order_acquire))
    {
      if (wg.push_item_to_worker(worker_offset, work))
      {
        wake_up(worker_id(worker_index));
        return;
      }
    }
  }

  // Try to push to any worker's queue in the workgroup with exponential backoff
  uint32_t           retry_count        = 0;
  constexpr uint32_t max_retries        = 1000;  // Prevent infinite loops
  constexpr uint32_t max_backoff_shift  = 10U;   // Maximum bit shift for backoff
  constexpr uint32_t max_backoff_cycles = 1024U; // Maximum backoff cycles

  while (retry_count < max_retries)
  {
    for (uint32_t attempt = 0; attempt < wg.thread_count_; ++attempt)
    {
      uint32_t worker_offset = (start_offset + attempt) % wg.thread_count_;
      uint32_t worker_index  = wg.start_thread_idx_ + worker_offset;

      if (wg.push_item_to_worker(worker_offset, work))
      {
        wake_up(worker_id(worker_index));
        return;
      }

      if (worker_index == src.get_index())
      {
        busy_work(src);
      }
      // Only wake workers every few failed attempts to reduce overhead
      else if (attempt % 2 == 0)
      {
        wake_up(worker_id(worker_index));
      }
    }

    // Exponential backoff to reduce contention
    uint32_t backoff_cycles = std::min(1U << std::min(retry_count, max_backoff_shift), max_backoff_cycles);
    for (uint32_t i = 0; i < backoff_cycles; ++i)
    {
      pause_exec();
    }
    retry_count++;
  }

  // Fallback: try blocking push to own worker's queue
  uint32_t own_worker_offset = src.get_index() - wg.start_thread_idx_;
  if (own_worker_offset < wg.thread_count_)
  {
    while (!wg.push_item_to_worker(own_worker_offset, work))
    {
      // If we can't push, busy work to yield CPU time
      busy_work(src);
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
