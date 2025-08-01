// SPDX-License-Identifier: MIT

#include "ouly/allocators/memory_mapped_allocators.hpp"
#include <cstring>
#include <iostream>
#include <string>

auto main() -> int
{
  std::cout << "Testing OULY Memory-Mapped Allocators\n";
  std::cout << "=====================================\n\n";

  // Test 1: Virtual Memory Allocator
  std::cout << "1. Testing Virtual Memory Allocator:\n";
  {
    ouly::virtual_allocator alloc;

    std::cout << "   Page size: " << alloc.page_size() << " bytes\n";
    std::cout << "   Allocation granularity: " << alloc.allocation_granularity() << " bytes\n";

    // Allocate 1MB
    constexpr size_t size = 1024UL * 1024UL;
    void*            ptr  = alloc.allocate(size);

    if (ptr != nullptr)
    {
      std::cout << "   ✓ Successfully allocated " << size << " bytes\n";

      // Write some data
      constexpr unsigned char test_value = 0xAA;
      std::memset(ptr, test_value, size);

      // Verify the data
      auto*            data       = static_cast<unsigned char*>(ptr);
      bool             verified   = true;
      constexpr size_t test_count = 1000;
      for (size_t i = 0; i < test_count; ++i)
      {
        if (data[i] != test_value)
        {
          verified = false;
          break;
        }
      }

      if (verified)
      {
        std::cout << "   ✓ Memory write/read verification passed\n";
      }
      else
      {
        std::cout << "   ✗ Memory write/read verification failed\n";
      }

      alloc.deallocate(ptr, size);
      std::cout << "   ✓ Memory deallocated\n";
    }
    else
    {
      std::cout << "   ✗ Failed to allocate memory\n";
    }
  }

  std::cout << "\n";

  // Test 2: Memory-Mapped File Allocator
  std::cout << "2. Testing Memory-Mapped File Allocator:\n";
  {
    const char*      filename  = "test_mmap.bin";
    constexpr size_t file_size = 64UL * 1024UL; // 64KB

    // Create and test mmap allocator
    ouly::file_allocator alloc(filename, file_size, true);

    if (alloc.is_mapped())
    {
      std::cout << "   ✓ Successfully mapped file '" << filename << "'\n";
      std::cout << "   ✓ Mapped size: " << alloc.size() << " bytes\n";

      // Write some test data
      auto* data = static_cast<char*>(alloc.data());
      if (data != nullptr)
      {
        const char* test_string = "Hello from memory-mapped file!";
        std::strcpy(data, test_string);

        // Sync to disk
        if (alloc.sync())
        {
          std::cout << "   ✓ Data written and synced to disk\n";
        }
        else
        {
          std::cout << "   ⚠ Data written but sync failed\n";
        }
      }

      // Test the bump allocator functionality
      constexpr size_t alloc_size1 = 100;
      constexpr size_t alloc_size2 = 200;
      void*            alloc_ptr1  = alloc.allocate(alloc_size1);
      void*            alloc_ptr2  = alloc.allocate(alloc_size2);

      if (alloc_ptr1 != nullptr && alloc_ptr2 != nullptr)
      {
        std::cout << "   ✓ Bump allocator working: allocated " << alloc_size1 << " + " << alloc_size2 << " bytes\n";

        // Verify pointers are in correct order
        if (static_cast<char*>(alloc_ptr2) > static_cast<char*>(alloc_ptr1))
        {
          std::cout << "   ✓ Allocation order is correct\n";
        }

        alloc.deallocate(alloc_ptr1, alloc_size1);
        alloc.deallocate(alloc_ptr2, alloc_size2);
      }
    }
    else
    {
      std::cout << "   ✗ Failed to map file\n";
    }

    // Clean up the test file
    std::remove(filename);
    std::cout << "   ✓ Test file cleaned up\n";
  }

  std::cout << "\n";

  // Test 3: Platform Memory Utilities
  std::cout << "3. Testing Platform Memory Utilities:\n";
  {
    auto info = ouly::detail::get_memory_info();
    std::cout << "   System page size: " << info.page_size_ << " bytes\n";
    std::cout << "   Allocation granularity: " << info.allocation_granularity_ << " bytes\n";

    // Test anonymous mapping
    constexpr size_t map_size = 4096;
    void*            mapped   = ouly::detail::map_anonymous(map_size, ouly::detail::protection::read_write);

    if (mapped != nullptr)
    {
      std::cout << "   ✓ Anonymous mapping successful\n";

      // Test memory protection change
      bool protect_result = ouly::detail::virtual_protect(mapped, map_size, ouly::detail::protection::read);

      if (protect_result)
      {
        std::cout << "   ✓ Memory protection change successful\n";
      }
      else
      {
        std::cout << "   ⚠ Memory protection change failed\n";
      }

      // Test memory advice
      bool advice_result = ouly::detail::advise(mapped, map_size, ouly::detail::advice::sequential);

      if (advice_result)
      {
        std::cout << "   ✓ Memory advice successful\n";
      }
      else
      {
        std::cout << "   ⚠ Memory advice not supported or failed\n";
      }

      ouly::detail::unmap(mapped, map_size);
      std::cout << "   ✓ Anonymous mapping unmapped\n";
    }
    else
    {
      std::cout << "   ✗ Anonymous mapping failed\n";
    }
  }

  std::cout << "\nAll tests completed!\n";
  return 0;
}
