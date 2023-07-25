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

  scheduler.begin_execution();

  struct executor
  {
    std::array<uint32_t, 1024> executed = {0};

    void execute(acl::worker_context const& id)
    {
      executed[id.get_worker().get_index()]++;
    }

    uint32_t sum() const
    {
      uint32_t result = 0;
      return std::accumulate(executed.begin(), executed.end(), 0);
    }
  };

  executor instance;
  for (uint32_t i = 0; i < instance.executed.size(); ++i)
    scheduler.submit<&executor::execute>(instance, acl::workgroup_id(i % 1), acl::main_worker_id);

  scheduler.end_execution();

  REQUIRE(instance.sum() == 1024);
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

  scheduler.submit(task, acl::default_workgroup_id, acl::main_worker_id);
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

  scheduler.submit(task, acl::default_workgroup_id, acl::main_worker_id);

  std::string        continue_string = "basic";
  constexpr uint32_t nb_elements     = 1000;
  for (uint32_t i = 0; i < nb_elements; ++i)
  {
    continue_string += "-i-" + std::to_string(i);
  }

  auto result = string_task.sync_wait_result(acl::main_worker_id, scheduler);
  REQUIRE(result == continue_string);
  scheduler.end_execution();
}
