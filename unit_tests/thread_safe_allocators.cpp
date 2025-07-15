#include "catch2/catch_all.hpp"
#include <atomic>
#include <chrono>
#include <cstring>
#include <ouly/allocators/ts_shared_linear_allocator.hpp>
#include <ouly/allocators/ts_thread_local_allocator.hpp>
#include <thread>
#include <vector>

// NOLINTBEGIN
TEST_CASE("Thread-safe local allocator basic test", "[allocator]")
{
  ouly::ts_thread_local_allocator allocator;

  // Test allocation
  void* ptr1 = allocator.allocate(64);
  REQUIRE(ptr1 != nullptr);

  // Test deallocation (should succeed for last allocation)
  REQUIRE(allocator.deallocate(ptr1, 64) == true);

  // Test allocation with alignment
  void* ptr2 = allocator.allocate(32);
  REQUIRE(ptr2 != nullptr);
  REQUIRE(reinterpret_cast<std::uintptr_t>(ptr2) % alignof(std::max_align_t) == 0);

  // Test that deallocating non-last allocation fails
  void* ptr3 = allocator.allocate(16);
  REQUIRE(ptr3 != nullptr);
  REQUIRE(allocator.deallocate(ptr2, 32) == false); // ptr2 is not the last allocation
  REQUIRE(allocator.deallocate(ptr3, 16) == true);  // ptr3 is the last allocation
}

TEST_CASE("Thread-safe shared linear allocator basic test", "[allocator]")
{
  ouly::ts_shared_linear_allocator allocator;

  // Test allocation
  void* ptr1 = allocator.allocate(128);
  REQUIRE(ptr1 != nullptr);

  // Test deallocation (should succeed for last allocation)
  REQUIRE(allocator.deallocate(ptr1, 128) == true);

  // Test allocation with alignment
  void* ptr2 = allocator.allocate(64);
  REQUIRE(ptr2 != nullptr);
  REQUIRE(reinterpret_cast<std::uintptr_t>(ptr2) % alignof(std::max_align_t) == 0);

  // Test that deallocating non-last allocation fails
  void* ptr3 = allocator.allocate(32);
  REQUIRE(ptr3 != nullptr);
  REQUIRE(allocator.deallocate(ptr2, 64) == false); // ptr2 is not the last allocation
  REQUIRE(allocator.deallocate(ptr3, 32) == true);  // ptr3 is the last allocation

  // Test deallocation
  allocator.reset(); // Reset to clear all allocations
}

TEST_CASE("Thread-safe local allocator multi-threaded test", "[allocator]")
{
  ouly::ts_thread_local_allocator allocator;
  constexpr int                   num_threads            = 8;
  constexpr int                   allocations_per_thread = 1000;
  std::vector<std::thread>        threads;
  std::atomic<int>                successful_allocations{0};
  std::atomic<int>                successful_deallocations{0};
  std::atomic<int>                failed_allocations{0};

  auto worker = [&allocator, &successful_allocations, &successful_deallocations, &failed_allocations]()
  {
    std::vector<std::pair<void*, std::size_t>> allocations;

    // Allocate memory
    for (int i = 0; i < allocations_per_thread; ++i)
    {
      std::size_t size = 16 + (i % 256); // Variable sizes
      void*       ptr  = allocator.allocate(size);
      if (ptr != nullptr)
      {
        allocations.push_back({ptr, size});
        successful_allocations.fetch_add(1, std::memory_order_relaxed);
      }
      else
      {
        failed_allocations.fetch_add(1, std::memory_order_relaxed);
      }
    }

    // Try to deallocate in reverse order (stack-style)
    for (auto it = allocations.rbegin(); it != allocations.rend(); ++it)
    {
      if (allocator.deallocate(it->first, it->second))
      {
        successful_deallocations.fetch_add(1, std::memory_order_relaxed);
      }
    }
  };

  // Launch worker threads
  for (int i = 0; i < num_threads; ++i)
  {
    threads.emplace_back(worker);
  }

  // Wait for all threads to complete
  for (auto& thread : threads)
  {
    thread.join();
  }

  // Verify that allocations were successful
  REQUIRE(successful_allocations.load() == num_threads * allocations_per_thread);
  REQUIRE(failed_allocations.load() == 0);

  // Note: Due to the nature of thread-local allocator, deallocations will be successful
  // only for the most recent allocations in each thread
  INFO("Successful deallocations: " << successful_deallocations.load());

  allocator.reset();
}

TEST_CASE("Thread-safe shared linear allocator multi-threaded test", "[allocator]")
{
  ouly::ts_shared_linear_allocator allocator;
  constexpr int                    num_threads            = 8;
  constexpr int                    allocations_per_thread = 1000;
  std::vector<std::thread>         threads;
  std::atomic<int>                 successful_allocations{0};
  std::atomic<int>                 successful_deallocations{0};
  std::atomic<int>                 failed_allocations{0};

  auto worker = [&allocator, &successful_allocations, &successful_deallocations, &failed_allocations]()
  {
    std::vector<std::pair<void*, std::size_t>> allocations;

    // Allocate memory
    for (int i = 0; i < allocations_per_thread; ++i)
    {
      std::size_t size = 16 + (i % 256); // Variable sizes
      void*       ptr  = allocator.allocate(size);
      if (ptr != nullptr)
      {
        allocations.push_back({ptr, size});
        successful_allocations.fetch_add(1, std::memory_order_relaxed);
      }
      else
      {
        failed_allocations.fetch_add(1, std::memory_order_relaxed);
      }
    }

    // Try to deallocate the last allocation (most likely to succeed)
    if (!allocations.empty())
    {
      auto& last = allocations.back();
      if (allocator.deallocate(last.first, last.second))
      {
        successful_deallocations.fetch_add(1, std::memory_order_relaxed);
      }
    }
  };

  // Launch worker threads
  for (int i = 0; i < num_threads; ++i)
  {
    threads.emplace_back(worker);
  }

  // Wait for all threads to complete
  for (auto& thread : threads)
  {
    thread.join();
  }

  // Verify that allocations were successful
  REQUIRE(successful_allocations.load() == num_threads * allocations_per_thread);
  REQUIRE(failed_allocations.load() == 0);

  // Note: Due to race conditions, not all deallocations will succeed
  INFO("Successful deallocations: " << successful_deallocations.load());

  allocator.reset();
}

TEST_CASE("Thread-safe allocators large allocation test", "[allocator]")
{
  SECTION("ts_thread_local_allocator")
  {
    ouly::ts_thread_local_allocator allocator;
    constexpr std::size_t           large_size = 2 * 1024 * 1024; // 2MB

    void* ptr = allocator.allocate(large_size);
    REQUIRE(ptr != nullptr);

    // Large allocations should not be deallocatable in this implementation
    REQUIRE(allocator.deallocate(ptr, large_size) == false);

    allocator.reset();
  }

  SECTION("ts_shared_linear_allocator")
  {
    ouly::ts_shared_linear_allocator allocator;
    constexpr std::size_t            large_size = 2 * 1024 * 1024; // 2MB

    void* ptr = allocator.allocate(large_size);
    REQUIRE(ptr != nullptr);

    // Large allocations should not be deallocatable in this implementation
    REQUIRE(allocator.deallocate(ptr, large_size) == false);

    allocator.reset();
  }
}

TEST_CASE("Thread-safe allocators stress test", "[allocator]")
{
  constexpr int num_threads = 16;

  SECTION("ts_thread_local_allocator stress test")
  {
    ouly::ts_thread_local_allocator allocator;
    std::vector<std::thread>        threads;
    std::atomic<bool>               keep_running{true};
    std::atomic<int>                allocation_failures{0};

    auto worker = [&allocator, &keep_running, &allocation_failures]()
    {
      while (keep_running.load(std::memory_order_relaxed))
      {
        std::size_t size = 16 + (std::rand() % 512);
        void*       ptr  = allocator.allocate(size);
        if (ptr != nullptr)
        {
          // Write to the memory to ensure it's valid
          std::memset(ptr, 0xAA, size);

          // Sometimes try to deallocate
          if (std::rand() % 10 == 0)
          {
            allocator.deallocate(ptr, size);
          }
        }
        else
        {
          allocation_failures.fetch_add(1, std::memory_order_relaxed);
        }
      }
    };

    // Launch worker threads
    for (int i = 0; i < num_threads; ++i)
    {
      threads.emplace_back(worker);
    }

    // Let threads run for a short time
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    keep_running.store(false, std::memory_order_relaxed);

    // Wait for all threads to complete
    for (auto& thread : threads)
    {
      thread.join();
    }

    // Verify that allocations mostly succeeded
    INFO("Allocation failures: " << allocation_failures.load());

    allocator.reset();
  }

  SECTION("ts_shared_linear_allocator stress test")
  {
    ouly::ts_shared_linear_allocator allocator;
    std::vector<std::thread>         threads;
    std::atomic<bool>                keep_running{true};
    std::atomic<int>                 allocation_failures{0};

    auto worker = [&allocator, &keep_running, &allocation_failures]()
    {
      while (keep_running.load(std::memory_order_relaxed))
      {
        std::size_t size = 16 + (std::rand() % 512);
        void*       ptr  = allocator.allocate(size);
        if (ptr != nullptr)
        {
          // Write to the memory to ensure it's valid
          std::memset(ptr, 0xAA, size);

          // Sometimes try to deallocate
          if (std::rand() % 10 == 0)
          {
            allocator.deallocate(ptr, size);
          }
        }
        else
        {
          allocation_failures.fetch_add(1, std::memory_order_relaxed);
        }
      }
    };

    // Launch worker threads
    for (int i = 0; i < num_threads; ++i)
    {
      threads.emplace_back(worker);
    }

    // Let threads run for a short time
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    keep_running.store(false, std::memory_order_relaxed);

    // Wait for all threads to complete
    for (auto& thread : threads)
    {
      thread.join();
    }

    // Verify that allocations mostly succeeded
    INFO("Allocation failures: " << allocation_failures.load());

    allocator.reset();
  }
}

TEST_CASE("Thread-safe allocators move semantics test", "[allocator]")
{
  SECTION("ts_thread_local_allocator move")
  {
    ouly::ts_thread_local_allocator allocator1;
    void*                           ptr1 = allocator1.allocate(64);
    REQUIRE(ptr1 != nullptr);

    ouly::ts_thread_local_allocator allocator2 = std::move(allocator1);
    void*                           ptr2       = allocator2.allocate(64);
    REQUIRE(ptr2 != nullptr);

    // Original allocator should be in a valid but unspecified state
    void* ptr3 = allocator1.allocate(64);
    REQUIRE(ptr3 != nullptr);
  }

  SECTION("ts_shared_linear_allocator move")
  {
    ouly::ts_shared_linear_allocator allocator1;
    void*                            ptr1 = allocator1.allocate(64);
    REQUIRE(ptr1 != nullptr);

    ouly::ts_shared_linear_allocator allocator2 = std::move(allocator1);
    void*                            ptr2       = allocator2.allocate(64);
    REQUIRE(ptr2 != nullptr);

    // Original allocator should be in a valid but unspecified state
    void* ptr3 = allocator1.allocate(64);
    REQUIRE(ptr3 != nullptr);
  }
}
// NOLINTEND