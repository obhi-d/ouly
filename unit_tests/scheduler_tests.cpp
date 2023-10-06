#include <acl/scheduler/parallel_for.hpp>
#include <acl/scheduler/scheduler.hpp>
#include <catch2/catch_all.hpp>
#include <ranges>
#include <string>

TEST_CASE("scheduler: Construction")
{
  acl::scheduler scheduler;
  scheduler.create_group(acl::workgroup_id(0), "default", 0, 16);
  scheduler.create_group(acl::workgroup_id(1), "io", 16, 2);


  struct executor
  {
    std::array<uint32_t, 18> executed = {0};
    std::array<std::vector<uint32_t>, 18> accumulate;

    void execute(acl::worker_context const& id)
    {
      executed[id.get_worker().get_index()]++;
    }

    void execute2(acl::worker_context const& id, uint32_t n)
    {
      accumulate[id.get_worker().get_index()].push_back(n);
    }

    uint32_t sum() const
    {
      uint32_t result = std::accumulate(executed.begin(), executed.end(), 0);
      for(auto const& r : accumulate)
        result += std::accumulate(r.begin(), r.end(), 0);
      return result;
    }

    std::vector<uint32_t> get_missing(uint32_t max) const
    {
      std::vector<uint32_t> result;
      result.resize(max, 0);
      std::iota(result.begin(), result.end(), 0);
      for(auto const& r : accumulate)
      {
        for(auto v : r)
        {
          auto r = std::find(result.begin(), result.end(), v);
          if (r != result.end())
            result.erase(r);
        }
      }
      return result;
    }
  };

  scheduler.begin_execution();
  executor instance;
  for (uint32_t i = 0; i < 1024; ++i)
    acl::async<&executor::execute>(acl::worker_context::get(acl::default_workgroup_id), &instance, acl::workgroup_id(i % 2));
  scheduler.end_execution();

  auto sum = instance.sum();
  // REQUIRE(sum == 1024);

  scheduler.begin_execution();
  executor instance2;
  for (uint32_t i = 0; i < 1024; ++i)
    acl::async<&executor::execute2>(acl::worker_context::get(acl::default_workgroup_id), &instance2, i,
                                   acl::workgroup_id(i % 2));
  scheduler.end_execution();

  if (instance2.sum() != 1023 * 512)
  {
    auto missing = instance2.get_missing(1024);
    scheduler.print_logs();
    for(auto m : missing)
      printf("\nmissing: %d", m);
  }

  REQUIRE(instance2.sum() == 1023 * 512);
}

TEST_CASE("scheduler: Simplest ParallelFor")
{
  acl::scheduler scheduler;
  scheduler.create_group(acl::workgroup_id(0), "default", 0, 16);

  scheduler.begin_execution();

  constexpr uint32_t nb_elements = 10000;
  std::vector<int>   list;
  list.reserve(nb_elements);
  int64_t sum = 0;
  for (uint32_t i = 0; i < nb_elements; ++i)
  {
    list.emplace_back(std::rand());
    sum += list[i];
  }

  std::atomic_int64_t parallel_sum = 0;
  acl::parallel_for(
    [&parallel_sum](int a, acl::worker_context const& c)
    {
      auto id = acl::worker_id::get();
      REQUIRE(id.get_index() < 16);
      auto ctx = acl::worker_context::get(acl::default_workgroup_id);
      REQUIRE(ctx.get_worker().get_index() < 16);
      REQUIRE(ctx.get_group_offset() < 16);
      parallel_sum += a;
    },
    std::span(list.begin(), list.end()), 2, acl::default_workgroup_id);

  REQUIRE(parallel_sum.load() == sum);

  parallel_sum = 0;
  acl::parallel_for(
    [&parallel_sum](auto start, auto end, acl::worker_context const& c)
    {
      for (auto it = start; it != end; ++it)
        parallel_sum += *it;
    },
    std::span(list.begin(), list.end()), 2, acl::default_workgroup_id);

  REQUIRE(parallel_sum.load() == sum);

  scheduler.end_execution();
}

acl::co_task<std::string> continue_string()
{
  std::string        continue_string;
  constexpr uint32_t nb_elements = 1000;
  for (uint32_t i = 0; i < nb_elements; ++i)
  {
    continue_string += "-i-" + std::to_string(i);
  }

  co_return continue_string;
}

acl::co_task<std::string> create_string(acl::co_task<std::string>& tunein)
{
  std::string basic_string = "basic";
  co_await tunein;
  auto tune_result = tunein.result();
  co_return basic_string + tune_result;
}

TEST_CASE("scheduler: Test co_task")
{
  acl::scheduler scheduler;
  scheduler.create_group(acl::workgroup_id(0), "default", 0, 16);

  scheduler.begin_execution();

  auto task        = continue_string();
  auto string_task = create_string(task);

  acl::async(acl::worker_context::get(acl::default_workgroup_id), task, acl::default_workgroup_id);
  scheduler.submit(string_task, acl::default_workgroup_id, acl::main_worker_id);

  std::string        continue_string = "basic";
  constexpr uint32_t nb_elements     = 1000;
  for (uint32_t i = 0; i < nb_elements; ++i)
  {
    continue_string += "-i-" + std::to_string(i);
  }

  auto result = string_task.sync_wait_result();
  REQUIRE(result == continue_string);
  scheduler.end_execution();
}

acl::co_sequence<std::string> create_string_seq(acl::co_task<std::string>& tunein)
{
  std::string basic_string = "basic";
  co_await tunein;
  auto tune_result = tunein.result();
  co_return basic_string + tune_result;
}

TEST_CASE("scheduler: Test co_sequence")
{
  acl::scheduler scheduler;
  scheduler.create_group(acl::workgroup_id(0), "default", 0, 2);

  scheduler.begin_execution();

  auto task        = continue_string();
  auto string_task = create_string_seq(task);

  acl::co_sequence<std::string> move_string_task = std::move(string_task);

  scheduler.submit(task, acl::default_workgroup_id, acl::main_worker_id);

  std::string        continue_string = "basic";
  constexpr uint32_t nb_elements     = 1000;
  for (uint32_t i = 0; i < nb_elements; ++i)
  {
    continue_string += "-i-" + std::to_string(i);
  }

  auto result = move_string_task.sync_wait_result(acl::main_worker_id, scheduler);
  REQUIRE(result == continue_string);
  scheduler.end_execution();
}

acl::co_task<void> work_on(std::vector<uint32_t>& id, std::mutex& lck, uint32_t worker)
{
  auto lock = std::scoped_lock(lck);
  id.push_back(worker);
  co_return;
}

TEST_CASE("scheduler: Test submit_to")
{
  acl::scheduler scheduler;
  auto           wg_default = acl::workgroup_id(0);
  auto           wg_game    = acl::workgroup_id(1);
  auto           wg_log     = acl::workgroup_id(2);
  auto           wg_render  = acl::workgroup_id(3);
  auto           wg_stream  = acl::workgroup_id(4);

  scheduler.create_group(wg_default, "default", 0, 32);
  scheduler.create_group(wg_game, "game", 0, 16);
  scheduler.create_group(wg_log, "log", 16, 1);
  scheduler.create_group(wg_render, "render", 12, 4);
  scheduler.create_group(wg_stream, "stream", 17, 2);

  std::atomic_int default_ = 0;
  std::atomic_int game_    = 0;
  std::atomic_int log_     = 0;
  std::atomic_int render_  = 0;
  std::atomic_int stream_  = 0;

  scheduler.begin_execution(
    [&](acl::worker_desc desc)
    {
      if (desc.belongs_to(wg_default))
        default_++;
      if (desc.belongs_to(wg_game))
        game_++;
      if (desc.belongs_to(wg_log))
        log_++;
      if (desc.belongs_to(wg_render))
        render_++;
      if (desc.belongs_to(wg_stream))
        stream_++;
    });

  REQUIRE(default_.load() == 32);
  REQUIRE(game_.load() == 16);
  REQUIRE(log_.load() == 1);
  REQUIRE(render_.load() == 4);
  REQUIRE(stream_.load() == 2);

  std::vector<acl::co_task<void>> tasks;
  std::vector<uint32_t>           collection;
  std::mutex                      lock;
  for (uint32_t i = 0, end = scheduler.get_worker_count(); i < end; ++i)
  {
    tasks.emplace_back(work_on(collection, lock, i));
    scheduler.submit_to(tasks[i], acl::worker_id(i), acl::main_worker_id);
  }

  for (uint32_t i = 0, end = scheduler.get_worker_count(); i < end; ++i)
    tasks[i].sync_wait_result();

  std::ranges::sort(collection);
  for (uint32_t i = 0, end = scheduler.get_worker_count(); i < end; ++i)
  {
    REQUIRE(i < collection.size());
    REQUIRE(collection[i] == i);
  }
}
