
#include <acl/scheduler/scheduler.hpp>
#include <acl/scheduler/task.hpp>
#include <latch>
#include <numeric>

namespace acl
{

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
thread_local acl::detail::worker const* g_worker = nullptr;

auto worker_context::get(workgroup_id group) noexcept -> worker_context const&
{
  return g_worker->contexts_[group.get_index()];
}

auto worker_id::get() noexcept -> worker_id const&
{
  return g_worker->id_;
}

scheduler::~scheduler() noexcept
{
  if (!stop_.load())
  {
    end_execution();
  }
}

inline void scheduler::do_work(worker_id thread, acl::detail::work_item& work) noexcept
{
  work(workers_[thread.get_index()].contexts_[work.get_compressed_data<acl::workgroup_id>().get_index()]);
}

void scheduler::busy_work(worker_id thread) noexcept
{
  {
    auto& lw = local_work_[thread.get_index()];
    if (lw)
    {
      do_work(thread, lw);
      lw = nullptr;
      return;
    }
  }

  work(thread);
}

void scheduler::run(worker_id thread)
{
  g_worker = &workers_[thread.get_index()];

  entry_fn_(worker_desc(thread, group_ranges_[thread.get_index()].mask_));

  while (true)
  {
    {
      auto& lw = local_work_[thread.get_index()];
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

    wake_status_[thread.get_index()].store(false);
    wake_events_[thread.get_index()].wait();
  }

  workers_[thread.get_index()].quitting_.store(true);
}

inline auto scheduler::work(worker_id thread) noexcept -> bool
{
  auto wrk = get_work(thread);
  if (!wrk)
  {
    return false;
  }
  assert(&workers_[thread.get_index()].contexts_[wrk.get_compressed_data<workgroup_id>().get_index()].get_scheduler() ==
         this);
  do_work(thread, wrk);
  return true;
}

auto scheduler::get_work(worker_id thread) noexcept -> acl::detail::work_item
{
  auto const& range = group_ranges_[thread.get_index()];

  // try to get work from own queue
  for (uint32_t start = 0; start < range.count_; ++start)
  {
    auto  group_id = range.priority_order_[start];
    auto& group    = workgroups_[group_id];
    for (uint32_t queue_idx = 0, queue_end = group.thread_count_ - 1; queue_idx <= queue_end; ++queue_idx)
    {
      auto& queue = group.work_queues_[(thread.get_index() - group.start_thread_idx_ + queue_idx) & queue_end];
      if (queue.first.try_lock())
      {
        if (!queue.second.empty())
        {
          auto item = queue.second.pop_front_unsafe();
          queue.first.unlock();
          return item;
        }
        queue.first.unlock();
      }
    }
  }

  // Exclusive
  {
    auto& worker    = workers_[thread.get_index()];
    auto& work_list = worker.exlusive_items_;
    auto  lck       = std::scoped_lock(work_list.first);
    if (!work_list.second.empty())
    {
      return work_list.second.pop_front_unsafe();
    }
  }

  return {};
}

void scheduler::wake_up(worker_id thread) noexcept
{
  bool sleeping = true;
  if (!wake_status_[thread.get_index()].exchange(true))
  {
    wake_events_[thread.get_index()].notify();
    return;
  }
}

void scheduler::begin_execution(scheduler_worker_entry&& entry, void* user_context)
{
  local_work_   = std::make_unique<acl::detail::work_item[]>(worker_count_);
  workers_      = std::make_unique<acl::detail::worker[]>(worker_count_);
  group_ranges_ = std::make_unique<acl::detail::group_range[]>(worker_count_);
  wake_status_  = std::make_unique<std::atomic_bool[]>(worker_count_);
  wake_events_  = std::make_unique<acl::detail::wake_event[]>(worker_count_);
  workers_      = std::make_unique<acl::detail::worker[]>(worker_count_);

  threads_.reserve(worker_count_ - 1);

  auto wgroup_count = static_cast<uint32_t>(workgroups_.size());

  for (uint32_t group = 0; group < wgroup_count; ++group)
  {
    auto const& g = workgroups_[group];

    for (uint32_t i = g.start_thread_idx_, end = g.thread_count_ + i; i < end; ++i)
    {
      auto& worker = workers_[i];
      auto& range  = group_ranges_[i];
      range.mask_ |= 1U << group;
      range.priority_order_[range.count_++] = static_cast<uint8_t>(group);
    }
  }

  for (uint32_t w = 0; w < worker_count_; ++w)
  {
    auto& worker = workers_[w];
    worker.id_   = worker_id(w);
    auto& range  = group_ranges_[w];

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
      worker.contexts_[g] = worker_context(*this, user_context, worker_id(w), workgroup_id(g), group_ranges_[w].mask_,
                                           w - workgroups_[g].start_thread_idx_);
    }
    wake_status_[w].store(true);
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

  entry_fn_(worker_desc(worker_id(0), group_ranges_[0].mask_));

  for (uint32_t thread = 1; thread < worker_count_; ++thread)
  {
    threads_.emplace_back(&scheduler::run, this, worker_id(thread));
  }

  g_worker = &workers_[0];
  start_counter.wait();
  entry_fn_ = {};
}

void scheduler::take_ownership() noexcept
{
  g_worker = &workers_[0];
}

void scheduler::finish_pending_tasks() noexcept
{

  bool has_work = false;
  while (true)
  {
    has_work = false;
    for (auto& group : workgroups_)
    {
      bool has_items = false;
      for (uint32_t q = 0; q < group.thread_count_; ++q)
      {
        auto lck = std::scoped_lock(group.work_queues_[q].first);
        has_items |= !group.work_queues_[q].second.empty();
      }
      if (has_items)
      {
        for (uint32_t w = group.start_thread_idx_, end = w + group.thread_count_; w < end; ++w)
        {
          wake_up(worker_id(w));
        }
      }

      has_work |= has_items;
    }

    for (uint32_t w = 0; w < worker_count_; ++w)
    {
      auto& worker    = workers_[w];
      bool  has_items = false;
      {
        auto lck  = std::scoped_lock(worker.exlusive_items_.first);
        has_items = !worker.exlusive_items_.second.empty();
      }

      has_work |= has_items;
      if (!has_items)
      {
        wake_up(worker_id(w));
      }
    }
    if (!has_work)
    {
      break;
    }
  }
}

void scheduler::end_execution()
{
  finish_pending_tasks();
  stop_ = true;
  for (uint32_t thread = 1; thread < worker_count_; ++thread)
  {
    while (!workers_[thread].quitting_.load())
    {
      wake_up(worker_id(thread));
    }
    threads_[thread - 1].join();
  }
  threads_.clear();
}

void scheduler::submit(worker_id src, worker_id dst, acl::detail::work_item work)
{
  if (src == dst)
  {
    do_work(src, work);
  }
  else
  {
    {
      auto& worker = workers_[dst.get_index()];
      auto  lck    = std::scoped_lock(worker.exlusive_items_.first);
      worker.exlusive_items_.second.emplace_back(std::move(work));
    }
    if (!wake_status_[dst.get_index()].exchange(true))
    {
      wake_events_[dst.get_index()].notify();
    }
  }
}

void scheduler::submit(worker_id src, workgroup_id dst, acl::detail::work_item work)
{
  auto& wg     = workgroups_[dst.get_index()];
  auto& worker = workers_[src.get_index()];

  for (uint32_t i = wg.start_thread_idx_, end = i + wg.thread_count_; i != end; ++i)
  {
    bool sleeping = true;
    if (!wake_status_[i].exchange(true))
    {
      local_work_[i] = work;
      wake_events_[i].notify();
      return;
    }
  }

  while (true)
  {
    wg.push_offset_++;
    for (uint32_t i = 0, end = wg.thread_count_ - 1; i <= end; ++i)
    {
      uint32_t q     = (wg.push_offset_ + i) & end;
      auto&    queue = wg.work_queues_[q];
      if (queue.first.try_lock())
      {
        queue.second.emplace_back(std::move(work));
        queue.first.unlock();
        q += wg.start_thread_idx_;
        if (!wake_status_[q].exchange(true))
        {
          wake_events_[q].notify();
        }
        return;
      }
    }
  }
}

void scheduler::create_group(workgroup_id group, uint32_t thread_offset, uint32_t thread_count, uint32_t priority)
{
  if (group.get_index() >= workgroups_.size())
  {
    workgroups_.resize(group.get_index() + 1);
  }
  // thread_count = acl::next_pow2(thread_count);
  worker_count_ =
   std::max(workgroups_[group.get_index()].create_group(thread_offset, thread_count, priority), worker_count_);
}

auto scheduler::create_group(uint32_t thread_offset, uint32_t thread_count, uint32_t priority) -> workgroup_id
{
  workgroups_.emplace_back();
  // thread_count = acl::next_pow2(thread_count);
  worker_count_ = std::max(workgroups_.back().create_group(thread_offset, thread_count, priority), worker_count_);
  // no empty group found
  return workgroup_id(static_cast<uint32_t>(workgroups_.size() - 1));
}

void scheduler::clear_group(workgroup_id group)
{
  workgroups_[group.get_index()].start_thread_idx_ = 0;
  workgroups_[group.get_index()].thread_count_     = 0;
  workgroups_[group.get_index()].push_offset_      = 0;
  workgroups_[group.get_index()].work_queues_      = nullptr;
}

} // namespace acl
