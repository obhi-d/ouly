#include "ouly/allocators/memory_mapped_allocators.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>

void test_virtual_memory_allocator()
{
  std::cout << "1. Testing Virtual Memory Allocator:\n";
  constexpr size_t k_page_size = 4096;

  try
  {
    ouly::virtual_allocator vm_alloc;

    // Allocate some memory
    auto* ptr = vm_alloc.allocate(k_page_size);
    if (ptr != ouly::virtual_allocator::null())
    {
      std::cout << "   ✓ Successfully allocated 4KB of virtual memory\n";

      // Write some data
      auto*       data      = static_cast<char*>(ptr);
      std::string test_data = "Hello from virtual memory!";
      std::ranges::copy(test_data, data);
      data[test_data.size()] = '\0';

      std::cout << "   ✓ Written data: " << data << "\n";

      // Deallocate
      vm_alloc.deallocate(ptr, k_page_size);
      std::cout << "   ✓ Successfully deallocated memory\n";
    }
    else
    {
      std::cout << "   ✗ Failed to allocate virtual memory\n";
    }
  }
  catch (const std::exception& e)
  {
    std::cout << "   ✗ Exception: " << e.what() << "\n";
  }
}

void test_mmap_write_operations(const std::string& filename)
{
  std::cout << "2. Testing mio-style Memory-Mapped File:\n";
  std::error_code error;
  ouly::mmap_sink sink;

  sink.map(filename, error);
  if (!error)
  {
    std::cout << "   ✓ Successfully mapped file: " << filename << " (" << sink.size() << " bytes)\n";
    std::cout << "   ✓ Container interface works: empty=" << sink.empty() << ", size=" << sink.size() << "\n";

    // Test container-like operations
    if (!sink.empty())
    {
      // Fill with pattern using iterators
      constexpr unsigned char fill_pattern = 0x42;
      std::ranges::fill(sink, fill_pattern);
      std::cout << "   ✓ Filled file with pattern using iterators\n";

      // Write a message at the beginning
      std::string message = "Hello from mio-style mmap!";
      auto*       dest    = sink.begin();
      for (char c : message)
      {
        *dest++ = static_cast<unsigned char>(c);
      }

      // Access via array operator
      sink[message.size()] = 0;

      std::cout << "   ✓ Written message: ";
      for (size_t i = 0; i < message.size(); ++i)
      {
        std::cout << static_cast<char>(sink[i]);
      }
      std::cout << "\n";

      // Test reverse iterators
      auto last_byte = *sink.rbegin();
      std::cout << "   ✓ Last byte via reverse iterator: 0x" << std::hex << static_cast<int>(last_byte) << std::dec
                << "\n";

      // Sync to disk
      sink.sync(error);
      if (!error)
      {
        std::cout << "   ✓ Synchronized data to disk\n";
      }
      else
      {
        std::cout << "   ✗ Failed to sync: " << error.message() << "\n";
      }
    }
  }
  else
  {
    std::cout << "   ✗ Failed to map file: " << error.message() << "\n";
  }
}

void test_mmap_read_operations(const std::string& filename)
{
  std::cout << "\n3. Testing read-only mapping:\n";
  std::error_code   error;
  ouly::mmap_source source;

  source.map(filename, error);
  if (!error)
  {
    std::cout << "   ✓ Successfully opened read-only mapping (" << source.size() << " bytes)\n";

    // Read the message we wrote earlier
    if (!source.empty())
    {
      std::cout << "   ✓ Read message: ";
      size_t      msg_length = 0;
      const auto* data_ptr   = source.data();
      // Find message length (until null terminator)
      while (msg_length < source.size() && data_ptr[msg_length] != 0)
      {
        ++msg_length;
      }
      for (size_t i = 0; i < msg_length; ++i)
      {
        std::cout << static_cast<char>(data_ptr[i]);
      }
      std::cout << "\n";

      // Test const iterators
      auto first_byte = *source.cbegin();
      std::cout << "   ✓ First byte: '" << static_cast<char>(first_byte) << "'\n";
    }
  }
  else
  {
    std::cout << "   ✗ Failed to map file for reading: " << error.message() << "\n";
  }
}

void test_factory_functions(const std::string& filename)
{
  std::cout << "\n4. Testing factory functions:\n";
  std::error_code error;

  auto factory_source = ouly::make_mmap_source(filename, error);
  if (!error)
  {
    std::cout << "   ✓ make_mmap_source() works (" << factory_source.size() << " bytes)\n";
  }
  else
  {
    std::cout << "   ✗ make_mmap_source() failed: " << error.message() << "\n";
  }

  auto factory_sink = ouly::make_mmap_sink(filename, error);
  if (!error)
  {
    std::cout << "   ✓ make_mmap_sink() works (" << factory_sink.size() << " bytes)\n";
  }
  else
  {
    std::cout << "   ✗ make_mmap_sink() failed: " << error.message() << "\n";
  }
}

auto main() -> int
{
  std::cout << "Testing OULY Memory-Mapped Allocators (mio-style interface)\n";
  std::cout << "==========================================================\n\n";

  constexpr size_t k_file_size = 8192;

  // Test 1: Virtual Memory Allocator
  test_virtual_memory_allocator();
  std::cout << "\n";

  // Test 2-4: Memory-Mapped File (mio-style interface)
  try
  {
    // Create a test file first
    const std::string filename = "test_mmap_file.dat";

    {
      std::ofstream file(filename, std::ios::binary);
      file.seekp(k_file_size - 1);
      file.write("", 1); // Create sparse file
    }

    test_mmap_write_operations(filename);
    test_mmap_read_operations(filename);
    test_factory_functions(filename);

    // Clean up test file
    std::remove(filename.c_str());
  }
  catch (const std::exception& e)
  {
    std::cout << "   ✗ Exception: " << e.what() << "\n";
  }

  std::cout << "\nAll tests completed!\n";
  return 0;
}
