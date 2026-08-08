#include "ouly/allocators/linear_allocator.hpp"
#include "catch2/catch_all.hpp"
#include "ouly/allocators/linear_arena_allocator.hpp"
#include "ouly/allocators/linear_stack_allocator.hpp"

// NOLINTBEGIN
TEST_CASE("Validate linear_allocator", "[linear_allocator]")
{
  using allocator_t = ouly::linear_allocator<>;
  struct record
  {
    void*         data;
    std::uint32_t size;
  };
  constexpr std::uint32_t k_arena_size = 1000;
  allocator_t             allocator(k_arena_size);
  auto                    start  = ouly::allocate<std::uint8_t>(allocator, 40);
  auto                    off100 = ouly::allocate<std::uint8_t>(allocator, 100);
  CHECK(start + 40 == off100);
  allocator.deallocate(off100, 100);
  off100 = ouly::allocate<std::uint8_t>(allocator, 100);
  CHECK(start + 40 == off100);
}

TEST_CASE("Validate linear_arena_allocator without alignment", "[linear_arena_allocator]")
{
  using namespace ouly;
  using allocator_t = ouly::linear_arena_allocator<
   ouly::config<ouly::cfg::underlying_allocator<default_allocator<ouly::config<ouly::cfg::min_alignment<8>>>>>>;
  struct record
  {
    void*         data;
    std::uint32_t size;
  };
  constexpr std::uint32_t k_arena_size = 1000;
  allocator_t             allocator(k_arena_size);
  auto                    start  = ouly::allocate<std::uint8_t>(allocator, 40);
  auto                    first  = start;
  auto                    off100 = ouly::allocate<std::uint8_t>(allocator, 100);
  CHECK(start + 40 == off100);
  allocator.deallocate(off100, 100);
  off100 = ouly::allocate<std::uint8_t>(allocator, 100);
  CHECK(start + 40 == off100);
  auto new_arena = ouly::allocate<std::uint8_t>(allocator, 1000);
  CHECK(2 == allocator.get_arena_count());
  auto from_old = ouly::allocate<std::uint8_t>(allocator, 40);
  CHECK(off100 + 100 == from_old);
  allocator.deallocate(new_arena, 1000);
  new_arena = ouly::allocate<std::uint8_t>(allocator, 1000);
  CHECK(2 == allocator.get_arena_count());
  allocator.rewind();
  start = ouly::allocate<std::uint8_t>(allocator, 40);
  CHECK(start == first);
  CHECK(2 == allocator.get_arena_count());
  allocator.smart_rewind();
  start = ouly::allocate<std::uint8_t>(allocator, 40);
  CHECK(start == first);
  CHECK(1 == allocator.get_arena_count());
}

TEST_CASE("Validate linear_arena_allocator with alignment", "[linear_arena_allocator]")
{
  using namespace ouly;
  using allocator_t = ouly::linear_arena_allocator<
   ouly::config<ouly::cfg::underlying_allocator<default_allocator<ouly::config<ouly::cfg::min_alignment<128>>>>>>;
  struct record
  {
    void*         data;
    std::uint32_t size;
  };
  constexpr std::uint32_t k_arena_size = 1152;
  allocator_t             allocator(k_arena_size);
  auto                    start  = ouly::allocate<std::uint8_t>(allocator, 256, 128);
  auto                    first  = start;
  auto                    off100 = ouly::allocate<std::uint8_t>(allocator, 512, 128);
  CHECK(start + 256 == off100);
  allocator.deallocate(off100, 512);
  off100 = ouly::allocate<std::uint8_t>(allocator, 512, 128);
  CHECK(start + 256 == off100);
  auto new_arena = ouly::allocate<std::uint8_t>(allocator, 1024, 128);
  CHECK(2 == allocator.get_arena_count());
  auto from_old = ouly::allocate<std::uint8_t>(allocator, 256);
  CHECK(off100 + 512 == from_old);
  allocator.deallocate(new_arena, 1024);
  new_arena = ouly::allocate<std::uint8_t>(allocator, 1024, 128);
  CHECK(2 == allocator.get_arena_count());
  allocator.rewind();
  start = ouly::allocate<std::uint8_t>(allocator, 64, 128);
  CHECK(start == first);
  CHECK(2 == allocator.get_arena_count());
  allocator.smart_rewind();
  start = ouly::allocate<std::uint8_t>(allocator, 64, 128);
  CHECK(start == first);
  CHECK(1 == allocator.get_arena_count());
}

TEST_CASE("Validate linear_stack_allocator with alignment", "[linear_stack_allocator]")
{
  using namespace ouly;
  using allocator_t = linear_stack_allocator<>;
  struct record
  {
    void*         data;
    std::uint32_t size;
  };
  allocator_t   allocator(64);
  std::uint8_t* first = nullptr;
  {
    auto ar = allocator.get_auto_rewind_point();
    auto r1 = allocator.get_rewind_point();
    auto a1 = ouly::allocate<std::uint8_t>(allocator, 32, 0);
    first   = a1;
    allocator.rewind(r1);
    auto a2 = ouly::allocate<std::uint8_t>(allocator, 32, 0);
    CHECK(a1 == a2);
    allocator.rewind(r1);
    a1                       = ouly::allocate<std::uint8_t>(allocator, 32, 0);
    a2                       = ouly::allocate<std::uint8_t>(allocator, 32, 0);
    [[maybe_unused]] auto a3 = ouly::allocate<std::uint8_t>(allocator, 16, 0);
    auto                  r2 = allocator.get_rewind_point();
    auto                  a4 = ouly::allocate<std::uint8_t>(allocator, 32, 0);
    [[maybe_unused]] auto a6 = ouly::allocate<std::uint8_t>(allocator, 32, 0);
    allocator.rewind(r2);
    auto a7 = ouly::allocate<std::uint8_t>(allocator, 32, 0);
    CHECK(a4 == a7);
    auto r3                   = allocator.get_rewind_point();
    a7                        = ouly::allocate<std::uint8_t>(allocator, 2, 0);
    [[maybe_unused]] auto a8  = ouly::allocate<std::uint8_t>(allocator, 128, 0);
    [[maybe_unused]] auto a9  = ouly::allocate<std::uint8_t>(allocator, 32, 0);
    [[maybe_unused]] auto a10 = ouly::allocate<std::uint8_t>(allocator, 64, 0);
    allocator.rewind(r3);
    auto a11 = ouly::allocate<std::uint8_t>(allocator, 16, 0);
    CHECK(a7 == a11);
  }
  auto a1 = ouly::allocate<std::uint8_t>(allocator, 32, 0);
  CHECK(a1 == first);
}

namespace
{
void fill_pattern(std::uint8_t* data, std::uint32_t size)
{
  for (std::uint32_t index = 0; index < size; ++index)
  {
    data[index] = static_cast<std::uint8_t>((index * 7 + 1) & 0xff);
  }
}

auto has_pattern(std::uint8_t const* data, std::uint32_t size) -> bool
{
  for (std::uint32_t index = 0; index < size; ++index)
  {
    if (data[index] != static_cast<std::uint8_t>((index * 7 + 1) & 0xff))
    {
      return false;
    }
  }
  return true;
}
} // namespace

TEST_CASE("Validate linear_allocator realloc", "[linear_allocator]")
{
  using allocator_t                    = ouly::linear_allocator<>;
  constexpr std::uint32_t k_arena_size = 1000;

  allocator_t allocator(k_arena_size);
  auto*       block = ouly::allocate<std::uint8_t>(allocator, 40);
  fill_pattern(block, 40);

  // The block is at the head of the arena, so it simply grows in place
  auto* grown = static_cast<std::uint8_t*>(allocator.realloc(block, 40, 100));
  CHECK(grown == block);
  CHECK(has_pattern(grown, 40));
  CHECK(allocator.get_free_size() == k_arena_size - 100);

  // Shrinking the head hands the tail back to the arena
  auto* shrunk = static_cast<std::uint8_t*>(allocator.realloc(grown, 100, 60));
  CHECK(shrunk == block);
  CHECK(has_pattern(shrunk, 40));
  CHECK(allocator.get_free_size() == k_arena_size - 60);

  // Once another allocation sits on top the block has to move
  auto* top   = ouly::allocate<std::uint8_t>(allocator, 20);
  auto* moved = static_cast<std::uint8_t*>(allocator.realloc(shrunk, 60, 120));
  CHECK(moved == top + 20);
  CHECK(has_pattern(moved, 40));
  CHECK(allocator.get_free_size() == k_arena_size - 200);
}

TEST_CASE("Validate linear_arena_allocator realloc", "[linear_arena_allocator]")
{
  using allocator_t                    = ouly::linear_arena_allocator<>;
  constexpr std::uint32_t k_arena_size = 1000;

  allocator_t allocator(k_arena_size);
  auto*       block = ouly::allocate<std::uint8_t>(allocator, 800);
  fill_pattern(block, 800);

  auto* grown = static_cast<std::uint8_t*>(allocator.realloc(block, 800, 900));
  CHECK(grown == block);
  CHECK(allocator.get_arena_count() == 1);

  // A block that no longer fits its arena moves to a fresh one, contents intact
  auto* moved = static_cast<std::uint8_t*>(allocator.realloc(grown, 900, 1200));
  CHECK(moved != grown);
  CHECK(has_pattern(moved, 800));
  CHECK(allocator.get_arena_count() == 2);

  // The vacated block was released, so the first arena can serve it again
  auto* reused = ouly::allocate<std::uint8_t>(allocator, 900);
  CHECK(reused == block);
  CHECK(allocator.get_arena_count() == 2);
}

TEST_CASE("Validate linear_stack_allocator realloc", "[linear_stack_allocator]")
{
  using allocator_t = ouly::linear_stack_allocator<>;

  allocator_t allocator(256);
  auto*       block = ouly::allocate<std::uint8_t>(allocator, 32);
  fill_pattern(block, 32);

  auto* grown = static_cast<std::uint8_t*>(allocator.realloc(block, 32, 64));
  CHECK(grown == block);
  CHECK(has_pattern(grown, 32));
  CHECK(allocator.get_arena_count() == 1);

  auto* top   = ouly::allocate<std::uint8_t>(allocator, 16);
  auto* moved = static_cast<std::uint8_t*>(allocator.realloc(grown, 64, 96));
  CHECK(moved == top + 16);
  CHECK(has_pattern(moved, 32));
  CHECK(allocator.get_arena_count() == 1);

  // Growing past the arena capacity takes a new arena
  auto* large = static_cast<std::uint8_t*>(allocator.realloc(moved, 96, 512));
  CHECK(has_pattern(large, 32));
  CHECK(allocator.get_arena_count() == 2);
}

TEST_CASE("linear_allocator does not pay for alignment it already has", "[linear_allocator]")
{
  using allocator_t                    = ouly::linear_allocator<>;
  constexpr std::uint32_t k_arena_size = 1024;

  allocator_t allocator(k_arena_size);

  // The arena base is max_align_t aligned, so an aligned request off a matching head costs nothing
  auto* first = static_cast<std::uint8_t*>(allocator.allocate(64, ouly::alignment<16>()));
  CHECK(reinterpret_cast<std::uintptr_t>(first) % 16 == 0);
  CHECK(allocator.get_free_size() == k_arena_size - 64);

  auto* second = static_cast<std::uint8_t*>(allocator.allocate(32, ouly::alignment<16>()));
  CHECK(second == first + 64);
  CHECK(allocator.get_free_size() == k_arena_size - 96);

  // A head that is off the boundary is padded by exactly what it takes to reach it
  auto* odd = static_cast<std::uint8_t*>(allocator.allocate(8));
  CHECK(odd == second + 32);
  CHECK(allocator.get_free_size() == k_arena_size - 104);

  // 8 bytes of padding to get from 104 back onto the 16 byte boundary, and not one more
  auto* realigned = static_cast<std::uint8_t*>(allocator.allocate(16, ouly::alignment<16>()));
  CHECK(realigned == odd + 16);
  CHECK(allocator.get_free_size() == k_arena_size - 128);

  auto* over = static_cast<std::uint8_t*>(allocator.allocate(16, ouly::alignment<64>()));
  CHECK(reinterpret_cast<std::uintptr_t>(over) % 64 == 0);
  CHECK(over >= realigned + 16);
}

TEST_CASE("linear allocators take an alignment known only at runtime", "[linear_allocator]")
{
  constexpr std::uint32_t k_arena_size = 4096;

  SECTION("linear_allocator")
  {
    ouly::linear_allocator<> allocator(k_arena_size);
    auto*                    block = static_cast<std::uint8_t*>(allocator.allocate(100, std::align_val_t{64}));
    CHECK(reinterpret_cast<std::uintptr_t>(block) % 64 == 0);
    CHECK(allocator.get_free_size() == k_arena_size - 100);
    allocator.deallocate(block, 100, std::align_val_t{64});
    CHECK(allocator.get_free_size() == k_arena_size);
  }

  SECTION("linear_arena_allocator")
  {
    ouly::linear_arena_allocator<> allocator(k_arena_size);
    auto*                          block = static_cast<std::uint8_t*>(allocator.allocate(100, std::align_val_t{256}));
    CHECK(reinterpret_cast<std::uintptr_t>(block) % 256 == 0);
    allocator.deallocate(block, 100, std::align_val_t{256});
    CHECK(allocator.get_arena_count() == 1);
  }

  SECTION("linear_stack_allocator")
  {
    ouly::linear_stack_allocator<> allocator(k_arena_size);
    auto*                          block = static_cast<std::uint8_t*>(allocator.allocate(100, std::align_val_t{128}));
    CHECK(reinterpret_cast<std::uintptr_t>(block) % 128 == 0);
    auto* next = static_cast<std::uint8_t*>(allocator.allocate(64, std::align_val_t{128}));
    CHECK(reinterpret_cast<std::uintptr_t>(next) % 128 == 0);
    CHECK(next == block + 128);
  }
}

TEST_CASE("linear_arena_allocator serves over-aligned requests from a fresh arena", "[linear_arena_allocator]")
{
  // An arena small enough that the request has to open a new one
  ouly::linear_arena_allocator<> allocator(64);

  auto* block = static_cast<std::uint8_t*>(allocator.allocate(4096, ouly::alignment<4096>()));
  CHECK(block != nullptr);
  CHECK(reinterpret_cast<std::uintptr_t>(block) % 4096 == 0);
}
// NOLINTEND