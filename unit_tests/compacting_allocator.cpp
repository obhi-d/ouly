#include "ouly/allocators/compacting_allocator.hpp"
#include "catch2/catch_all.hpp"
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

// Keeps a growable backing buffer, fills every allocation with a deterministic pattern and
// verifies the pattern survives compaction.
struct compact_mem
{
  using size_type = ouly::compacting_allocator::size_type;

  std::vector<char>                       buffer_;
  std::unordered_map<uint32_t, size_type> sizes_;

  static char pattern(uint32_t id, size_type i)
  {
    return static_cast<char>((id * 31U + i * 7U + 0x5U) & 0xFFU);
  }

  void track(ouly::compacting_allocator const& allocator, ouly::allocation_id id, size_type size)
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

  void verify_all(ouly::compacting_allocator const& allocator) const
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

TEST_CASE("compacting_allocator: randomized alloc/free/compact", "[compacting_allocator][default]")
{
  uint32_t seed = Catch::getSeed();
  std::cout << " Seed : " << seed << std::endl;

  ouly::compacting_allocator allocator;
  compact_mem                mem;
  std::vector<uint32_t>      live;

  for (uint32_t iter = 0; iter < 2500; ++iter)
  {
    auto const action = xorshift(seed) % 100;
    if (action < 55 || live.empty())
    {
      auto const size = (xorshift(seed) % 5000) + 1;
      auto const id   = allocator.allocate(size);
      REQUIRE(id != ouly::allocation_id());
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

TEST_CASE("compacting_allocator: compaction closes gaps and merges moves", "[compacting_allocator][default]")
{
  ouly::compacting_allocator allocator;
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

TEST_CASE("compacting_allocator: budget makes compaction incremental", "[compacting_allocator][default]")
{
  ouly::compacting_allocator allocator;
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
