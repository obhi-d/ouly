#include <acl/scheduler/scheduler.hpp>
#include <catch2/catch_all.hpp>
#include <ranges>

TEST_CASE("scheduler: Construction")
{
  acl::scheduler scheduler;
  scheduler.create_group(0, "default", 0, 16);
  scheduler.create_group(0, "io", 16, 2);

  scheduler.begin_execution();

  struct executor
  {
    std::array<uint32_t, 1024> executed = {0};

    void execute(acl::worker_id id)
    {
      executed[id.get_index()]++;
    }

    uint32_t sum() const
    {
      uint32_t result = 0;
      return std::accumulate(executed.begin(), executed.end(), 0);
    }
  };

  executor instance;
  for (uint32_t i = 0; i < instance.executed.size(); ++i)
    scheduler.submit<&executor::execute>(instance, i % 1, acl::worker_id());
  scheduler.end_execution();

  REQUIRE(instance.sum() == 1024);
}
