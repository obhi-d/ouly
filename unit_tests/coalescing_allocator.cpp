#include "ouly/allocators/coalescing_allocator.hpp"
#include "catch2/catch_all.hpp"
#include "ouly/allocators/coalescing_arena_allocator.hpp"
#include <cstring>
#include <iostream>
#include <limits>
#include <random>
#include <unordered_map>
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
  auto                       first = allocator.allocate(256);
  REQUIRE(allocator.get_offset(first) == 0);
  auto second = allocator.allocate(256);
  REQUIRE(allocator.get_offset(second) == 256);
  allocator.deallocate(first);
  auto third = allocator.allocate(256);
  REQUIRE(allocator.get_offset(third) == 0);
  auto s = allocator.allocate(256);
  auto u = allocator.allocate(16);
  auto v = allocator.allocate(60);
  auto w = allocator.allocate(160);
  REQUIRE(allocator.get_offset(v) + 60 == allocator.get_offset(w));
  allocator.deallocate(u);
  allocator.deallocate(s);
  allocator.deallocate(v);
  allocator.validate_integrity();
  // the three neighbours coalesced into one gap that fits exactly
  auto x = allocator.allocate(256 + 16 + 60);
  REQUIRE(allocator.get_offset(x) == 512);
  auto aligned = allocator.allocate(100, 4096);
  REQUIRE(allocator.get_offset(aligned) % 4096 == 0);
  allocator.validate_integrity();
}

namespace
{

// Keeps a growable backing buffer, fills every allocation with a deterministic pattern and
// verifies the pattern survives compaction.
struct compact_mem
{
  using size_type = ouly::coalescing_allocator::size_type;

  std::vector<char>                       buffer_;
  std::unordered_map<uint32_t, size_type> sizes_;

  static char pattern(uint32_t id, size_type i)
  {
    return static_cast<char>((id * 31U + i * 7U + 0x5U) & 0xFFU);
  }

  void track(ouly::coalescing_allocator const& allocator, ouly::allocation_id id, size_type size)
  {
    auto const offset = allocator.get_offset(id);
    if (offset + size > buffer_.size())
      buffer_.resize(offset + size, 0x17);
    for (size_type i = 0; i < size; ++i)
      buffer_[offset + i] = pattern(id.get(), i);
    sizes_[id.get()] = size;
  }

  auto mover()
  {
    return [this](size_type from, size_type to, size_type size)
    {
      REQUIRE(from + size <= buffer_.size());
      REQUIRE(to < from);
      std::memmove(buffer_.data() + to, buffer_.data() + from, size);
    };
  }

  void verify_all(ouly::coalescing_allocator const& allocator) const
  {
    bool ok = true;
    for (auto const& [id, size] : sizes_)
    {
      auto const offset = allocator.get_offset(ouly::allocation_id{id});
      REQUIRE(offset + size <= buffer_.size());
      for (size_type i = 0; i < size; ++i)
        ok &= buffer_[offset + i] == pattern(id, i);
    }
    REQUIRE(ok);
  }
};

} // namespace

TEST_CASE("coalescing_allocator: randomized alloc/free/compact", "[coalescing_allocator][default]")
{
  uint32_t seed = Catch::getSeed();
  std::cout << " Seed : " << seed << std::endl;

  ouly::coalescing_allocator allocator;
  compact_mem                mem;
  std::vector<uint32_t>      live;

  for (uint32_t iter = 0; iter < 2500; ++iter)
  {
    auto const action = xorshift(seed) % 100;
    if (action < 55 || live.empty())
    {
      auto const size      = (xorshift(seed) % 5000) + 1;
      auto const alignment = 1U << (xorshift(seed) % 9);
      auto const id        = allocator.allocate(size, alignment);
      REQUIRE(id != ouly::allocation_id());
      REQUIRE(allocator.get_offset(id) % alignment == 0);
      mem.track(allocator, id, size);
      live.push_back(id.get());
    }
    else if (action < 90)
    {
      auto const idx = xorshift(seed) % live.size();
      auto const id  = live[idx];
      allocator.deallocate(ouly::allocation_id{id});
      mem.sizes_.erase(id);
      live[idx] = live.back();
      live.pop_back();
    }
    else
    {
      bool const budgeted = (xorshift(seed) & 1) != 0;
      auto const budget   = budgeted ? xorshift(seed) % 10000 : std::numeric_limits<uint32_t>::max();
      auto const result   = allocator.compact(mem.mover(), budget);
      REQUIRE(result.bytes_moved_ <= budget);
      if (!budgeted)
        REQUIRE(result.completed_);
      mem.verify_all(allocator);
    }
    allocator.validate_integrity();
  }
  mem.verify_all(allocator);
}

TEST_CASE("coalescing_allocator: compaction closes gaps and merges moves", "[coalescing_allocator][default]")
{
  ouly::coalescing_allocator allocator;
  compact_mem                mem;

  std::vector<ouly::allocation_id> ids;
  for (uint32_t i = 0; i < 4; ++i)
  {
    auto id = allocator.allocate(100);
    mem.track(allocator, id, 100);
    ids.push_back(id);
  }

  // freeing the first allocation leaves one gap; the remaining run moves as a single merged move
  allocator.deallocate(ids[0]);
  mem.sizes_.erase(ids[0].get());
  auto const result = allocator.compact(mem.mover());
  REQUIRE(result.completed_);
  REQUIRE(result.moves_ == 1);
  REQUIRE(result.allocations_moved_ == 3);
  REQUIRE(result.bytes_moved_ == 300);
  for (uint32_t i = 1; i < 4; ++i)
    REQUIRE(allocator.get_offset(ids[i]) == (i - 1) * 100);
  mem.verify_all(allocator);
  allocator.validate_integrity();

  // an already compact layout moves nothing
  auto const noop = allocator.compact(mem.mover());
  REQUIRE(noop.completed_);
  REQUIRE(noop.moves_ == 0);
  REQUIRE(noop.allocations_moved_ == 0);

  // freed space is reused from the front after compaction
  auto again = allocator.allocate(100);
  REQUIRE(allocator.get_offset(again) == 300);
  allocator.validate_integrity();
}

TEST_CASE("coalescing_allocator: budget makes compaction incremental", "[coalescing_allocator][default]")
{
  ouly::coalescing_allocator allocator;
  compact_mem                mem;

  std::vector<ouly::allocation_id> ids;
  for (uint32_t i = 0; i < 24; ++i)
  {
    auto id = allocator.allocate(200);
    mem.track(allocator, id, 200);
    ids.push_back(id);
  }
  for (uint32_t i = 0; i < ids.size(); i += 2)
  {
    allocator.deallocate(ids[i]);
    mem.sizes_.erase(ids[i].get());
  }

  auto const first = allocator.compact(mem.mover(), 400);
  REQUIRE(first.bytes_moved_ <= 400);
  REQUIRE(!first.completed_);
  mem.verify_all(allocator);
  allocator.validate_integrity();

  // repeated budgeted passes eventually converge to a fully compacted state
  for (uint32_t pass = 0; pass < 64; ++pass)
  {
    if (allocator.compact(mem.mover(), 400).completed_)
      break;
    allocator.validate_integrity();
  }
  auto const final_pass = allocator.compact(mem.mover());
  REQUIRE(final_pass.completed_);
  REQUIRE(final_pass.allocations_moved_ == 0);
  mem.verify_all(allocator);

  // fully packed: live allocations occupy [0, live_bytes)
  size_t live_bytes = 0;
  for (auto const& [id, size] : mem.sizes_)
    live_bytes += size;
  for (auto const& [id, size] : mem.sizes_)
    REQUIRE(allocator.get_offset(ouly::allocation_id{id}) + size <= live_bytes);
}
// NOLINTEND