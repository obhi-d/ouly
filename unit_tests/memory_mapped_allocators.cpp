#include "catch2/catch_all.hpp"
#include "ouly/allocators/mmap_file.hpp"
#include "ouly/allocators/virtual_allocator.hpp"
#include <algorithm>
#include <fstream>
#include <string>
#include <system_error>

// NOLINTBEGIN

namespace
{
auto create_test_file(const std::filesystem::path& filename, size_t size) -> void
{
  std::ofstream file(filename, std::ios::binary);
  file.seekp(size - 1);
  file.write("", 1); // Create sparse file
}

auto cleanup_test_file(const std::filesystem::path& filename) -> void
{
  std::filesystem::remove(filename);
}
} // namespace

TEST_CASE("Validate virtual_allocator", "[virtual_allocator]")
{
  using allocator_t = ouly::virtual_allocator<>;

  SECTION("Basic allocation and deallocation")
  {
    allocator_t      allocator;
    constexpr size_t alloc_size = 4096;

    auto* ptr = allocator.allocate(alloc_size);
    REQUIRE(ptr != allocator_t::null());

    // Write and read data to verify the memory is usable
    auto*       data         = static_cast<char*>(ptr);
    std::string test_message = "Hello from virtual memory!";
    std::ranges::copy(test_message, data);
    data[test_message.size()] = '\0';

    REQUIRE(std::string(data) == test_message);

    allocator.deallocate(ptr, alloc_size);
  }

  SECTION("Multiple allocations")
  {
    allocator_t      allocator;
    constexpr size_t alloc_size = 1024;
    constexpr size_t num_allocs = 5;

    std::vector<void*> ptrs;
    ptrs.reserve(num_allocs);

    // Allocate multiple blocks
    for (size_t i = 0; i < num_allocs; ++i)
    {
      auto* ptr = allocator.allocate(alloc_size);
      REQUIRE(ptr != allocator_t::null());
      ptrs.push_back(ptr);
    }

    // Verify all allocations are unique
    for (size_t i = 0; i < num_allocs; ++i)
    {
      for (size_t j = i + 1; j < num_allocs; ++j)
      {
        REQUIRE(ptrs[i] != ptrs[j]);
      }
    }

    // Deallocate all blocks
    for (auto* ptr : ptrs)
    {
      allocator.deallocate(ptr, alloc_size);
    }
  }
}

TEST_CASE("Validate mmap_sink (read-write file mapping)", "[mmap_sink]")
{
  const std::filesystem::path filename  = "test_mmap_sink.dat";
  constexpr size_t            file_size = 8192;

  // Create test file
  create_test_file(filename, file_size);

  SECTION("Basic file mapping and container interface")
  {
    ouly::mmap_sink sink;

    sink.map(filename);
    REQUIRE_FALSE(sink.empty());
    REQUIRE(sink.size() == file_size);
    REQUIRE(sink.data() != nullptr);
  }

  SECTION("Write operations using container interface")
  {
    ouly::mmap_sink sink;
    sink.map(filename);

    // Test iterator-based operations
    constexpr unsigned char fill_pattern = 0xAB;
    std::ranges::fill(sink, fill_pattern);

    // Verify fill pattern
    REQUIRE(*sink.begin() == fill_pattern);
    REQUIRE(*sink.rbegin() == fill_pattern);
    REQUIRE(sink[0] == fill_pattern);
    REQUIRE(sink[sink.size() - 1] == fill_pattern);

    // Write a message at the beginning
    std::string message = "Test message in mmap!";
    auto*       dest    = sink.begin();
    for (char c : message)
    {
      *dest++ = static_cast<unsigned char>(c);
    }
    sink[message.size()] = 0; // Null terminator

    // Verify the message was written
    for (size_t i = 0; i < message.size(); ++i)
    {
      REQUIRE(static_cast<char>(sink[i]) == message[i]);
    }
    REQUIRE(sink[message.size()] == 0);

    // Test sync operation
    sink.sync();
  }

  SECTION("Iterator compatibility")
  {
    ouly::mmap_sink sink;

    sink.map(filename);
    REQUIRE_FALSE(sink.empty());

    // Test that iterators work with STL algorithms
    auto begin_it  = sink.begin();
    auto end_it    = sink.end();
    auto rbegin_it = sink.rbegin();
    auto rend_it   = sink.rend();

    REQUIRE(std::distance(begin_it, end_it) == static_cast<std::ptrdiff_t>(file_size));
    REQUIRE(std::distance(rbegin_it, rend_it) == static_cast<std::ptrdiff_t>(file_size));

    // Test iterator arithmetic
    REQUIRE(begin_it + file_size == end_it);
    REQUIRE(end_it - file_size == begin_it);
  }

  cleanup_test_file(filename);
}

TEST_CASE("Validate mmap_source (read-only file mapping)", "[mmap_source]")
{
  const std::string filename     = "test_mmap_source.dat";
  constexpr size_t  file_size    = 4096;
  const std::string test_content = "Read-only test content for mmap_source";

  // Create test file with content
  {
    std::ofstream file(filename, std::ios::binary);
    file.write(test_content.c_str(), test_content.size());
    file.seekp(file_size - 1);
    file.write("", 1); // Pad to desired size
  }

  SECTION("Basic read-only mapping")
  {
    ouly::mmap_source source;
    source.map(filename);
    REQUIRE_FALSE(source.empty());
    REQUIRE(source.size() == file_size);
    REQUIRE(source.data() != nullptr);
  }

  SECTION("Read operations using container interface")
  {
    ouly::mmap_source source;
    source.map(filename);
    REQUIRE_FALSE(source.empty());

    // Verify we can read the content we wrote
    for (size_t i = 0; i < test_content.size(); ++i)
    {
      REQUIRE(static_cast<char>(source[i]) == test_content[i]);
    }

    // Test const iterators
    auto begin_it = source.cbegin();
    auto end_it   = source.cend();
    REQUIRE(std::distance(begin_it, end_it) == static_cast<std::ptrdiff_t>(file_size));

    // Verify first byte matches expected content
    REQUIRE(static_cast<char>(*source.cbegin()) == test_content[0]);
  }

  cleanup_test_file(filename);
}

TEST_CASE("Validate factory functions", "[mmap_factory]")
{
  const std::string filename  = "test_factory.dat";
  constexpr size_t  file_size = 2048;

  create_test_file(filename, file_size);

  SECTION("make_mmap_source factory")
  {
    auto source = ouly::make_mmap_source(filename);

    REQUIRE_FALSE(source.empty());
    REQUIRE(source.size() == file_size);
  }

  SECTION("make_mmap_sink factory")
  {
    auto sink = ouly::make_mmap_sink(filename);

    REQUIRE_FALSE(sink.empty());
    REQUIRE(sink.size() == file_size);

    // Test that we can write to the factory-created sink
    constexpr unsigned char test_byte = 0xFF;
    sink[0]                           = test_byte;
    REQUIRE(sink[0] == test_byte);
  }

  cleanup_test_file(filename);
}

TEST_CASE("Validate error handling", "[mmap_error_handling]")
{
  SECTION("Non-existent file")
  {
    ouly::mmap_source source;

    try
    {
      source.map("non_existent_file.dat");
      FAIL("Expected exception for non-existent file");
    }
    catch (const std::system_error& /*e*/)
    {}
  }

  SECTION("Factory function with non-existent file")
  {

    try
    {
      [[maybe_unused]] auto source = ouly::make_mmap_source("non_existent_file.dat");
      FAIL("Expected exception for non-existent file");
    }
    catch (const std::system_error& /*e*/)
    {}
  }
}
// NOLINTEND
