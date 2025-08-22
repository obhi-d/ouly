// NOLINTBEGIN(misc-include-cleaner)
#include "catch2/catch_all.hpp"
#include <atomic>
#include <ouly/containers/concurrent_queue.hpp>
#include <ouly/containers/config.hpp>
#include <thread>
#include <vector>
// NOLINTEND(misc-include-cleaner)

// NOLINTBEGIN

TEST_CASE("concurrent_queue basic operations",
          "[concurrent_queue]") // NOLINT(readability-function-cognitive-complexity)
{
  ouly::concurrent_queue<int> queue;

  SECTION("initial state")
  {
    REQUIRE(queue.empty());
    REQUIRE(queue.size() == 0);

    int value = 0; // NOLINT(readability-isolate-declaration)
    REQUIRE_FALSE(queue.try_dequeue(value));
  }

  SECTION("single enqueue/dequeue")
  {
    queue.enqueue(42); // NOLINT(readability-magic-numbers)
    REQUIRE_FALSE(queue.empty());
    REQUIRE(queue.size() == 1);

    int value = 0; // NOLINT(readability-isolate-declaration)
    REQUIRE(queue.try_dequeue(value));
    REQUIRE(value == 42); // NOLINT(readability-magic-numbers)
    REQUIRE(queue.empty());
    REQUIRE(queue.size() == 0);
  }

  SECTION("multiple enqueue/dequeue")
  {
    constexpr int test_count = 10;
    for (int i = 0; i < test_count; ++i)
    {
      queue.enqueue(i);
    }

    REQUIRE(queue.size() == test_count);
    REQUIRE_FALSE(queue.empty());

    for (int i = 0; i < test_count; ++i)
    {
      int value = 0; // NOLINT(readability-isolate-declaration)
      REQUIRE(queue.try_dequeue(value));
      // Note: LIFO order due to implementation
      REQUIRE(value == (test_count - 1 - i));
    }

    REQUIRE(queue.empty());
    REQUIRE(queue.size() == 0);
  }

  SECTION("emplace operation")
  {
    queue.emplace(100); // NOLINT(readability-magic-numbers)
    REQUIRE(queue.size() == 1);

    int value = 0; // NOLINT(readability-isolate-declaration)
    REQUIRE(queue.try_dequeue(value));
    REQUIRE(value == 100); // NOLINT(readability-magic-numbers)
  }
}

TEST_CASE("concurrent_queue with move-only types", "[concurrent_queue]")
{
  ouly::concurrent_queue<std::unique_ptr<int>> queue;

  SECTION("enqueue/dequeue unique_ptr")
  {
    auto ptr = std::make_unique<int>(42); // NOLINT(readability-magic-numbers)
    queue.enqueue(std::move(ptr));

    REQUIRE_FALSE(queue.empty());
    REQUIRE(queue.size() == 1);

    std::unique_ptr<int> result;
    REQUIRE(queue.try_dequeue(result));
    REQUIRE(result != nullptr);
    REQUIRE(*result == 42); // NOLINT(readability-magic-numbers)
  }
}

TEST_CASE("concurrent_queue stress test", "[concurrent_queue]") // NOLINT(readability-function-cognitive-complexity)
{
  ouly::concurrent_queue<int> queue;

  constexpr int num_producers      = 4;
  constexpr int num_consumers      = 2;
  constexpr int items_per_producer = 1000; // NOLINT(readability-magic-numbers)
  constexpr int total_items        = num_producers * items_per_producer;

  std::vector<std::thread> producers;
  std::vector<std::thread> consumers;
  std::atomic<int>         consumed_count{0};
  std::atomic<bool>        stop_consumers{false};

  // Start producers
  for (int p = 0; p < num_producers; ++p)
  {
    producers.emplace_back([&queue, p]() { // NOLINT(cppcoreguidelines-avoid-capturing-lambda-coroutines)
      for (int i = 0; i < items_per_producer; ++i) {
        queue.enqueue(p * items_per_producer + i);
      }
    });
  }

  // Start consumers
  for (int c = 0; c < num_consumers; ++c)
  {
    consumers.emplace_back([&queue, &consumed_count, &stop_consumers]() { // NOLINT(cppcoreguidelines-avoid-capturing-lambda-coroutines)
      int value = 0; // NOLINT(readability-isolate-declaration)
      while (!stop_consumers.load(std::memory_order_acquire)) {
        if (queue.try_dequeue(value)) {
          consumed_count.fetch_add(1, std::memory_order_relaxed);
        } else {
          std::this_thread::yield();
        }
      }
      // Drain remaining items
      while (queue.try_dequeue(value)) {
        consumed_count.fetch_add(1, std::memory_order_relaxed);
      }
    });
  }

  // Wait for all producers to finish
  for (auto& t : producers)
  {
    t.join();
  }

  // Give consumers time to drain
  std::this_thread::sleep_for(std::chrono::milliseconds(100)); // NOLINT(readability-magic-numbers)
  stop_consumers.store(true, std::memory_order_release);

  // Wait for all consumers to finish
  for (auto& t : consumers)
  {
    t.join();
  }

  REQUIRE(consumed_count.load() == total_items);
  REQUIRE(queue.empty());
  REQUIRE(queue.size() == 0);
}

TEST_CASE("concurrent_queue bucket overflow", "[concurrent_queue]")
{
  // Test that the queue correctly handles bucket overflow
  ouly::concurrent_queue<int> queue;

  // Fill more than one bucket (assuming default bucket size)
  constexpr int many_items = 8192; // NOLINT(readability-magic-numbers) Should exceed typical bucket size

  for (int i = 0; i < many_items; ++i)
  {
    queue.enqueue(i);
  }

  REQUIRE(queue.size() == many_items);
  REQUIRE_FALSE(queue.empty());

  // Dequeue all items
  for (int i = 0; i < many_items; ++i)
  {
    int value = 0; // NOLINT(readability-isolate-declaration)
    REQUIRE(queue.try_dequeue(value));
  }

  REQUIRE(queue.empty());
  REQUIRE(queue.size() == 0);
}

TEST_CASE("concurrent_queue fast variant mode",
          "[concurrent_queue]") // NOLINT(readability-function-cognitive-complexity)
{
  using FastConfig = ouly::config<ouly::cfg::single_threaded_consumer_for_each>;
  ouly::concurrent_queue<int, FastConfig> fast_queue;

  SECTION("basic enqueue and for_each")
  {
    // Enqueue some items
    fast_queue.enqueue(1);
    fast_queue.enqueue(2);
    fast_queue.emplace(3);

    REQUIRE(fast_queue.size() == 3);
    REQUIRE_FALSE(fast_queue.empty());

    // Test for_each
    std::vector<int> collected;
    fast_queue.for_each(
     [&collected](const int& item)
     {
       collected.push_back(item);
     });

    REQUIRE(collected.size() == 3);
    // Note: order may vary due to LIFO nature of bucket processing
    std::sort(collected.begin(), collected.end());
    REQUIRE(collected[0] == 1);
    REQUIRE(collected[1] == 2);
    REQUIRE(collected[2] == 3); // NOLINT(readability-magic-numbers)
  }

  SECTION("clear functionality")
  {
    // Enqueue many items
    constexpr int item_count = 100; // NOLINT(readability-magic-numbers)
    for (int i = 0; i < item_count; ++i)
    {
      fast_queue.enqueue(i);
    }

    REQUIRE(fast_queue.size() == item_count);
    REQUIRE_FALSE(fast_queue.empty());

    // Clear the queue
    fast_queue.clear();

    REQUIRE(fast_queue.size() == 0);
    REQUIRE(fast_queue.empty());

    // Verify for_each finds nothing
    bool found_any = false;
    fast_queue.for_each(
     [&found_any](const int&)
     {
       found_any = true;
     });
    REQUIRE_FALSE(found_any);
  }

  SECTION("concurrent enqueue with single-threaded for_each")
  {
    constexpr int num_producers      = 4;
    constexpr int items_per_producer = 250; // NOLINT(readability-magic-numbers)
    constexpr int total_items        = num_producers * items_per_producer;

    std::vector<std::thread> producers;

    // Start concurrent producers
    for (int p = 0; p < num_producers; ++p)
    {
      producers.emplace_back([&fast_queue, p]() { // NOLINT(cppcoreguidelines-avoid-capturing-lambda-coroutines)
        for (int i = 0; i < items_per_producer; ++i) {
          fast_queue.enqueue(p * items_per_producer + i);
        }
      });
    }

    // Wait for all producers to finish
    for (auto& t : producers)
    {
      t.join();
    }

    // Verify all items were enqueued
    REQUIRE(fast_queue.size() == total_items);

    // Single-threaded traversal
    std::atomic<int> traverse_count{0};
    fast_queue.for_each(
     [&traverse_count](const int&)
     {
       traverse_count.fetch_add(1, std::memory_order_relaxed);
     });

    REQUIRE(traverse_count.load() == total_items);

    // Clear and verify
    fast_queue.clear();
    REQUIRE(fast_queue.empty());
  }
}

TEST_CASE("concurrent_queue fast variant bucket overflow", "[concurrent_queue]")
{
  using FastConfig = ouly::config<ouly::cfg::single_threaded_consumer_for_each>;
  ouly::concurrent_queue<int, FastConfig> fast_queue;

  // Fill more than one bucket to test bucket allocation
  constexpr int many_items = 8192; // NOLINT(readability-magic-numbers) Should exceed typical bucket size

  for (int i = 0; i < many_items; ++i)
  {
    fast_queue.enqueue(i);
  }

  REQUIRE(fast_queue.size() == many_items);
  REQUIRE_FALSE(fast_queue.empty());

  // Count all items using for_each
  int count = 0;
  fast_queue.for_each(
   [&count](const int&)
   {
     ++count;
   });

  REQUIRE(count == many_items);

  // Clear all items
  fast_queue.clear();
  REQUIRE(fast_queue.empty());
  REQUIRE(fast_queue.size() == 0);
}

// Test that try_dequeue is not available in fast variant mode
// This test should not compile if try_dequeue is available
#if 0
TEST_CASE("concurrent_queue fast variant try_dequeue not available", "[concurrent_queue]")
{
  using FastConfig = ouly::config<ouly::cfg::single_threaded_consumer_for_each>;
  ouly::concurrent_queue<int, FastConfig> fast_queue;
  
  int value;
  // This line should cause a compilation error:
  fast_queue.try_dequeue(value);
}
#endif

// NOLINTEND
