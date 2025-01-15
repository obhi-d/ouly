#include <acl/allocators/linear_allocator.hpp>
#include <acl/allocators/linear_arena_allocator.hpp>
#include <acl/allocators/linear_stack_allocator.hpp>
#include <catch2/catch_all.hpp>

// NOLINTBEGIN
TEST_CASE("Validate linear_allocator", "[linear_allocator]")
{
  using allocator_t = acl::linear_allocator<>;
  struct record
  {
    void*         data;
    std::uint32_t size;
  };
  constexpr std::uint32_t k_arena_size = 1000;
  allocator_t             allocator(k_arena_size);
  auto                    start  = acl::allocate<std::uint8_t>(allocator, 40);
  auto                    off100 = acl::allocate<std::uint8_t>(allocator, 100);
  CHECK(start + 40 == off100);
  allocator.deallocate(off100, 100);
  off100 = acl::allocate<std::uint8_t>(allocator, 100);
  CHECK(start + 40 == off100);
}

TEST_CASE("Validate linear_arena_allocator without alignment", "[linear_arena_allocator]")
{
  using namespace acl;
  using allocator_t = acl::linear_arena_allocator<
   acl::config<acl::cfg::underlying_allocator<default_allocator<acl::config<acl::cfg::min_alignment<8>>>>>>;
  struct record
  {
    void*         data;
    std::uint32_t size;
  };
  constexpr std::uint32_t k_arena_size = 1000;
  allocator_t             allocator(k_arena_size);
  auto                    start  = acl::allocate<std::uint8_t>(allocator, 40);
  auto                    first  = start;
  auto                    off100 = acl::allocate<std::uint8_t>(allocator, 100);
  CHECK(start + 40 == off100);
  allocator.deallocate(off100, 100);
  off100 = acl::allocate<std::uint8_t>(allocator, 100);
  CHECK(start + 40 == off100);
  auto new_arena = acl::allocate<std::uint8_t>(allocator, 1000);
  CHECK(2 == allocator.get_arena_count());
  auto from_old = acl::allocate<std::uint8_t>(allocator, 40);
  CHECK(off100 + 100 == from_old);
  allocator.deallocate(new_arena, 1000);
  new_arena = acl::allocate<std::uint8_t>(allocator, 1000);
  CHECK(2 == allocator.get_arena_count());
  allocator.rewind();
  start = acl::allocate<std::uint8_t>(allocator, 40);
  CHECK(start == first);
  CHECK(2 == allocator.get_arena_count());
  allocator.smart_rewind();
  start = acl::allocate<std::uint8_t>(allocator, 40);
  CHECK(start == first);
  CHECK(1 == allocator.get_arena_count());
}

TEST_CASE("Validate linear_arena_allocator with alignment", "[linear_arena_allocator]")
{
  using namespace acl;
  using allocator_t = acl::linear_arena_allocator<
   acl::config<acl::cfg::underlying_allocator<default_allocator<acl::config<acl::cfg::min_alignment<128>>>>>>;
  struct record
  {
    void*         data;
    std::uint32_t size;
  };
  constexpr std::uint32_t k_arena_size = 1152;
  allocator_t             allocator(k_arena_size);
  auto                    start  = acl::allocate<std::uint8_t>(allocator, 256, 128);
  auto                    first  = start;
  auto                    off100 = acl::allocate<std::uint8_t>(allocator, 512, 128);
  CHECK(start + 256 == off100);
  allocator.deallocate(off100, 512);
  off100 = acl::allocate<std::uint8_t>(allocator, 512, 128);
  CHECK(start + 256 == off100);
  auto new_arena = acl::allocate<std::uint8_t>(allocator, 1024, 128);
  CHECK(2 == allocator.get_arena_count());
  auto from_old = acl::allocate<std::uint8_t>(allocator, 256);
  CHECK(off100 + 512 == from_old);
  allocator.deallocate(new_arena, 1024);
  new_arena = acl::allocate<std::uint8_t>(allocator, 1024, 128);
  CHECK(2 == allocator.get_arena_count());
  allocator.rewind();
  start = acl::allocate<std::uint8_t>(allocator, 64, 128);
  CHECK(start == first);
  CHECK(2 == allocator.get_arena_count());
  allocator.smart_rewind();
  start = acl::allocate<std::uint8_t>(allocator, 64, 128);
  CHECK(start == first);
  CHECK(1 == allocator.get_arena_count());
}

TEST_CASE("Validate linear_stack_allocator with alignment", "[linear_stack_allocator]")
{
  using namespace acl;
  using allocator_t = linear_stack_allocator<>;
  struct record
  {
    void*         data;
    std::uint32_t size;
  };
  constexpr std::uint32_t k_arena_size = 1;
  allocator_t             allocator(64);
  std::uint8_t*           first = nullptr;
  {
    auto ar = allocator.get_auto_rewind_point();
    auto r1 = allocator.get_rewind_point();
    auto a1 = acl::allocate<std::uint8_t>(allocator, 32, 0);
    first   = a1;
    allocator.rewind(r1);
    auto a2 = acl::allocate<std::uint8_t>(allocator, 32, 0);
    CHECK(a1 == a2);
    allocator.rewind(r1);
    a1      = acl::allocate<std::uint8_t>(allocator, 32, 0);
    a2      = acl::allocate<std::uint8_t>(allocator, 32, 0);
    auto a3 = acl::allocate<std::uint8_t>(allocator, 16, 0);
    auto r2 = allocator.get_rewind_point();
    auto a4 = acl::allocate<std::uint8_t>(allocator, 32, 0);
    auto a6 = acl::allocate<std::uint8_t>(allocator, 32, 0);
    allocator.rewind(r2);
    auto a7 = acl::allocate<std::uint8_t>(allocator, 32, 0);
    CHECK(a4 == a7);
    auto r3  = allocator.get_rewind_point();
    a7       = acl::allocate<std::uint8_t>(allocator, 2, 0);
    auto a8  = acl::allocate<std::uint8_t>(allocator, 128, 0);
    auto a9  = acl::allocate<std::uint8_t>(allocator, 32, 0);
    auto a10 = acl::allocate<std::uint8_t>(allocator, 64, 0);
    allocator.rewind(r3);
    auto a11 = acl::allocate<std::uint8_t>(allocator, 16, 0);
    CHECK(a7 == a11);
  }
  auto a1 = acl::allocate<std::uint8_t>(allocator, 32, 0);
  CHECK(a1 == first);
}
// NOLINTEND