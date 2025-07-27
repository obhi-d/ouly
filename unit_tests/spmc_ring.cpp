#include "ouly/scheduler/detail/spmc_ring.hpp"
#include "catch2/catch_all.hpp"
#include "test_common.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <mutex>
#include <numeric>
#include <set>
#include <thread>
#include <unordered_set>
#include <vector>

using namespace ouly::detail;

// NOLINTBEGIN

TEST_CASE("spmc_ring basic operations", "[spmc_ring]")
{
  SECTION("single threaded push/pop")
  {
    spmc_ring<int, 16> ring;

    SECTION("push and pop_back work correctly")
    {
      REQUIRE(ring.push_back(42));
      REQUIRE(ring.push_back(24));
      REQUIRE(ring.push_back(100));

      int value = 0;
      REQUIRE(ring.pop_back(value));
      REQUIRE(value == 100);

      REQUIRE(ring.pop_back(value));
      REQUIRE(value == 24);

      REQUIRE(ring.pop_back(value));
      REQUIRE(value == 42);

      REQUIRE_FALSE(ring.pop_back(value)); // empty
    }

    SECTION("capacity limits work correctly")
    {
      // Fill the ring to capacity
      for (int i = 0; i < 16; ++i)
      {
        REQUIRE(ring.push_back(i));
      }

      // Should be full now
      REQUIRE_FALSE(ring.push_back(16));

      // Pop one and verify we can push again
      int value = 0;
      REQUIRE(ring.pop_back(value));
      REQUIRE(ring.push_back(16));

      // Should be full again
      REQUIRE_FALSE(ring.push_back(17));
    }

    SECTION("empty ring operations")
    {
      int value = 999;
      REQUIRE_FALSE(ring.pop_back(value));
      REQUIRE(value == 999); // should be unchanged

      REQUIRE_FALSE(ring.steal(value));
      REQUIRE(value == 999); // should be unchanged
    }
  }

  SECTION("single threaded steal operations")
  {
    spmc_ring<int, 16> ring;

    SECTION("steal from populated ring")
    {
      REQUIRE(ring.push_back(1));
      REQUIRE(ring.push_back(2));
      REQUIRE(ring.push_back(3));

      int value = 0;
      REQUIRE(ring.steal(value));
      REQUIRE(value == 1); // steal from front

      REQUIRE(ring.steal(value));
      REQUIRE(value == 2);

      REQUIRE(ring.steal(value));
      REQUIRE(value == 3);

      REQUIRE_FALSE(ring.steal(value)); // empty
    }

    SECTION("mixed pop_back and steal")
    {
      REQUIRE(ring.push_back(1));
      REQUIRE(ring.push_back(2));
      REQUIRE(ring.push_back(3));
      REQUIRE(ring.push_back(4));

      int value = 0;
      REQUIRE(ring.steal(value));
      REQUIRE(value == 1); // steal from front

      REQUIRE(ring.pop_back(value));
      REQUIRE(value == 4); // pop from back

      REQUIRE(ring.steal(value));
      REQUIRE(value == 2);

      REQUIRE(ring.pop_back(value));
      REQUIRE(value == 3);

      REQUIRE_FALSE(ring.steal(value));
      REQUIRE_FALSE(ring.pop_back(value));
    }
  }
}

TEST_CASE("spmc_ring multi-threaded steal operations", "[spmc_ring][multithreaded]")
{
  SECTION("multiple consumers stealing concurrently")
  {
    constexpr int num_items     = 256; // Safe number that fits in ring
    constexpr int num_consumers = 4;

    spmc_ring<int, 512> ring;

    // Producer fills the ring completely first
    std::vector<int> items_to_produce(num_items);
    std::iota(items_to_produce.begin(), items_to_produce.end(), 0);

    // Fill the ring BEFORE starting consumers
    for (int item : items_to_produce)
    {
      while (!ring.push_back(item))
      {
        // Ring is full, wait a bit - this shouldn't happen with our size
        std::this_thread::sleep_for(std::chrono::microseconds(1));
      }
    }

    std::atomic<int>              items_stolen{0};
    std::vector<std::vector<int>> stolen_items(num_consumers);
    std::vector<std::thread>      consumers;

    // Start consumers
    for (int i = 0; i < num_consumers; ++i)
    {
      consumers.emplace_back(
       [&, consumer_id = i]()
       {
         int value = 0;
         while (items_stolen.load() < num_items)
         {
           if (ring.steal(value))
           {
             stolen_items[consumer_id].push_back(value);
             items_stolen.fetch_add(1);
           }
           else
           {
             std::this_thread::yield();
           }
         }
       });
    }

    // Wait for all consumers to finish
    for (auto& t : consumers)
    {
      t.join();
    }

    // Verify all items were stolen exactly once
    std::set<int> all_stolen;
    for (const auto& consumer_items : stolen_items)
    {
      for (int item : consumer_items)
      {
        REQUIRE(all_stolen.insert(item).second); // Should be unique
      }
    }

    REQUIRE(all_stolen.size() == num_items);
    REQUIRE(*all_stolen.begin() == 0);
    REQUIRE(*all_stolen.rbegin() == num_items - 1);

    // Ring should be empty
    int dummy = 0;
    REQUIRE_FALSE(ring.steal(dummy));
    REQUIRE_FALSE(ring.pop_back(dummy));
  }
  SECTION("producer vs single consumer race")
  {
    constexpr int       num_iterations = 10000;
    spmc_ring<int, 256> ring;

    std::atomic<bool> stop_flag{false};
    std::atomic<int>  produced{0};
    std::atomic<int>  consumed{0};

    // Producer thread
    std::thread producer(
     [&]()
     {
       for (int i = 0; i < num_iterations; ++i)
       {
         while (!ring.push_back(i))
         {
           std::this_thread::yield();
         }
         produced.fetch_add(1);
       }
       stop_flag = true;
     });

    // Consumer thread
    std::thread consumer(
     [&]()
     {
       int value = 0;
       while (!stop_flag.load() || consumed.load() < produced.load())
       {
         if (ring.steal(value))
         {
           consumed.fetch_add(1);
         }
         else
         {
           std::this_thread::yield();
         }
       }
     });

    producer.join();
    consumer.join();

    REQUIRE(produced.load() == num_iterations);
    REQUIRE(consumed.load() == num_iterations);

    // Ring should be empty
    int dummy = 0;
    REQUIRE_FALSE(ring.steal(dummy));
  }
}

TEST_CASE("spmc_ring producer pop_back vs consumer steal", "[spmc_ring][multithreaded]")
{
  SECTION("producer pop_back vs multiple stealers")
  {
    constexpr int num_items    = 5000;
    constexpr int num_stealers = 3;

    spmc_ring<int, 512> ring;

    std::atomic<int>  next_item{0};
    std::atomic<int>  items_popped{0};
    std::atomic<int>  items_stolen{0};
    std::atomic<bool> done_producing{false};

    std::vector<std::vector<int>> stolen_items(num_stealers);
    std::vector<int>              popped_items;

    // Producer thread (pushes and occasionally pops back)
    std::thread producer(
     [&]()
     {
       for (int i = 0; i < num_items; ++i)
       {
         // Push item
         while (!ring.push_back(i))
         {
           std::this_thread::yield();
         }
         next_item.fetch_add(1);

         // Occasionally pop back (simulate work-stealing owner taking work back)
         if (i % 7 == 0) // Pop every 7th item
         {
           int value = 0;
           if (ring.pop_back(value))
           {
             popped_items.push_back(value);
             items_popped.fetch_add(1);
           }
         }
       }
       done_producing = true;
     });

    // Stealer threads
    std::vector<std::thread> stealers;
    for (int i = 0; i < num_stealers; ++i)
    {
      stealers.emplace_back(
       [&, stealer_id = i]()
       {
         int value = 0;
         while (!done_producing.load() || items_stolen.load() + items_popped.load() < next_item.load())
         {
           if (ring.steal(value))
           {
             stolen_items[stealer_id].push_back(value);
             items_stolen.fetch_add(1);
           }
           else
           {
             std::this_thread::yield();
           }
         }
       });
    }

    producer.join();
    for (auto& t : stealers)
    {
      t.join();
    }

    // Verify item conservation
    std::set<int> all_processed;

    // Add popped items
    for (int item : popped_items)
    {
      REQUIRE(all_processed.insert(item).second);
    }

    // Add stolen items
    for (const auto& stealer_items : stolen_items)
    {
      for (int item : stealer_items)
      {
        REQUIRE(all_processed.insert(item).second);
      }
    }

    // Verify total count
    REQUIRE(all_processed.size() == num_items);
    REQUIRE(items_popped.load() + items_stolen.load() == num_items);

    // Ring should be empty
    int dummy = 0;
    REQUIRE_FALSE(ring.steal(dummy));
    REQUIRE_FALSE(ring.pop_back(dummy));
  }
}

TEST_CASE("spmc_ring stress test with queue size validation", "[spmc_ring][multithreaded][stress]")
{
  SECTION("simplified race condition test")
  {
    // This test is designed to expose the specific race condition bug
    constexpr int num_iterations = 1000;

    for (int test_run = 0; test_run < 100; ++test_run)
    {
      spmc_ring<int, 8> ring; // Small ring to force more contention

      std::atomic<int>  pushed{0};
      std::atomic<int>  popped{0};
      std::atomic<int>  stolen{0};
      std::atomic<bool> done{false};

      // Producer thread
      std::thread producer_popper(
       [&]()
       {
         for (int i = 0; i < num_iterations; ++i)
         {

           while (!ring.push_back(i))
           {
             std::this_thread::yield();
           }
           pushed.fetch_add(1);

           if (i % 4)
           {
             int value;
             if (ring.pop_back(value))
             {
               (void)value;
               popped.fetch_add(1);
             }
           }
         }
         done = true;
       });

      // Steal thread
      std::thread steal_thread(
       [&]()
       {
         int value;
         while (!done.load() || popped.load() + stolen.load() < pushed.load())
         {
           if (ring.steal(value))
           {
             stolen.fetch_add(1);
           }
           else
           {
             std::this_thread::yield();
           }
         }
       });

      producer_popper.join();
      steal_thread.join();

      // Count remaining items
      int remaining = 0;
      int dummy;
      while (ring.steal(dummy) || ring.pop_back(dummy))
      {
        remaining++;
      }

      int total_consumed = popped.load() + stolen.load() + remaining;

      if (pushed.load() != total_consumed)
      {
        INFO("Test run: " << test_run);
        INFO("Pushed: " << pushed.load());
        INFO("Popped: " << popped.load());
        INFO("Stolen: " << stolen.load());
        INFO("Remaining: " << remaining);
        INFO("Total consumed: " << total_consumed);
        FAIL("Conservation violation detected!");
      }
    }
  }

  SECTION("continuous push/pop/steal with size tracking")
  {
    constexpr int test_duration_ms = 1000;
    constexpr int num_stealers     = 4;

    spmc_ring<int, 256> ring;

    std::atomic<bool> stop_flag{false};
    std::atomic<int>  total_pushed{0};
    std::atomic<int>  total_popped{0};
    std::atomic<int>  total_stolen{0};

    // Producer thread
    std::thread producer(
     [&]()
     {
       int counter = 0;
       while (!stop_flag.load())
       {
         if (ring.push_back(counter))
         {
           total_pushed.fetch_add(1);
           counter++;
         }

         if (counter % 4)
         {
           int value = 0;
           if (ring.pop_back(value))
           {
             total_popped.fetch_add(1);
           }
           else
           {
             std::this_thread::yield();
           }
         }
       }
     });

    // Stealer threads
    std::vector<std::thread> stealers;
    for (int i = 0; i < num_stealers; ++i)
    {
      stealers.emplace_back(
       [&]()
       {
         int value = 0;
         while (!stop_flag.load())
         {
           if (ring.steal(value))
           {
             total_stolen.fetch_add(1);
           }
           else
           {
             std::this_thread::yield();
           }
         }
       });
    }

    // Let it run for specified duration
    std::this_thread::sleep_for(std::chrono::milliseconds(test_duration_ms));
    stop_flag = true;

    // Wait for all threads to finish
    producer.join();
    for (auto& t : stealers)
    {
      t.join();
    }

    // Verify conservation: items pushed = items popped + items stolen + items remaining
    int remaining_items = 0;
    int dummy           = 0;
    while (ring.steal(dummy) || ring.pop_back(dummy))
    {
      remaining_items++;
    }

    int total_consumed = total_popped.load() + total_stolen.load() + remaining_items;

    INFO("Pushed: " << total_pushed.load());
    INFO("Popped: " << total_popped.load());
    INFO("Stolen: " << total_stolen.load());
    INFO("Remaining: " << remaining_items);
    INFO("Total consumed: " << total_consumed);

    REQUIRE(total_pushed.load() == total_consumed);
    REQUIRE(total_pushed.load() > 0); // We should have done some work
    REQUIRE(total_stolen.load() > 0); // Stealers should have stolen something
  }
}

TEST_CASE("spmc_ring edge cases and race conditions", "[spmc_ring][multithreaded]")
{
  SECTION("last item race condition")
  {
    // Test the specific race condition when there's only one item left
    constexpr int num_iterations = 1000;

    for (int iter = 0; iter < num_iterations; ++iter)
    {
      spmc_ring<int, 4> ring;
      REQUIRE(ring.push_back(42));

      std::atomic<int>  pop_result{-1};
      std::atomic<int>  steal_result{-1};
      std::atomic<bool> pop_success{false};
      std::atomic<bool> steal_success{false};

      // Start both operations nearly simultaneously
      std::thread pop_thread(
       [&]()
       {
         int value = 0;
         if (ring.pop_back(value))
         {
           pop_result  = value;
           pop_success = true;
         }
       });

      std::thread steal_thread(
       [&]()
       {
         int value = 0;
         if (ring.steal(value))
         {
           steal_result  = value;
           steal_success = true;
         }
       });

      pop_thread.join();
      steal_thread.join();

      // Exactly one should succeed
      REQUIRE((pop_success.load() ^ steal_success.load())); // XOR - exactly one true

      if (pop_success)
      {
        REQUIRE(pop_result.load() == 42);
      }
      if (steal_success)
      {
        REQUIRE(steal_result.load() == 42);
      }

      // Ring should be empty
      int dummy = 0;
      REQUIRE_FALSE(ring.steal(dummy));
      REQUIRE_FALSE(ring.pop_back(dummy));
    }
  }

  SECTION("capacity edge cases")
  {
    spmc_ring<int, 4> ring;

    // Fill to capacity
    REQUIRE(ring.push_back(1));
    REQUIRE(ring.push_back(2));
    REQUIRE(ring.push_back(3));
    REQUIRE(ring.push_back(4));
    REQUIRE_FALSE(ring.push_back(5)); // Should fail - full

    // Steal one, should be able to push again
    int value = 0;
    REQUIRE(ring.steal(value));
    REQUIRE(value == 1);
    REQUIRE(ring.push_back(5));
    REQUIRE_FALSE(ring.push_back(6)); // Should fail - full again

    // Pop back one, should be able to push again
    REQUIRE(ring.pop_back(value));
    REQUIRE(value == 5);
    REQUIRE(ring.push_back(6));
  }
}

TEST_CASE("spmc_ring with complex data types", "[spmc_ring]")
{
  struct ComplexItem
  {
    int    id;
    double data;
    char   marker;

    bool operator==(const ComplexItem& other) const
    {
      return id == other.id && data == other.data && marker == other.marker;
    }
  };

  static_assert(std::is_trivially_copyable_v<ComplexItem>);
  static_assert(std::is_trivially_destructible_v<ComplexItem>);

  spmc_ring<ComplexItem, 16> ring;

  SECTION("push and retrieve complex items")
  {
    ComplexItem item1{1, 3.14, 'A'};
    ComplexItem item2{2, 2.71, 'B'};
    ComplexItem item3{3, 1.41, 'C'};

    REQUIRE(ring.push_back(item1));
    REQUIRE(ring.push_back(item2));
    REQUIRE(ring.push_back(item3));

    ComplexItem retrieved;

    REQUIRE(ring.steal(retrieved));
    REQUIRE(retrieved == item1);

    REQUIRE(ring.pop_back(retrieved));
    REQUIRE(retrieved == item3);

    REQUIRE(ring.steal(retrieved));
    REQUIRE(retrieved == item2);
  }
}

TEST_CASE("spmc_ring memory ordering verification", "[spmc_ring][multithreaded]")
{
  SECTION("verify no memory reordering issues")
  {
    struct IntPair
    {
      int first;
      int second;

      IntPair() = default;
      IntPair(int f, int s) : first(f), second(s) {}
    };

    static_assert(std::is_trivially_copyable_v<IntPair>);
    static_assert(std::is_trivially_destructible_v<IntPair>);

    constexpr int           num_pairs = 1000;
    spmc_ring<IntPair, 128> ring;

    std::atomic<bool>    start_flag{false};
    std::vector<IntPair> stolen_pairs;
    std::mutex           stolen_mutex;

    // Producer thread - pushes pairs where second = first + 1
    std::thread producer(
     [&]()
     {
       start_flag = true;
       for (int i = 0; i < num_pairs; ++i)
       {
         IntPair pair{i, i + 1};
         while (!ring.push_back(pair))
         {
           std::this_thread::yield();
         }
       }
     });

    // Consumer thread
    std::thread consumer(
     [&]()
     {
       while (!start_flag.load())
       {
         std::this_thread::yield();
       }

       IntPair pair;
       int     consumed = 0;
       while (consumed < num_pairs)
       {
         if (ring.steal(pair))
         {
           std::lock_guard<std::mutex> lock(stolen_mutex);
           stolen_pairs.push_back(pair);
           consumed++;
         }
         else
         {
           std::this_thread::yield();
         }
       }
     });

    producer.join();
    consumer.join();

    // Verify all pairs maintain the invariant
    REQUIRE(stolen_pairs.size() == num_pairs);
    for (const auto& pair : stolen_pairs)
    {
      REQUIRE(pair.second == pair.first + 1);
    }
  }
}

// NOLINTEND
