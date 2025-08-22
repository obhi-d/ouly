#include "ouly/containers/basic_queue.hpp"
#include "catch2/catch_all.hpp"
#include "ouly/utility/config.hpp"
#include <string>

// NOLINTBEGIN

struct string_traits
{
  static constexpr uint32_t pool_size_v = 4;
};

TEST_CASE("Check basic_queue empty", "[basic_queue]")
{
  ouly::basic_queue<std::string, string_traits> queue;
  bool                                          exception = false;
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
  ouly::basic_queue<std::string, string_traits> queue;

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

TEST_CASE("Check for leaks in basic_queue", "[basic_queue]")
{
  int object_count = 0;
  struct leak_track
  {
    int& object_counter;

    leak_track(int& counter) : object_counter(counter)
    {
      ++object_counter;
    }
    ~leak_track()
    {
      --object_counter;
    }
    leak_track(leak_track const& other) : object_counter(other.object_counter)
    {
      ++object_counter;
    }
    leak_track(leak_track&& other) : object_counter(other.object_counter)
    {
      ++object_counter;
    }
    leak_track& operator=(leak_track const& other)
    {
      if (this == &other)
        return *this;
      object_counter = other.object_counter;
      return *this;
    }
    leak_track& operator=(leak_track&& other)
    {
      if (this == &other)
        return *this;
      object_counter = other.object_counter;
      return *this;
    }
  };

  ouly::basic_queue<leak_track, ouly::config<ouly::cfg::pool_size<4>>> queue;

  for (uint32_t i = 0; i < 10; ++i)
    queue.emplace_back(object_count);

  CHECK(object_count == 10);
  queue.pop_front();
  CHECK(object_count == 9);

  queue.clear();
  CHECK(queue.empty() == true);
  CHECK(object_count == 0);
}

TEST_CASE("Validate basic_queue initialization", "[basic_queue]")
{
  ouly::basic_queue<std::string, string_traits> queue1;
  ouly::basic_queue<std::string, string_traits> queue2;

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

  ouly::basic_queue<std::string, string_traits> queue3 = std::move(queue1);

  CHECK(queue1.empty() == true);

  for (uint32_t i = 0; i < 100; ++i)
  {
    CHECK(queue3.empty() == false);
    CHECK(queue3.pop_front() == std::to_string(i));
  }

  CHECK(queue3.empty() == true);

  for (uint32_t i = 0; i < 100; ++i)
    queue3.emplace_back(std::to_string(i));

  ouly::basic_queue<std::string, string_traits> queue4 = queue3;

  CHECK(queue3.empty() == false);

  for (uint32_t i = 0; i < 100; ++i)
  {
    CHECK(queue4.empty() == false);
    CHECK(queue4.pop_front() == std::to_string(i));
  }

  CHECK(queue4.empty() == true);
}

TEST_CASE("basic_queue: Test swap functionality", "[basic_queue][swap]")
{
  ouly::basic_queue<std::string, string_traits> queue1;
  queue1.emplace_back("first");
  queue1.emplace_back("second");

  ouly::basic_queue<std::string, string_traits> queue2;
  queue2.emplace_back("third");
  queue2.emplace_back("fourth");
  queue2.emplace_back("fifth");

  // Store original sizes
  std::size_t size1 = queue1.size();
  std::size_t size2 = queue2.size();

  // Test member swap
  queue1.swap(queue2);

  // Verify sizes are swapped
  REQUIRE(queue1.size() == size2);
  REQUIRE(queue2.size() == size1);

  // Verify contents are swapped
  REQUIRE(queue1.pop_front() == "third");
  REQUIRE(queue1.pop_front() == "fourth");
  REQUIRE(queue1.pop_front() == "fifth");
  REQUIRE(queue1.empty() == true);

  REQUIRE(queue2.pop_front() == "first");
  REQUIRE(queue2.pop_front() == "second");
  REQUIRE(queue2.empty() == true);

  // Setup for friend swap test
  queue1.emplace_back("a");
  queue1.emplace_back("b");
  queue2.emplace_back("x");
  queue2.emplace_back("y");
  queue2.emplace_back("z");

  // Test friend swap function
  swap(queue1, queue2);

  // Verify contents are swapped back
  REQUIRE(queue1.size() == 3);
  REQUIRE(queue2.size() == 2);

  REQUIRE(queue1.pop_front() == "x");
  REQUIRE(queue1.pop_front() == "y");
  REQUIRE(queue1.pop_front() == "z");
  REQUIRE(queue1.empty() == true);

  REQUIRE(queue2.pop_front() == "a");
  REQUIRE(queue2.pop_front() == "b");
  REQUIRE(queue2.empty() == true);
}

// NOLINTEND