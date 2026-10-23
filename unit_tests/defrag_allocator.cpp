#include "catch2/catch_all.hpp"
#include "ouly/allocators/coalescing_allocator.hpp"
#include "ouly/allocators/coalescing_arena_allocator.hpp"
#include "ouly/allocators/linear_allocator.hpp"
#include <cstring>
#include <iostream>
#include <limits>
#include <unordered_map>
#include <vector>

// NOLINTBEGIN
namespace
{

// Counts the arenas a coalescing_arena_allocator holds
struct counting_manager
{
  uint32_t arena_count_ = 0;

  void add(ouly::arena_id, ouly::allocation_size_type)
  {
    arena_count_++;
  }

  void remove(ouly::arena_id)
  {
    arena_count_--;
  }
};

} // namespace

TEST_CASE("coalescing_arena_allocator: alignment is honored", "[coalescing_arena_allocator][default]")
{
  counting_manager                 mgr;
  ouly::coalescing_arena_allocator allocator{4096};

  auto odd = allocator.allocate(13, mgr);
  REQUIRE(odd.get_allocation_id() != ouly::allocation_id());

  auto aligned = allocator.allocate(100, mgr, ouly::alignment<256>());
  REQUIRE(aligned.get_allocation_id() != ouly::allocation_id());
  REQUIRE(aligned.get_offset() % 256 == 0);
  // the aligned range must fit inside the raw block
  auto const raw_offset = allocator.get_offset(aligned.get_allocation_id());
  auto const raw_size   = allocator.get_size(aligned.get_allocation_id());
  REQUIRE(aligned.get_offset() >= raw_offset);
  REQUIRE(aligned.get_offset() + 100 <= raw_offset + raw_size);

  auto small_align = allocator.allocate(24, mgr, ouly::alignment<8>());
  REQUIRE(small_align.get_offset() % 8 == 0);
  allocator.validate_integrity();

  for (auto const& al : {odd, aligned, small_align})
  {
    allocator.deallocate(al.get_allocation_id(), mgr);
  }
  REQUIRE(mgr.arena_count_ == 1); // the emptied arena is retained for reuse
  REQUIRE(allocator.trim(mgr) == 1);
  REQUIRE(mgr.arena_count_ == 0);
}

TEST_CASE("linear_allocator: small alignments after odd-sized allocations", "[linear_allocator][default]")
{
  ouly::linear_allocator<> allocator(1024);

  [[maybe_unused]] auto* odd = allocator.allocate(3);
  auto*                  p8  = allocator.allocate(16, ouly::alignment<8>());
  REQUIRE(reinterpret_cast<uintptr_t>(p8) % 8 == 0);
  auto* p4 = allocator.allocate(10, ouly::alignment<4>());
  REQUIRE(reinterpret_cast<uintptr_t>(p4) % 4 == 0);
  auto* p64 = allocator.allocate(32, ouly::alignment<64>());
  REQUIRE(reinterpret_cast<uintptr_t>(p64) % 64 == 0);
}

TEST_CASE("ouly::align rounds pointers up", "[alignment][default]")
{
  alignas(64) char buffer[128];
  REQUIRE(ouly::align(buffer, 64) == buffer);
  REQUIRE(ouly::align(buffer + 1, 64) == buffer + 64);
  REQUIRE(ouly::align(buffer + 63, 16) == buffer + 64);
}

TEST_CASE("coalescing_allocator: exhaustion empties the free list", "[coalescing_allocator][default]")
{
  using size_type     = ouly::coalescing_allocator::size_type;
  constexpr auto kMax = std::numeric_limits<size_type>::max();

  ouly::coalescing_allocator allocator;
  // consume the entire range, leaving no gap anywhere
  auto const all = allocator.allocate(kMax);
  REQUIRE(all != ouly::allocation_id());
  REQUIRE(allocator.get_offset(all) == 0);
  // exhausted: allocation must fail with a null id, not crash
  REQUIRE(allocator.allocate(1) == ouly::allocation_id());
  allocator.validate_integrity();
  // freeing re-opens the whole range
  allocator.deallocate(all);
  auto const first  = allocator.allocate(0x80);
  auto const second = allocator.allocate(0x80);
  REQUIRE(allocator.get_offset(first) == 0);
  REQUIRE(allocator.get_offset(second) == 0x80);

  ouly::coalescing_allocator bounded{0x100};
  auto const                 lhs = bounded.allocate(0x80);
  auto const                 rhs = bounded.allocate(0x80);
  REQUIRE(bounded.allocate(1) == ouly::allocation_id());
  bounded.deallocate(lhs);
  REQUIRE(bounded.get_offset(bounded.allocate(0x40, 0x40)) == 0);
  REQUIRE(bounded.get_offset(bounded.allocate(0x40)) == 0x40);
  REQUIRE(bounded.allocate(1) == ouly::allocation_id());
  bounded.deallocate(rhs);
  bounded.validate_integrity();
}

// NOLINTEND
