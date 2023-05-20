#include "acl/basic_queue.hpp"
#include <catch2/catch_all.hpp>
#include <string>

struct string_traits
{
  static constexpr uint32_t pool_size = 4;
};

TEST_CASE("Validate basic_queue", "[basic_queue]")
{
  acl::basic_queue<std::string, string_traits> queue;

  for (uint32_t i = 0; i < 100; ++i)
    queue.emplace_back(std::to_string(i));

  for (uint32_t i = 0; i < 100; ++i)
  {
    CHECK(queue.empty() == false);
    CHECK(queue.pop_front() == std::to_string(i));
  }

  CHECK(queue.empty() == true);

  queue.emplace_back(std::to_string(0));
  CHECK(queue.pop_front() == std::to_string(0));
}
