#include <acl/allocators/coalescing_allocator.hpp>
#include <acl/allocators/coalescing_arena_allocator.hpp>
#include <catch2/catch_all.hpp>
#include <iostream>
#include <unordered_set>

struct alloc_mem_manager
{
  using arena_data_t = std::vector<char>;
  struct allocation
  {
    acl::allocation_id alloc_id;
    acl::arena_id      arena;
    std::size_t        offset;
    std::size_t        size = 0;
    allocation()            = default;
    allocation(acl::allocation_id id, acl::arena_id arena, std::size_t ioffset, std::size_t isize)
        : alloc_id(id), arena(arena), offset(ioffset), size(isize)
    {}
  };

  std::vector<arena_data_t> arenas;
  std::vector<arena_data_t> backup_arenas;
  std::vector<allocation>   allocs;
  std::vector<allocation>   backup_allocs;
  uint32_t                  arena_count = 0;

  bool drop_arena([[maybe_unused]] acl::uhandle id)
  {
    arenas[id].clear();
    return true;
  }

  void fill(allocation const& l)
  {
    std::minstd_rand                   gen;
    std::uniform_int_distribution<int> generator(65, 122);
    for (std::size_t s = 0; s < l.size; ++s)
      arenas[l.arena.id][s + l.offset] = static_cast<char>(generator(gen));
  }

  void add([[maybe_unused]] acl::arena_id id, [[maybe_unused]] uint32_t size)
  {
    arena_data_t arena;
    arena.resize(size, 0x17);
    if (id.id >= arenas.size())
      arenas.resize(id.id + 1);

    arenas[id.id] = std::move(arena);
    arena_count++;
  }

  void remove(acl::arena_id h)
  {
    arenas[h.id].clear();
    arenas[h.id].shrink_to_fit();
    arena_count--;
  }
};

uint32_t xorshift(uint32_t& state)
{
  uint32_t x = state;

  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;

  return state = x;
}

TEST_CASE("coalescing_arena_allocator all tests", "[coalescing_arena_allocator][default]")
{
  uint32_t seed = Catch::getSeed();
  std::cout << " Seed : " << seed << std::endl;
  seed = 1847702527;
  enum action
  {
    e_allocate,
    e_deallocate
  };
  constexpr uint32_t              page_size = 10000;
  alloc_mem_manager               mgr;
  acl::coalescing_arena_allocator allocator;
  allocator.set_arena_size(page_size);

  for (std::uint32_t allocs = 0; allocs < 10000; ++allocs)
  {
    if ((xorshift(seed) & 0x1) || mgr.allocs.empty())
    {
      auto size  = xorshift(seed) % page_size;
      auto alloc = allocator.allocate(size, mgr);
      mgr.allocs.emplace_back(alloc.id, alloc.arena, alloc.offset, size);
      mgr.fill(mgr.allocs.back());
    }
    else
    {
      std::size_t chosen = xorshift(seed) % mgr.allocs.size();
      auto        handle = mgr.allocs[chosen];
      allocator.deallocate(handle.alloc_id, mgr);
      mgr.allocs.erase(chosen + mgr.allocs.begin());
    }
    allocator.validate_integrity();
  }
}

TEST_CASE("coalescing_arena_allocator dedictated arena tests", "[coalescing_arena_allocator][default]")
{
  acl::coalescing_arena_allocator allocator;
  constexpr uint32_t              page_size = 100;

  allocator.set_arena_size(page_size);
  REQUIRE(allocator.get_arena_size() == page_size);

  allocator.set_arena_size(page_size / 2);
  REQUIRE(allocator.get_arena_size() == page_size);

  alloc_mem_manager mgr;
  auto              block = allocator.allocate(50, mgr);

  REQUIRE(block.offset == 0);

  auto ded_block = allocator.allocate(10, mgr, {}, std::true_type{});
  REQUIRE(ded_block.arena.id == 2);

  REQUIRE(mgr.arena_count == 2);

  allocator.deallocate(ded_block.id, mgr);

  REQUIRE(mgr.arena_count == 1);
}

TEST_CASE("coalescing_allocator without memory manager", "[coalescing_allocator][default]")
{
  acl::coalescing_allocator allocator;
  auto                      offset = allocator.allocate(256);
  REQUIRE(offset == 0);
  auto noffset = allocator.allocate(256);
  REQUIRE(offset + 256 == noffset);
  allocator.deallocate(0, 256);
  auto toffset = allocator.allocate(256);
  REQUIRE(toffset == 0);
  auto soffset = allocator.allocate(256);
  auto uoffset = allocator.allocate(16);
  auto voffset = allocator.allocate(60);
  auto woffset = allocator.allocate(160);
  REQUIRE(voffset + 60 == woffset);
  allocator.deallocate(uoffset, 16);
  allocator.deallocate(soffset, 250);
  allocator.deallocate(soffset + 250, 6);
  allocator.deallocate(voffset, 60);
  auto xoffset = allocator.allocate(256 + 16 + 60);
  REQUIRE(xoffset == soffset);
}
