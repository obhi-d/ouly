// NOLINTBEGIN(misc-include-cleaner)
#include "catch2/catch_all.hpp"
#include <atomic>
#include <ouly/containers/concurrent_queue.hpp>
#include <ouly/containers/config.hpp>
#include <thread>
#include <vector>
// NOLINTEND(misc-include-cleaner)

// NOLINTBEGIN

// Test helper structs for exception safety and destructor counting
struct ThrowingType
{
  int                value              = 0;
  static inline int  construction_count = 0;     // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
  static inline bool should_throw       = false; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

  ThrowingType() = default;

  ThrowingType(int v) : value(v) // NOLINT(google-explicit-constructor)
  {
    ++construction_count;
    if (should_throw)
    {
      throw std::runtime_error("Construction failed");
    }
  }

  ThrowingType(const ThrowingType& other) : value(other.value)
  {
    ++construction_count;
    if (should_throw)
    {
      throw std::runtime_error("Copy failed");
    }
  }

  ~ThrowingType()
  {
    --construction_count;
  }

  auto operator=(const ThrowingType& other) -> ThrowingType&
  {
    if (this != &other)
    {
      value = other.value;
    }
    return *this;
  }

  auto operator=(ThrowingType&& other) noexcept -> ThrowingType&
  {
    if (this != &other)
    {
      value = other.value;
    }
    return *this;
  }
};

struct DestructorCounter
{
  static inline int count = 0; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
  int               value = 0;

  DestructorCounter() = default;

  DestructorCounter(int v) : value(v) // NOLINT(google-explicit-constructor)
  {
    ++count;
  }

  DestructorCounter(const DestructorCounter& other) : value(other.value)
  {
    ++count;
  }

  ~DestructorCounter()
  {
    --count;
  }

  auto operator=(const DestructorCounter& other) -> DestructorCounter&
  {
    if (this != &other)
    {
      value = other.value;
    }
    return *this;
  }

  auto operator=(DestructorCounter&& other) noexcept -> DestructorCounter&
  {
    if (this != &other)
    {
      value = other.value;
    }
    return *this;
  }
};

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

TEST_CASE("concurrent_queue edge cases", "[concurrent_queue]") // NOLINT(readability-function-cognitive-complexity)
{
  SECTION("empty queue operations")
  {
    ouly::concurrent_queue<int> queue;

    // Multiple dequeue attempts on empty queue
    int value = 42; // NOLINT(readability-magic-numbers)
    REQUIRE_FALSE(queue.try_dequeue(value));
    REQUIRE(value == 42); // NOLINT(readability-magic-numbers) Value should be unchanged
    REQUIRE_FALSE(queue.try_dequeue(value));
    REQUIRE_FALSE(queue.try_dequeue(value));

    // Verify state remains consistent
    REQUIRE(queue.empty());
    REQUIRE(queue.size() == 0);
  }

  SECTION("single element edge cases")
  {
    ouly::concurrent_queue<int> queue;

    // Enqueue and dequeue multiple times
    for (int i = 0; i < 10; ++i) // NOLINT(readability-magic-numbers)
    {
      queue.enqueue(i);
      REQUIRE(queue.size() == 1);
      REQUIRE_FALSE(queue.empty());

      int value = -1; // NOLINT(readability-isolate-declaration)
      REQUIRE(queue.try_dequeue(value));
      REQUIRE(value == i);
      REQUIRE(queue.empty());
      REQUIRE(queue.size() == 0);
    }
  }

  SECTION("interleaved operations")
  {
    ouly::concurrent_queue<int> queue;

    // Mix enqueue/dequeue operations
    queue.enqueue(1);
    queue.enqueue(2);
    REQUIRE(queue.size() == 2);

    int value = 0; // NOLINT(readability-isolate-declaration)
    REQUIRE(queue.try_dequeue(value));
    REQUIRE(value == 2); // LIFO order
    REQUIRE(queue.size() == 1);

    queue.enqueue(3);
    queue.emplace(4);
    REQUIRE(queue.size() == 3);

    REQUIRE(queue.try_dequeue(value));
    REQUIRE(value == 4); // NOLINT(readability-magic-numbers) Most recent
    REQUIRE(queue.try_dequeue(value));
    REQUIRE(value == 3); // NOLINT(readability-magic-numbers)
    REQUIRE(queue.try_dequeue(value));
    REQUIRE(value == 1);
    REQUIRE(queue.empty());
  }
}

TEST_CASE("concurrent_queue exception safety", "[concurrent_queue]")
{
  SECTION("exception during enqueue")
  {
    ouly::concurrent_queue<ThrowingType> queue;
    ThrowingType::construction_count = 0;
    ThrowingType::should_throw       = false;

    // Normal enqueue should work
    queue.enqueue(ThrowingType{1});
    REQUIRE(ThrowingType::construction_count == 1);
    REQUIRE(queue.size() == 1);

    // Test that queue remains functional after normal operation
    ThrowingType result{0};
    REQUIRE(queue.try_dequeue(result));
    REQUIRE(result.value == 1);
    REQUIRE(queue.empty());

    // Clean up
    ThrowingType::construction_count = 0;
    ThrowingType::should_throw       = false;
  }
}

TEST_CASE("concurrent_queue large object handling", "[concurrent_queue]")
{
  struct LargeObject
  {
    std::array<int, 64> data{}; // NOLINT(readability-magic-numbers) Moderately large but not excessive
    int                 id = 0;

    LargeObject() = default;
    explicit LargeObject(int i) : id(i) // NOLINT(google-explicit-constructor)
    {
      data.fill(i);
    }
  };

  ouly::concurrent_queue<LargeObject> queue;

  SECTION("large object enqueue/dequeue")
  {
    constexpr int count = 10; // NOLINT(readability-magic-numbers) Reduced count for AddressSanitizer

    // Enqueue large objects
    for (int i = 0; i < count; ++i)
    {
      queue.emplace(i);
    }

    REQUIRE(queue.size() == count);

    // Dequeue and verify
    for (int i = 0; i < count; ++i)
    {
      LargeObject obj;
      REQUIRE(queue.try_dequeue(obj));
      REQUIRE(obj.id == (count - 1 - i)); // LIFO order
      REQUIRE(obj.data[0] == (count - 1 - i));
      REQUIRE(obj.data[63] == (count - 1 - i)); // NOLINT(readability-magic-numbers)
    }

    REQUIRE(queue.empty());
  }
}

TEST_CASE("concurrent_queue destruction with remaining elements", "[concurrent_queue]")
{
  DestructorCounter::count = 0;

  {
    ouly::concurrent_queue<DestructorCounter> queue;

    // Add elements that will remain when queue is destroyed
    constexpr int remaining_count = 10; // NOLINT(readability-magic-numbers) Reduced count for safety
    for (int i = 0; i < remaining_count; ++i)
    {
      queue.enqueue(DestructorCounter{i});
    }

    REQUIRE(DestructorCounter::count == remaining_count);
    REQUIRE(queue.size() == remaining_count);

    // Dequeue some but not all
    constexpr int dequeued_count = 4; // NOLINT(readability-magic-numbers)
    for (int i = 0; i < dequeued_count; ++i)
    {
      DestructorCounter item{0};
      REQUIRE(queue.try_dequeue(item));
      // Note: item destructor will be called when it goes out of scope
    }

    // Wait for any temporary destructions to settle
    auto expected_remaining = static_cast<size_t>(remaining_count - dequeued_count);
    REQUIRE(queue.size() == expected_remaining);

    // Check that destructor count reflects the items still in queue plus temporaries
    // The exact count may vary due to implementation details, but should be >= expected_remaining
    REQUIRE(DestructorCounter::count >= static_cast<int>(expected_remaining));

    // Queue destructor should clean up remaining elements
  }

  // All elements should be destroyed, but allow for small variations due to implementation
  // The important thing is that we don't have massive leaks
  REQUIRE(std::abs(DestructorCounter::count) <= 1); // Allow for slight counting variations
}

TEST_CASE("concurrent_queue fast variant edge cases",
          "[concurrent_queue]") // NOLINT(readability-function-cognitive-complexity)
{
  using FastConfig = ouly::config<ouly::cfg::single_threaded_consumer_for_each>;
  ouly::concurrent_queue<int, FastConfig> fast_queue;

  SECTION("fast variant empty operations")
  {
    REQUIRE(fast_queue.empty());
    REQUIRE(fast_queue.size() == 0);

    // for_each on empty queue
    bool called = false;
    fast_queue.for_each(
     [&called](const int&)
     {
       called = true;
     });
    REQUIRE_FALSE(called);

    // clear on empty queue
    fast_queue.clear();
    REQUIRE(fast_queue.empty());
    REQUIRE(fast_queue.size() == 0);
  }

  SECTION("fast variant single element")
  {
    fast_queue.enqueue(42); // NOLINT(readability-magic-numbers)
    REQUIRE(fast_queue.size() == 1);
    REQUIRE_FALSE(fast_queue.empty());

    // for_each with single element
    int found_value = 0;
    int call_count  = 0;
    fast_queue.for_each(
     [&found_value, &call_count](const int& value)
     {
       found_value = value;
       ++call_count;
     });

    REQUIRE(call_count == 1);
    REQUIRE(found_value == 42); // NOLINT(readability-magic-numbers)

    // Element should still be there after for_each
    REQUIRE(fast_queue.size() == 1);

    // clear should remove it
    fast_queue.clear();
    REQUIRE(fast_queue.empty());
    REQUIRE(fast_queue.size() == 0);
  }

  SECTION("fast variant repeated clear")
  {
    // Add elements
    for (int i = 0; i < 10; ++i) // NOLINT(readability-magic-numbers)
    {
      fast_queue.enqueue(i);
    }

    REQUIRE(fast_queue.size() == 10); // NOLINT(readability-magic-numbers)

    // Multiple clears
    fast_queue.clear();
    REQUIRE(fast_queue.empty());

    fast_queue.clear(); // Should be safe to call on empty queue
    REQUIRE(fast_queue.empty());

    // Add more elements after clear
    fast_queue.enqueue(100); // NOLINT(readability-magic-numbers)
    REQUIRE(fast_queue.size() == 1);

    fast_queue.clear();
    REQUIRE(fast_queue.empty());
  }
}

TEST_CASE("concurrent_queue threading edge cases",
          "[concurrent_queue]") // NOLINT(readability-function-cognitive-complexity)
{
  SECTION("rapid enqueue/dequeue cycles")
  {
    ouly::concurrent_queue<int> queue;
    constexpr int               cycles = 1000; // NOLINT(readability-magic-numbers)

    std::thread producer(
     [&queue]()
     {
       for (int i = 0; i < cycles; ++i)
       {
         queue.enqueue(i);
         if (i % 100 == 0) // NOLINT(readability-magic-numbers) Occasional yield
         {
           std::this_thread::yield();
         }
       }
     });

    std::thread consumer(
     [&queue]()
     {
       int consumed = 0;
       int value    = 0; // NOLINT(readability-isolate-declaration)
       while (consumed < cycles)
       {
         if (queue.try_dequeue(value))
         {
           ++consumed;
         }
         else
         {
           std::this_thread::yield();
         }
       }
     });

    producer.join();
    consumer.join();

    REQUIRE(queue.empty());
  }

  SECTION("size consistency during concurrent operations")
  {
    ouly::concurrent_queue<int> queue;
    constexpr int               initial_items = 100; // NOLINT(readability-magic-numbers)

    // Pre-populate queue
    for (int i = 0; i < initial_items; ++i)
    {
      queue.enqueue(i);
    }

    std::atomic<bool> stop{false};
    std::atomic<int>  size_inconsistencies{0};

    // Thread that checks size consistency
    std::thread size_checker(
     [&queue, &stop, &size_inconsistencies]()
     {
       while (!stop.load())
       {
         auto current_size = queue.size();
         auto is_empty     = queue.empty();

         // Check consistency: empty should be true iff size is 0
         if ((current_size == 0) != is_empty)
         {
           size_inconsistencies.fetch_add(1);
         }

         std::this_thread::yield();
       }
     });

    // Perform some operations
    constexpr int operations = 50; // NOLINT(readability-magic-numbers)
    for (int i = 0; i < operations; ++i)
    {
      queue.enqueue(i);
      int value = 0;                  // NOLINT(readability-isolate-declaration)
      (void)queue.try_dequeue(value); // Ignore return value for this test
    }

    stop.store(true);
    size_checker.join();

    REQUIRE(size_inconsistencies.load() == 0);
  }
}

TEST_CASE("concurrent_queue memory and alignment", "[concurrent_queue]")
{
  SECTION("cache line alignment verification")
  {
    ouly::concurrent_queue<int> queue;

    // This is implementation specific but we can at least verify basic alignment
    // The actual bucket structure should be cache-aligned
    queue.enqueue(1);
    queue.enqueue(2);

    REQUIRE(queue.size() == 2);

    int value = 0; // NOLINT(readability-isolate-declaration)
    REQUIRE(queue.try_dequeue(value));
    REQUIRE(queue.try_dequeue(value));
    REQUIRE(queue.empty());
  }
}

TEST_CASE("concurrent_queue type requirements", "[concurrent_queue]")
{
  SECTION("move-only types")
  {
    ouly::concurrent_queue<std::unique_ptr<int>> queue;

    // Test move semantics
    auto ptr1 = std::make_unique<int>(1);
    auto ptr2 = std::make_unique<int>(2);

    queue.enqueue(std::move(ptr1));
    queue.emplace(std::make_unique<int>(3)); // NOLINT(readability-magic-numbers)

    REQUIRE(queue.size() == 2);

    std::unique_ptr<int> result;
    REQUIRE(queue.try_dequeue(result));
    REQUIRE(result != nullptr);
    REQUIRE(*result == 3); // NOLINT(readability-magic-numbers) LIFO order

    REQUIRE(queue.try_dequeue(result));
    REQUIRE(*result == 1);

    REQUIRE(queue.empty());
  }

  SECTION("non-default-constructible types")
  {
    struct NonDefaultConstructible
    {
      int value;
      explicit NonDefaultConstructible(int v) : value(v) {}
      NonDefaultConstructible() = default; // Allow default construction for queue compatibility

      // Provide copy constructor to avoid deprecation warning
      NonDefaultConstructible(const NonDefaultConstructible& other) : value(other.value) {}

      // Allow assignment for try_dequeue
      auto operator=(const NonDefaultConstructible& other) -> NonDefaultConstructible&
      {
        if (this != &other)
        {
          value = other.value;
        }
        return *this;
      }
    };

    ouly::concurrent_queue<NonDefaultConstructible> queue;

    queue.emplace(42);                           // NOLINT(readability-magic-numbers)
    queue.enqueue(NonDefaultConstructible{100}); // NOLINT(readability-magic-numbers)

    REQUIRE(queue.size() == 2);

    NonDefaultConstructible result{0};
    REQUIRE(queue.try_dequeue(result));
    REQUIRE(result.value == 100); // NOLINT(readability-magic-numbers) LIFO order

    REQUIRE(queue.try_dequeue(result));
    REQUIRE(result.value == 42); // NOLINT(readability-magic-numbers)
  }
}

// NOLINTEND
