#include "acl/containers/basic_queue.hpp"
#include "catch2/catch_all.hpp"
#include <string>

// NOLINTBEGIN

struct string_traits
{
  static constexpr uint32_t pool_size_v = 4;
};

TEST_CASE("Check basic_queue empty", "[basic_queue]")
{
  acl::basic_queue<std::string, string_traits> queue;
  bool                                         exception = false;
  try
  {
    queue.pop_front();
  }
  catch (std::exception const&)
  {
    CHECK(queue.empty() == true);
    exception = true;
  }
  CHECK(exception == true);
}

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

  queue.clear();
  CHECK(queue.empty() == true);
}

TEST_CASE("Validate basic_queue initialization", "[basic_queue]")
{
  acl::basic_queue<std::string, string_traits> queue1;
  acl::basic_queue<std::string, string_traits> queue2;

  for (uint32_t i = 0; i < 100; ++i)
    queue1.emplace_back(std::to_string(i));

  queue2 = queue1;

  for (uint32_t i = 0; i < 100; ++i)
  {
    CHECK(queue2.empty() == false);
    CHECK(queue2.pop_front() == std::to_string(i));
  }

  for (uint32_t i = 0; i < 100; ++i)
    queue1.emplace_back(std::to_string(i));

  queue2 = std::move(queue1);

  for (int j = 0; j < 2; j++) // we emplaced_back after a copy construction, so 100 more
    for (uint32_t i = 0; i < 100; ++i)
    {
      CHECK(queue2.empty() == false);
      CHECK(queue2.pop_front() == std::to_string(i));
    }

  CHECK(queue2.empty() == true);

  for (uint32_t i = 0; i < 100; ++i)
    queue1.emplace_back(std::to_string(i));

  acl::basic_queue<std::string, string_traits> queue3 = std::move(queue1);

  CHECK(queue1.empty() == true);

  for (uint32_t i = 0; i < 100; ++i)
  {
    CHECK(queue3.empty() == false);
    CHECK(queue3.pop_front() == std::to_string(i));
  }

  CHECK(queue3.empty() == true);

  for (uint32_t i = 0; i < 100; ++i)
    queue3.emplace_back(std::to_string(i));

  acl::basic_queue<std::string, string_traits> queue4 = queue3;

  CHECK(queue3.empty() == false);

  for (uint32_t i = 0; i < 100; ++i)
  {
    CHECK(queue4.empty() == false);
    CHECK(queue4.pop_front() == std::to_string(i));
  }

  CHECK(queue4.empty() == true);
}

// NOLINTEND