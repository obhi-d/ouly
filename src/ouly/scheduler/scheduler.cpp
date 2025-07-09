// SPDX-License-Identifier: MIT

#include "ouly/scheduler/scheduler.hpp"
#include "ouly/scheduler/task.hpp"
#include <latch>
#if defined(_MSC_VER)
#include <intrin.h> // _mm_pause / __yield / YieldProcessor
#else
#include <immintrin.h> // x86 GCC/Clang
#if defined(__arm__) || defined(__aarch64__)
#include <arm_acle.h> // __builtin_arm_yield (GCC/Clang ≥ 9)
#endif
#endif
namespace ouly
{

// Always-inline to keep the call zero-overhead in tight loops
[[gnu::always_inline]] inline void pause_exec() noexcept
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

auto worker_context::get(workgroup_id group) noexcept -> worker_context const&
{
  return g_worker->contexts_[group.get_index()];
}

auto worker_id::this_worker::get_id() noexcept -> worker_id const&
{
  return g_worker_id;
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
  work(
   memory_block_.workers_[thread.get_index()].contexts_[work.get_compressed_data<ouly::workgroup_id>().get_index()]);
}

void scheduler::busy_work(worker_id thread) noexcept
{
  // Fast path: Check local work first
  auto& lw = memory_block_.local_work_[thread.get_index()];
  if (lw)
  {
    do_work(thread, lw);
    lw = nullptr;
    return;
  }

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
  g_worker    = &memory_block_.workers_[thread.get_index()];
  g_worker_id = thread;

  entry_fn_(worker_desc(thread, memory_block_.group_ranges_[thread.get_index()].mask_));

  while (true)
  {
    {
      auto& lw = memory_block_.local_work_[thread.get_index()];
      if (lw)
      {
        do_work(thread, lw);
        lw = nullptr;
      }
    }

    while (work(thread))
    {
      ;
    }

    if (stop_.load(std::memory_order_seq_cst))
    {
      break;
    }

    memory_block_.wake_data_[thread.get_index()].status_.store(false, std::memory_order_relaxed);
    memory_block_.wake_data_[thread.get_index()].event_.wait();
  }

  memory_block_.workers_[thread.get_index()].quitting_.store(true);
}

inline auto scheduler::work(worker_id thread) noexcept -> bool
{
  auto wrk = get_work(thread);
  if (!wrk)
  {
    return false;
  }
  OULY_ASSERT(&memory_block_.workers_[thread.get_index()]
                .contexts_[wrk.get_compressed_data<workgroup_id>().get_index()]
                .get_scheduler() == this);
  do_work(thread, wrk);
  return true;
}

namespace
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
} // namespace

// NOLINTNEXTLINE
auto scheduler::get_work(worker_id thread) noexcept -> ouly::detail::work_item
{
  auto const  thread_idx = thread.get_index();
  auto const& range      = memory_block_.group_ranges_[thread_idx];

  // LCG constants for fast PRNG
  static constexpr uint32_t lcg_multiplier    = 1664525U;
  static constexpr uint32_t lcg_increment     = 1013904223U;
  static constexpr uint32_t initial_seed_mask = 0xAAAAAAAAU;

  // Random seed for randomized stealing - use thread index and a simple counter for variety
  static thread_local uint32_t steal_seed = thread_idx ^ initial_seed_mask;

  // First, try to get work from own queues (highest priority)
  for (uint32_t start = 0; start < range.count_; ++start)
  {
    auto  group_id = range.priority_order_[start];
    auto& group    = workgroups_[group_id];

    if (group.thread_count_ == 0)
    {
      continue;
    }

    auto const my_queue_idx = thread_idx - group.start_thread_idx_;
    if (my_queue_idx < group.thread_count_)
    {
      auto& my_queue = group.work_queues_[my_queue_idx];
      // Use try_lock with immediate return to minimize contention
      if (my_queue.first.try_lock())
      {
        if (!my_queue.second.empty())
        {
          auto item = my_queue.second.pop_front_unsafe();
          my_queue.first.unlock();
          adaptive_work_stealer::record_success();
          return item;
        }
        my_queue.first.unlock();
      }
    }
  }

  // Then try exclusive items (medium priority)
  {
    auto& worker    = memory_block_.workers_[thread_idx];
    auto& work_list = worker.exclusive_items_;
    if (work_list.first.try_lock())
    {
      if (!work_list.second.empty())
      {
        auto item = work_list.second.pop_front_unsafe();
        work_list.first.unlock();
        adaptive_work_stealer::record_success();
        return item;
      }
      work_list.first.unlock();
    }
  }

  // Finally, try work stealing with optimized strategy (lowest priority)
  uint32_t           steal_attempts     = 0;
  constexpr uint32_t max_steal_attempts = 8; // Limit attempts to prevent excessive spinning

  for (uint32_t group_idx = 0; group_idx < range.count_ && steal_attempts < max_steal_attempts; ++group_idx)
  {
    auto  group_id = range.priority_order_[group_idx];
    auto& group    = workgroups_[group_id];

    if (group.thread_count_ <= 1)
    {
      continue; // Skip single-worker groups
    }

    // Randomized victim selection to reduce contention hot spots
    steal_seed              = steal_seed * lcg_multiplier + lcg_increment;
    auto const queue_count  = group.thread_count_;
    auto const random_start = steal_seed % queue_count;

    // Try up to queue_count/2 victims to balance thoroughness vs performance
    auto const max_victims = std::min(queue_count, (queue_count + 1) / 2);

    for (uint32_t victim_offset = 0; victim_offset < max_victims && steal_attempts < max_steal_attempts;
         ++victim_offset)
    {
      auto const victim_idx   = (random_start + victim_offset) % queue_count;
      auto&      victim_queue = group.work_queues_[victim_idx];

      // Skip our own queue in stealing phase
      if (victim_idx == (thread_idx - group.start_thread_idx_))
      {
        continue;
      }

      ++steal_attempts;

      // Use try_lock to avoid blocking - if we can't get the lock immediately,
      // the queue is likely busy and we should try elsewhere
      if (victim_queue.first.try_lock())
      {
        if (!victim_queue.second.empty())
        {
          // Steal from the front (FIFO) for task ordering consistency
          auto item = victim_queue.second.pop_front_unsafe();
          victim_queue.first.unlock();
          adaptive_work_stealer::record_success();
          return item;
        }
        victim_queue.first.unlock();
      }

      // Micro-backoff to reduce contention - use adaptive strategy
      if (steal_attempts > 2)
      {
        // Use adaptive delay instead of fixed exponential backoff
        uint32_t adaptive_delay = adaptive_work_stealer::get_adaptive_delay();

        if (adaptive_work_stealer::should_yield())
        {
          std::this_thread::yield();
        }
        else
        {
          for (uint32_t delay = adaptive_delay; delay > 0; --delay)
          {
            pause_exec();
          }
        }

        adaptive_work_stealer::record_failure();
      }
    }
  }

  return {};
}

// NOLINTNEXTLINE
void scheduler::wake_up(worker_id thread) noexcept
{
  auto& wake = memory_block_.wake_data_[thread.get_index()];
  if (!wake.status_.exchange(true, std::memory_order_acq_rel))
  {
    wake.event_.notify();
  }
}

void scheduler::begin_execution(scheduler_worker_entry&& entry, void* user_context)
{
  memory_block_.local_work_   = std::make_unique<ouly::detail::work_item[]>(worker_count_);
  memory_block_.workers_      = std::make_unique<ouly::detail::worker[]>(worker_count_);
  memory_block_.group_ranges_ = std::make_unique<ouly::detail::group_range[]>(worker_count_);
  memory_block_.wake_data_    = std::make_unique<wake_data[]>(worker_count_);

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
    auto& worker = memory_block_.workers_[w];
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
    memory_block_.wake_data_[w].status_.store(true, std::memory_order_relaxed);
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

  g_worker    = &memory_block_.workers_[0];
  g_worker_id = worker_id(0);
  start_counter.wait();
  entry_fn_ = {};
}

// NOLINTNEXTLINE
void scheduler::take_ownership() noexcept
{
  g_worker    = &memory_block_.workers_[0];
  g_worker_id = worker_id(0);
}

// NOLINTNEXTLINE
void scheduler::finish_pending_tasks() noexcept
{
  constexpr uint32_t yield_interval  = 10; // Yield every N iterations
  uint32_t           iteration_count = 0;

  while (true)
  {
    bool has_work = false;

    // Check workgroup queues efficiently
    for (auto& group : workgroups_)
    {
      if (group.thread_count_ == 0)
      {
        continue;
      }

      bool group_has_work = false;
      // Use try_lock to avoid blocking on busy queues
      for (uint32_t q = 0; q < group.thread_count_; ++q)
      {
        if (group.work_queues_[q].first.try_lock())
        {
          bool queue_has_work = !group.work_queues_[q].second.empty();
          group.work_queues_[q].first.unlock();

          if (queue_has_work)
          {
            group_has_work = true;
            break; // Found work in this group, no need to check other queues
          }
        }
        else
        {
          // If queue is locked, assume it has work (conservative approach)
          group_has_work = true;
          break;
        }
      }

      if (group_has_work)
      {
        // Wake up all threads in this group
        for (uint32_t w = group.start_thread_idx_, end = w + group.thread_count_; w < end; ++w)
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
      auto& worker          = memory_block_.workers_[w];
      bool  worker_has_work = false;

      if (worker.exclusive_items_.first.try_lock())
      {
        worker_has_work = !worker.exclusive_items_.second.empty();
        worker.exclusive_items_.first.unlock();
      }
      else
      {
        // If worker queue is locked, assume it has work
        worker_has_work = true;
      }

      if (worker_has_work)
      {
        wake_up(worker_id(w));
        has_work = true;
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
}

void scheduler::end_execution()
{
  finish_pending_tasks();
  stop_ = true;
  for (uint32_t thread = 1; thread < worker_count_; ++thread)
  {
    while (!memory_block_.workers_[thread].quitting_.load())
    {
      wake_up(worker_id(thread));
    }
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
    {
      auto& worker = memory_block_.workers_[dst.get_index()];
      auto  lck    = std::scoped_lock(worker.exclusive_items_.first);
      worker.exclusive_items_.second.emplace_back(std::move(work));
    }
    if (!memory_block_.wake_data_[dst.get_index()].status_.exchange(true, std::memory_order_acq_rel))
    {
      memory_block_.wake_data_[dst.get_index()].event_.notify();
    }
  }
}

void scheduler::submit([[maybe_unused]] worker_id src, workgroup_id dst, ouly::detail::work_item work)
{
  auto& wg = workgroups_[dst.get_index()];

  // First, try to find an available thread with local work slot
  for (uint32_t i = wg.start_thread_idx_, end = i + wg.thread_count_; i != end; ++i)
  {
    if (!memory_block_.wake_data_[i].status_.exchange(true, std::memory_order_acq_rel))
    {
      memory_block_.local_work_[i] = work;
      memory_block_.wake_data_[i].event_.notify();
      return;
    }
  }

  // If no local work slot available, use round-robin queue assignment with limited retries
  constexpr uint32_t max_retries = 4; // Prevent infinite spinning
  uint32_t           retry_count = 0;

  while (retry_count < max_retries)
  {
    // Increment push offset for round-robin behavior (simple increment, races are acceptable)
    auto current_offset = wg.push_offset_.get().fetch_add(1, std::memory_order_relaxed);

    for (uint32_t i = 0, end = wg.thread_count_; i < end; ++i)
    {
      uint32_t q     = (current_offset + i) & (wg.thread_count_ - 1);
      auto&    queue = wg.work_queues_[q];

      if (queue.first.try_lock())
      {
        queue.second.emplace_back(std::move(work));
        queue.first.unlock();

        // Wake up the target thread
        auto thread_idx = q + wg.start_thread_idx_;
        if (!memory_block_.wake_data_[thread_idx].status_.exchange(true, std::memory_order_acq_rel))
        {
          memory_block_.wake_data_[thread_idx].event_.notify();
        }
        return;
      }
    }

    ++retry_count;
    // Short backoff between retries to reduce contention
    if (retry_count < max_retries)
    {
      for (uint32_t delay = 1U << retry_count; delay > 0; --delay)
      {
        pause_exec();
      }
    }
  }

  // Fallback: force submission to the first queue if all retries failed
  // This ensures work is never lost, though it may cause temporary contention
  auto& fallback_queue = wg.work_queues_[0];
  auto  lck            = std::scoped_lock(fallback_queue.first);
  fallback_queue.second.emplace_back(std::move(work));

  auto thread_idx = wg.start_thread_idx_;
  if (!memory_block_.wake_data_[thread_idx].status_.exchange(true, std::memory_order_acq_rel))
  {
    memory_block_.wake_data_[thread_idx].event_.notify();
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

  // Store the thread count before clearing for proper cleanup
  uint32_t old_thread_count = wg.thread_count_;

  // Safely clear the workgroup by ensuring all operations are complete
  wg.start_thread_idx_ = 0;
  wg.thread_count_     = 0;
  wg.push_offset_.get().store(0, std::memory_order_release);

  // Clean up work queues safely
  if (wg.work_queues_ != nullptr)
  {
    // Destroy each work queue properly using the correct type
    for (uint32_t i = 0; i < old_thread_count; ++i)
    {
      using queue_type = ouly::detail::async_work_queue;
      wg.work_queues_[i].~queue_type();
    }
    ::operator delete[](wg.work_queues_, std::align_val_t{detail::cache_line_size});
    wg.work_queues_ = nullptr;
  }
}

} // namespace ouly
