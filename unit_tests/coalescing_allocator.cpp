#include "ouly/allocators/coalescing_allocator.hpp"
#include "catch2/catch_all.hpp"
#include "ouly/allocators/coalescing_arena_allocator.hpp"
#include <iostream>
#include <random>
#include <unordered_set>

// NOLINTBEGIN
struct alloc_mem_manager
{
  using arena_data_t = std::vector<char>;
  struct allocation
  {
    ouly::allocation_id alloc_id_;
    ouly::arena_id      arena_;
    std::size_t         offset_;
    std::size_t         size_ = 0;
    allocation()              = default;
    allocation(ouly::allocation_id id, ouly::arena_id arena_, std::size_t ioffset, std::size_t isize)
        : alloc_id_(id), arena_(arena_), offset_(ioffset), size_(isize)
    {}
  };

  std::vector<arena_data_t> arenas_;
  std::vector<arena_data_t> backup_arenas_;
  std::vector<allocation>   allocs_;
  std::vector<allocation>   backup_allocs_;
  uint32_t                  arena_count_ = 0;

  bool drop_arena([[maybe_unused]] std::uint32_t id)
  {
    arenas_[id].clear();
    return true;
  }

  void fill(allocation const& l)
  {
    std::minstd_rand                   gen;
    std::uniform_int_distribution<int> generator(65, 122);
    for (std::size_t s = 0; s < l.size_; ++s)
      arenas_[l.arena_.get()][s + l.offset_] = static_cast<char>(generator(gen));
  }

  void add([[maybe_unused]] ouly::arena_id id, [[maybe_unused]] uint32_t size_)
  {
    arena_data_t arena_;
    arena_.resize(size_, 0x17);
    if (id.get() >= arenas_.size())
      arenas_.resize(id.get() + 1);

    arenas_[id.get()] = std::move(arena_);
    arena_count_++;
  }

  void remove(ouly::arena_id h)
  {
    arenas_[h.get()].clear();
    arenas_[h.get()].shrink_to_fit();
    arena_count_--;
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
  constexpr uint32_t               page_size = 10000;
  alloc_mem_manager                mgr;
  ouly::coalescing_arena_allocator allocator;
  allocator.set_arena_size(page_size);

  for (std::uint32_t allocs_ = 0; allocs_ < 10000; ++allocs_)
  {
    if ((xorshift(seed) & 0x1) || mgr.allocs_.empty())
    {
      auto size_ = xorshift(seed) % page_size;
      auto alloc = allocator.allocate(size_, mgr);
      mgr.allocs_.emplace_back(alloc.get_allocation_id(), alloc.get_arena_id(), alloc.get_offset(), size_);
      mgr.fill(mgr.allocs_.back());
    }
    else
    {
      std::size_t chosen = xorshift(seed) % mgr.allocs_.size();
      auto        handle = mgr.allocs_[chosen];
      allocator.deallocate(handle.alloc_id_, mgr);
      mgr.allocs_.erase(chosen + mgr.allocs_.begin());
    }
    allocator.validate_integrity();
  }
}

TEST_CASE("coalescing_arena_allocator dedictated arena_ tests", "[coalescing_arena_allocator][default]")
{
  ouly::coalescing_arena_allocator allocator;
  constexpr uint32_t               page_size = 100;

  allocator.set_arena_size(page_size);
  REQUIRE(allocator.get_arena_size() == page_size);

  allocator.set_arena_size(page_size / 2);
  REQUIRE(allocator.get_arena_size() == page_size);

  alloc_mem_manager mgr;
  auto              block = allocator.allocate(50, mgr);

  REQUIRE(block.get_offset() == 0);

  auto ded_block = allocator.allocate(10, mgr, {}, std::true_type{});
  REQUIRE(ded_block.get_arena_id().get() == 2);

  REQUIRE(mgr.arena_count_ == 2);

  allocator.deallocate(ded_block.get_allocation_id(), mgr);

  REQUIRE(mgr.arena_count_ == 1);
}

TEST_CASE("coalescing_allocator without memory manager", "[coalescing_allocator][default]")
{
  ouly::coalescing_allocator allocator;
  auto                       offset_ = allocator.allocate(256);
  REQUIRE(offset_ == 0);
  auto noffset = allocator.allocate(256);
  REQUIRE(offset_ + 256 == noffset);
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
// NOLINTEND