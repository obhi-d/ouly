#include "catch2/catch_all.hpp"
#include "ouly/allocators/arena_allocator.hpp"
#include "ouly/allocators/best_fit_defrag_allocator.hpp"
#include "ouly/allocators/coalescing_allocator.hpp"
#include "ouly/allocators/first_fit_defrag_allocator.hpp"
#include "ouly/allocators/linear_allocator.hpp"
#include <cstring>
#include <iostream>
#include <limits>
#include <unordered_map>
#include <vector>

// NOLINTBEGIN
namespace
{

uint32_t xorshift(uint32_t& state)
{
  uint32_t x = state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  return state = x;
}

// Defragmentation-capable memory manager: keeps real backing memory per arena, fills every
// allocation with a deterministic pattern and verifies the pattern survives defragmentation.
template <typename Allocator>
struct defrag_mem_manager
{
  struct allocation
  {
    uint16_t arena_  = 0;
    uint32_t offset_ = 0;
    uint32_t size_   = 0;
  };

  std::vector<std::vector<char>>           arenas_;
  std::unordered_map<uint32_t, allocation> allocs_;
  uint32_t                                 arena_count_  = 0;
  uint32_t                                 rebind_count_ = 0;
  bool                                     in_defrag_    = false;

  static char pattern(uint32_t id, uint32_t i)
  {
    return static_cast<char>((id * 31U + i * 7U + 0x5U) & 0xFFU);
  }

  void track(ouly::ca_allocation const& al, uint32_t size)
  {
    auto id     = al.get_allocation_id().get();
    allocs_[id] = {al.get_arena_id().get(), al.get_offset(), size};
    auto& data  = arenas_[al.get_arena_id().get()];
    REQUIRE(al.get_offset() + size <= data.size());
    for (uint32_t i = 0; i < size; ++i)
      data[al.get_offset() + i] = pattern(id, i);
  }

  void verify_all() const
  {
    bool ok = true;
    for (auto const& [id, al] : allocs_)
    {
      REQUIRE(al.arena_ < arenas_.size());
      REQUIRE(al.offset_ + al.size_ <= arenas_[al.arena_].size());
      for (uint32_t i = 0; i < al.size_; ++i)
        ok &= arenas_[al.arena_][al.offset_ + i] == pattern(id, i);
    }
    REQUIRE(ok);
  }

  // CoalescingMemoryManager
  void add(ouly::arena_id id, ouly::allocation_size_type size)
  {
    if (id.get() >= arenas_.size())
      arenas_.resize(id.get() + 1);
    arenas_[id.get()].assign(size, 0x17);
    arena_count_++;
  }

  void remove(ouly::arena_id id)
  {
    REQUIRE(!arenas_[id.get()].empty());
    arenas_[id.get()].clear();
    arenas_[id.get()].shrink_to_fit();
    arena_count_--;
  }

  // CoalescingDefragMemoryManager
  void begin_defragment(Allocator&)
  {
    in_defrag_ = true;
  }

  void end_defragment(Allocator&)
  {
    in_defrag_ = false;
    verify_all();
  }

  void move_memory(ouly::arena_id src, ouly::arena_id dst, uint32_t from, uint32_t to, uint32_t size)
  {
    REQUIRE(in_defrag_);
    REQUIRE(from + size <= arenas_[src.get()].size());
    REQUIRE(to + size <= arenas_[dst.get()].size());
    std::memmove(arenas_[dst.get()].data() + to, arenas_[src.get()].data() + from, size);
  }

  void rebind_alloc(ouly::allocation_id id, ouly::arena_id arena, uint32_t offset)
  {
    auto it = allocs_.find(id.get());
    REQUIRE(it != allocs_.end());
    it->second.arena_  = arena.get();
    it->second.offset_ = offset;
    rebind_count_++;
  }
};

} // namespace

TEMPLATE_TEST_CASE("defrag allocators: randomized alloc/free/defragment", "[defrag_allocator][default]",
                   ouly::first_fit_defrag_allocator, ouly::best_fit_defrag_allocator)
{
  uint32_t seed = Catch::getSeed();
  std::cout << " Seed : " << seed << std::endl;

  constexpr uint32_t           page_size = 10000;
  defrag_mem_manager<TestType> mgr;
  TestType                     allocator{page_size};
  std::vector<uint32_t>        live;

  for (uint32_t iter = 0; iter < 2500; ++iter)
  {
    auto const action = xorshift(seed) % 100;
    if (action < 55 || live.empty())
    {
      auto const size    = (xorshift(seed) % (page_size / 2)) + 1;
      bool const aligned = (xorshift(seed) & 1) != 0;
      auto const al = aligned ? allocator.allocate(size, mgr, ouly::alignment<256>()) : allocator.allocate(size, mgr);
      REQUIRE(al.get_allocation_id() != ouly::allocation_id());
      if (aligned)
        REQUIRE((al.get_offset() % 256) == 0);
      mgr.track(al, size);
      live.push_back(al.get_allocation_id().get());
    }
    else if (action < 90)
    {
      auto const idx = xorshift(seed) % live.size();
      auto const id  = live[idx];
      allocator.deallocate(ouly::allocation_id{id}, mgr);
      mgr.allocs_.erase(id);
      live[idx] = live.back();
      live.pop_back();
    }
    else
    {
      bool const budgeted = (xorshift(seed) & 1) != 0;
      auto const budget   = budgeted ? xorshift(seed) % (2 * page_size) : std::numeric_limits<uint32_t>::max();
      auto const result   = allocator.defragment(mgr, budget);
      REQUIRE(result.bytes_moved_ <= budget);
      if (!budgeted)
        REQUIRE(result.completed_);
    }
    allocator.validate_integrity();
  }

  mgr.verify_all();
  for (auto id : live)
  {
    allocator.deallocate(ouly::allocation_id{id}, mgr);
    mgr.allocs_.erase(id);
  }
  allocator.validate_integrity();
  REQUIRE(mgr.arena_count_ == 0);
}

TEMPLATE_TEST_CASE("defrag allocators: consolidates sparse arenas", "[defrag_allocator][default]",
                   ouly::first_fit_defrag_allocator, ouly::best_fit_defrag_allocator)
{
  constexpr uint32_t           page_size = 1024;
  defrag_mem_manager<TestType> mgr;
  TestType                     allocator{page_size};

  // fill several arenas with small allocations
  std::vector<uint32_t> ids;
  for (uint32_t i = 0; i < 32; ++i)
  {
    auto al = allocator.allocate(128, mgr);
    mgr.track(al, 128);
    ids.push_back(al.get_allocation_id().get());
  }
  auto const arenas_before = mgr.arena_count_;
  REQUIRE(arenas_before >= 4);

  // free 3 out of every 4 allocations: arenas become sparse but not empty
  for (uint32_t i = 0; i < ids.size(); ++i)
  {
    if (i % 4 != 0)
    {
      allocator.deallocate(ouly::allocation_id{ids[i]}, mgr);
      mgr.allocs_.erase(ids[i]);
    }
  }
  REQUIRE(mgr.arena_count_ == arenas_before); // nothing dropped yet

  auto const result = allocator.defragment(mgr);
  REQUIRE(result.completed_);
  REQUIRE(result.arenas_removed_ >= 1);
  REQUIRE(mgr.arena_count_ < arenas_before);
  allocator.validate_integrity();

  // surviving allocations are usable: ids stable, data verified by end_defragment already
  for (uint32_t i = 0; i < ids.size(); i += 4)
  {
    REQUIRE(allocator.get_size(ouly::allocation_id{ids[i]}) >= 128);
    allocator.deallocate(ouly::allocation_id{ids[i]}, mgr);
    mgr.allocs_.erase(ids[i]);
  }
  REQUIRE(mgr.arena_count_ == 0);
}

TEMPLATE_TEST_CASE("defrag allocators: budget makes defragment incremental", "[defrag_allocator][default]",
                   ouly::first_fit_defrag_allocator, ouly::best_fit_defrag_allocator)
{
  constexpr uint32_t           page_size = 1024;
  defrag_mem_manager<TestType> mgr;
  TestType                     allocator{page_size};

  std::vector<uint32_t> ids;
  for (uint32_t i = 0; i < 24; ++i)
  {
    auto al = allocator.allocate(200, mgr);
    mgr.track(al, 200);
    ids.push_back(al.get_allocation_id().get());
  }
  for (uint32_t i = 0; i < ids.size(); i += 2)
  {
    allocator.deallocate(ouly::allocation_id{ids[i]}, mgr);
    mgr.allocs_.erase(ids[i]);
  }

  auto const first = allocator.defragment(mgr, 400);
  REQUIRE(first.bytes_moved_ <= 400);
  REQUIRE(!first.completed_);
  allocator.validate_integrity();

  // repeated budgeted passes eventually converge to a fully compacted state
  for (uint32_t pass = 0; pass < 64; ++pass)
  {
    if (allocator.defragment(mgr, 400).completed_)
      break;
    allocator.validate_integrity();
  }
  auto const final_pass = allocator.defragment(mgr);
  REQUIRE(final_pass.completed_);
  REQUIRE(final_pass.allocations_moved_ == 0);
  allocator.validate_integrity();
}

TEMPLATE_TEST_CASE("defrag allocators: dedicated allocations are pinned", "[defrag_allocator][default]",
                   ouly::first_fit_defrag_allocator, ouly::best_fit_defrag_allocator)
{
  constexpr uint32_t           page_size = 1024;
  defrag_mem_manager<TestType> mgr;
  TestType                     allocator{page_size};

  auto small = allocator.allocate(64, mgr);
  mgr.track(small, 64);
  auto dedicated = allocator.allocate(128, mgr, ouly::alignment<>{}, std::true_type{});
  mgr.track(dedicated, 128);
  auto big = allocator.allocate(2 * page_size, mgr); // dedicated by size
  mgr.track(big, 2 * page_size);

  auto const result = allocator.defragment(mgr);
  REQUIRE(result.completed_);
  REQUIRE(allocator.get_arena(dedicated.get_allocation_id()) == dedicated.get_arena_id());
  REQUIRE(allocator.get_arena(big.get_allocation_id()) == big.get_arena_id());
  allocator.validate_integrity();

  for (auto const& al : {small, dedicated, big})
  {
    allocator.deallocate(al.get_allocation_id(), mgr);
    mgr.allocs_.erase(al.get_allocation_id().get());
  }
  REQUIRE(mgr.arena_count_ == 0);
}

TEST_CASE("coalescing_arena_allocator: alignment is honored", "[coalescing_arena_allocator][default]")
{
  defrag_mem_manager<ouly::coalescing_arena_allocator> mgr;
  ouly::coalescing_arena_allocator                     allocator{4096};

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
  REQUIRE(mgr.arena_count_ == 0);
}

TEST_CASE("arena_allocator: alignment is honored", "[arena_allocator][default]")
{
  ouly::arena_allocator<> allocator(4096);

  auto [odd_id, odd_offset] = allocator.allocate(13);
  REQUIRE(odd_id != allocator.null());

  auto [aligned_id, aligned_offset] = allocator.allocate(100, ouly::alignment<256>());
  REQUIRE(aligned_id != allocator.null());
  REQUIRE(aligned_offset % 256 == 0);

  auto [small_id, small_offset] = allocator.allocate(24, ouly::alignment<8>());
  REQUIRE(small_id != allocator.null());
  REQUIRE(small_offset % 8 == 0);
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
  // consume the entire initial range, leaving the free list empty
  REQUIRE(allocator.allocate(kMax) == 0);
  // exhausted: allocation must fail with the sentinel, not crash
  REQUIRE(allocator.allocate(1) == kMax);
  // deallocating into an empty free list must re-seed it
  allocator.deallocate(0x100, 0x100);
  REQUIRE(allocator.allocate(0x80) == 0x100);
  REQUIRE(allocator.allocate(0x80) == 0x180);
  REQUIRE(allocator.allocate(1) == kMax);
}
// NOLINTEND
