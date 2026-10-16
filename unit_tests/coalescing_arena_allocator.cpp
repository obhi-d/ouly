#include "ouly/allocators/coalescing_arena_allocator.hpp"
#include "catch2/catch_all.hpp"
#include "ouly/allocators/linear_arena_allocator.hpp"
#include "ouly/allocators/ts_thread_local_allocator.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>
#include <vector>

// NOLINTBEGIN
namespace
{

struct arena_memory
{
  std::vector<std::vector<uint8_t>> arenas_;
  std::vector<uint32_t>             memory_classes_;
  uint32_t                          active_ = 0;

  void add(ouly::arena_id arena, ouly::allocation_size_type size, uint32_t memory_class)
  {
    if (arenas_.size() <= arena.get())
    {
      arenas_.resize(static_cast<size_t>(arena.get()) + 1);
      memory_classes_.resize(static_cast<size_t>(arena.get()) + 1);
    }
    arenas_[arena.get()].assign(size, 0);
    memory_classes_[arena.get()] = memory_class;
    ++active_;
  }

  void remove(ouly::arena_id arena)
  {
    REQUIRE(!arenas_[arena.get()].empty());
    arenas_[arena.get()].clear();
    --active_;
  }
};

struct test_executor
{
  arena_memory*                    memory_        = nullptr;
  ouly::ca_timeline_value          completed_     = 0;
  ouly::ca_timeline_value          next_timeline_ = 1;
  bool                             fail_schedule_ = false;
  bool                             fail_switch_   = false;
  std::vector<ouly::ca_relocation> copies_        = {};
  std::vector<ouly::ca_relocation> switches_      = {};

  auto schedule_copy(ouly::ca_relocation const& relocation) -> std::optional<ouly::ca_timeline_value>
  {
    if (fail_schedule_)
    {
      return std::nullopt;
    }
    REQUIRE(relocation.source_arena_ != relocation.destination_arena_);
    auto& source      = memory_->arenas_[relocation.source_arena_.get()];
    auto& destination = memory_->arenas_[relocation.destination_arena_.get()];
    REQUIRE(relocation.source_offset_ + relocation.size_ <= source.size());
    REQUIRE(relocation.destination_offset_ + relocation.size_ <= destination.size());
    std::memcpy(destination.data() + relocation.destination_offset_, source.data() + relocation.source_offset_,
                relocation.size_);
    copies_.push_back(relocation);
    return next_timeline_++;
  }

  auto switch_references(ouly::ca_relocation const& relocation) -> std::optional<ouly::ca_timeline_value>
  {
    if (fail_switch_)
    {
      return std::nullopt;
    }
    switches_.push_back(relocation);
    return next_timeline_++;
  }

  [[nodiscard]] auto is_complete(ouly::ca_timeline_value value) const -> bool
  {
    return value <= completed_;
  }
};

auto pattern(uint32_t id, uint32_t index) -> uint8_t
{
  return static_cast<uint8_t>((id * 37U + index * 11U + 3U) & 0xffU);
}

void fill(arena_memory& memory, ouly::ca_allocation const& allocation, uint32_t size)
{
  auto& data = memory.arenas_[allocation.get_arena_id().get()];
  for (uint32_t index = 0; index < size; ++index)
  {
    data[allocation.get_offset() + index] = pattern(allocation.get_allocation_id().get(), index);
  }
}

void verify(arena_memory const& memory, ouly::coalescing_arena_allocator const& allocator, ouly::allocation_id id)
{
  auto const arena  = allocator.get_arena(id).get();
  auto const offset = allocator.get_offset(id);
  auto const size   = allocator.get_size(id);
  REQUIRE(offset + size <= memory.arenas_[arena].size());
  for (uint32_t index = 0; index < size; ++index)
  {
    REQUIRE(memory.arenas_[arena][offset + index] == pattern(id.get(), index));
  }
}

static_assert(sizeof(ouly::allocation_id) == sizeof(uint32_t));
static_assert(sizeof(ouly::ca_move_id) == sizeof(ouly::allocation_id));
static_assert(sizeof(ouly::arena_id) == sizeof(uint16_t));
static_assert(std::same_as<ouly::coalescing_arena_allocator::size_type, ouly::allocation_size_type>);
static_assert(ouly::CoalescingMemoryManager<arena_memory>);
static_assert(ouly::CoalescingRelocationExecutor<test_executor>);

} // namespace

TEST_CASE("coalescing_arena_allocator: allocation follows ouly id, alignment, and class conventions",
          "[coalescing_arena_allocator][default]")
{
  arena_memory                     memory;
  ouly::coalescing_arena_allocator allocator{1024};

  auto odd     = allocator.allocate(13, memory);
  auto aligned = allocator.allocate(
   100, memory, ouly::ca_allocation_options{.alignment_ = 256, .memory_class_ = 7, .resource_class_ = 9});
  auto same_class = allocator.allocate(64, memory, ouly::ca_allocation_options{.memory_class_ = 7});
  auto dedicated  = allocator.allocate(1024, memory);

  REQUIRE(odd.get_allocation_id() != ouly::allocation_id());
  REQUIRE(aligned.get_offset() % 256 == 0);
  REQUIRE(allocator.get_alignment(aligned.get_allocation_id()) == 256);
  REQUIRE(allocator.get_memory_class(aligned.get_allocation_id()) == 7);
  REQUIRE(allocator.get_resource_class(aligned.get_allocation_id()) == 9);
  REQUIRE(aligned.get_arena_id() == same_class.get_arena_id());
  REQUIRE(aligned.get_arena_id() != odd.get_arena_id());
  REQUIRE(!allocator.is_movable(dedicated.get_allocation_id()));
  allocator.validate_integrity();

  allocator.deallocate(odd.get_allocation_id(), memory);
  allocator.deallocate(aligned.get_allocation_id(), memory);
  allocator.deallocate(same_class.get_allocation_id(), memory);
  allocator.deallocate(dedicated.get_allocation_id(), memory);
  REQUIRE(memory.active_ == 0);
}

TEST_CASE("coalescing_arena_allocator: evacuation is budgeted, persistent, and retired asynchronously",
          "[coalescing_arena_allocator][default]")
{
  constexpr uint32_t block_size = 1024;
  constexpr uint32_t alloc_size = 256;

  arena_memory                     memory;
  test_executor                    executor{.memory_ = &memory, .copies_ = {}};
  ouly::coalescing_arena_allocator allocator{block_size};
  std::vector<ouly::ca_allocation> allocations;

  for (uint32_t index = 0; index < 8; ++index)
  {
    allocations.push_back(allocator.allocate(alloc_size, memory));
    fill(memory, allocations.back(), alloc_size);
  }
  REQUIRE(memory.active_ == 2);

  for (auto index : {0U, 2U, 4U, 6U})
  {
    allocator.deallocate(allocations[index].get_allocation_id(), memory);
  }

  auto first =
   allocator.defragment(memory, executor, ouly::ca_defrag_budget{.max_copy_bytes_ = alloc_size, .max_move_count_ = 1});
  REQUIRE(first.bytes_scheduled_ == alloc_size);
  REQUIRE(first.moves_scheduled_ == 1);
  REQUIRE(first.budget_exhausted_);
  REQUIRE(first.evacuation_active_);
  REQUIRE(!first.completed_);
  REQUIRE(executor.copies_.size() == 1);

  auto const moving_id     = executor.copies_.front().allocation_;
  auto const source_arena  = executor.copies_.front().source_arena_;
  auto const original_slot = allocator.get_offset(moving_id);
  REQUIRE(allocator.get_arena(moving_id) == source_arena);
  REQUIRE(allocator.get_offset(moving_id) == original_slot);
  REQUIRE(!allocator.try_deallocate(moving_id, memory));

  // The source is frozen and every destination was reserved, so foreground allocation cannot refill it.
  auto foreground = allocator.allocate(128, memory);
  REQUIRE(foreground.get_arena_id() != source_arena);
  REQUIRE(memory.active_ == 3);

  executor.completed_ = 1;
  auto copied         = allocator.process_completions(memory, executor);
  REQUIRE(copied.bytes_copied_ == alloc_size);
  REQUIRE(copied.moves_completed_ == 0);
  REQUIRE(allocator.get_arena(moving_id) == executor.copies_.front().destination_arena_);
  verify(memory, allocator, moving_id);

  executor.completed_ = 2;
  auto retired        = allocator.process_completions(memory, executor);
  REQUIRE(retired.moves_completed_ == 1);
  REQUIRE(retired.arenas_removed_ == 0);

  auto second =
   allocator.defragment(memory, executor, ouly::ca_defrag_budget{.max_copy_bytes_ = alloc_size, .max_move_count_ = 1});
  REQUIRE(second.moves_scheduled_ == 1);
  REQUIRE(executor.copies_.size() == 2);

  executor.completed_ = 3;
  allocator.process_completions(memory, executor);
  executor.completed_ = 4;
  auto complete       = allocator.process_completions(memory, executor);
  REQUIRE(complete.completed_);
  REQUIRE(!complete.evacuation_active_);
  REQUIRE(complete.arenas_removed_ == 1);
  REQUIRE(complete.bytes_released_ == block_size);
  REQUIRE(memory.active_ == 2);
  REQUIRE(memory.arenas_[source_arena.get()].empty());

  for (auto index : {1U, 3U, 5U, 7U})
  {
    verify(memory, allocator, allocations[index].get_allocation_id());
  }
  allocator.validate_integrity();

  auto const stats = allocator.statistics();
  REQUIRE(stats.bytes_scheduled_ == 2 * alloc_size);
  REQUIRE(stats.bytes_copied_ == 2 * alloc_size);
  REQUIRE(stats.bytes_released_ == block_size);
  REQUIRE(stats.blocks_released_ == 1);
  REQUIRE(stats.moves_completed_ == 2);
  REQUIRE(stats.active_evacuation_targets_ == 0);

  for (auto index : {1U, 3U, 5U, 7U})
  {
    allocator.deallocate(allocations[index].get_allocation_id(), memory);
  }
  allocator.deallocate(foreground.get_allocation_id(), memory);
  REQUIRE(memory.active_ == 0);
}

TEST_CASE("coalescing_arena_allocator: pinned allocations and memory classes prevent unsafe evacuation",
          "[coalescing_arena_allocator][default]")
{
  arena_memory                     memory;
  test_executor                    executor{.memory_ = &memory};
  ouly::coalescing_arena_allocator allocator{512};

  auto class0_a = allocator.allocate(256, memory);
  auto class0_b = allocator.allocate(256, memory, ouly::ca_allocation_options{.movable_ = false});
  auto class1_a = allocator.allocate(256, memory, ouly::ca_allocation_options{.memory_class_ = 1});
  auto class1_b = allocator.allocate(256, memory, ouly::ca_allocation_options{.memory_class_ = 1});

  allocator.deallocate(class0_a.get_allocation_id(), memory);
  allocator.deallocate(class1_a.get_allocation_id(), memory);
  auto result = allocator.defragment(memory, executor);

  REQUIRE(result.completed_);
  REQUIRE(!result.evacuation_active_);
  REQUIRE(executor.copies_.empty());
  REQUIRE(allocator.get_arena(class0_b.get_allocation_id()) != allocator.get_arena(class1_b.get_allocation_id()));
  allocator.validate_integrity();

  allocator.deallocate(class0_b.get_allocation_id(), memory);
  allocator.deallocate(class1_b.get_allocation_id(), memory);
  REQUIRE(memory.active_ == 0);
}

TEST_CASE("coalescing_arena_allocator: failed scheduling cancels reservations and unfreezes the source",
          "[coalescing_arena_allocator][default]")
{
  arena_memory                     memory;
  test_executor                    executor{.memory_ = &memory, .fail_schedule_ = true};
  ouly::coalescing_arena_allocator allocator{512};
  std::vector<ouly::ca_allocation> allocations;

  for (uint32_t index = 0; index < 4; ++index)
  {
    allocations.push_back(allocator.allocate(256, memory));
  }
  allocator.deallocate(allocations[0].get_allocation_id(), memory);
  allocator.deallocate(allocations[2].get_allocation_id(), memory);

  auto result = allocator.defragment(memory, executor);
  REQUIRE(result.completed_);
  REQUIRE(!result.evacuation_active_);
  REQUIRE(executor.copies_.empty());
  REQUIRE(memory.active_ == 2);
  allocator.validate_integrity();

  auto replacement = allocator.allocate(128, memory);
  REQUIRE(memory.active_ == 2);
  allocator.validate_integrity();

  allocator.deallocate(allocations[1].get_allocation_id(), memory);
  allocator.deallocate(allocations[3].get_allocation_id(), memory);
  allocator.deallocate(replacement.get_allocation_id(), memory);
  REQUIRE(memory.active_ == 0);
}

TEST_CASE("coalescing_arena_allocator: failed rebinding preserves sources and cancels the evacuation",
          "[coalescing_arena_allocator][default]")
{
  arena_memory                     memory;
  test_executor                    executor{.memory_ = &memory, .fail_switch_ = true};
  ouly::coalescing_arena_allocator allocator{512};
  std::vector<ouly::ca_allocation> allocations;

  for (uint32_t index = 0; index < 4; ++index)
  {
    allocations.push_back(allocator.allocate(256, memory));
    fill(memory, allocations.back(), 256);
  }
  allocator.deallocate(allocations[0].get_allocation_id(), memory);
  allocator.deallocate(allocations[2].get_allocation_id(), memory);

  auto scheduled =
   allocator.defragment(memory, executor, ouly::ca_defrag_budget{.max_copy_bytes_ = 256, .max_move_count_ = 1});
  REQUIRE(scheduled.moves_scheduled_ == 1);
  auto const moving_id    = executor.copies_.front().allocation_;
  auto const source_arena = executor.copies_.front().source_arena_;

  executor.completed_ = 1;
  auto failed         = allocator.process_completions(memory, executor);
  REQUIRE(failed.bytes_copied_ == 256);
  REQUIRE(failed.moves_completed_ == 0);
  REQUIRE(failed.completed_);
  REQUIRE(allocator.get_arena(moving_id) == source_arena);
  verify(memory, allocator, moving_id);
  REQUIRE(memory.active_ == 2);
  allocator.validate_integrity();

  auto replacement = allocator.allocate(128, memory);
  REQUIRE(memory.active_ == 2);

  allocator.deallocate(allocations[1].get_allocation_id(), memory);
  allocator.deallocate(allocations[3].get_allocation_id(), memory);
  allocator.deallocate(replacement.get_allocation_id(), memory);
  REQUIRE(memory.active_ == 0);
}

namespace
{
/** @brief Scratch allocator that only exposes the unaligned call shape, and counts its use */
struct counting_scratch
{
  ouly::linear_stack_allocator<> arena_{1024};
  uint32_t                       allocations_ = 0;
  uint32_t                       reallocs_    = 0;

  auto allocate(std::size_t size) -> void*
  {
    ++allocations_;
    return arena_.allocate(size, ouly::alignment<alignof(std::max_align_t)>{});
  }

  auto realloc(void* pointer, std::size_t old_size, std::size_t new_size) -> void*
  {
    ++reallocs_;
    return arena_.realloc(pointer, old_size, new_size, ouly::alignment<alignof(std::max_align_t)>{});
  }
};

static_assert(ouly::ScratchAllocator<counting_scratch>);
static_assert(ouly::ScratchAllocator<ouly::linear_stack_allocator<>>);
static_assert(ouly::ScratchAllocator<ouly::linear_arena_allocator<>>);
static_assert(ouly::ScratchAllocator<ouly::ts_thread_local_allocator>);
} // namespace

TEST_CASE("coalescing_arena_allocator: defragmentation plans from a caller supplied scratch allocator",
          "[coalescing_arena_allocator][default]")
{
  constexpr uint32_t block_size = 1024;
  constexpr uint32_t alloc_size = 256;

  arena_memory                     memory;
  test_executor                    executor{.memory_ = &memory};
  ouly::coalescing_arena_allocator allocator{block_size};
  counting_scratch                 scratch;
  std::vector<ouly::ca_allocation> allocations;

  for (uint32_t index = 0; index < 8; ++index)
  {
    allocations.push_back(allocator.allocate(alloc_size, memory));
    fill(memory, allocations.back(), alloc_size);
  }
  for (auto index : {0U, 2U, 4U, 6U})
  {
    allocator.deallocate(allocations[index].get_allocation_id(), memory);
  }

  // Nothing to plan yet is nothing to allocate
  ouly::ca_defrag_budget const idle{.max_copy_bytes_ = 0, .max_move_count_ = 0};
  REQUIRE(allocator.defragment(memory, executor, idle, scratch).moves_scheduled_ == 0);
  REQUIRE(scratch.allocations_ == 0);

  auto scheduled = allocator.defragment(memory, executor, ouly::ca_defrag_budget{}, scratch);
  REQUIRE(scheduled.moves_scheduled_ == 2);
  REQUIRE(scratch.allocations_ > 0);

  executor.completed_ = executor.next_timeline_;
  allocator.process_completions(memory, executor);
  executor.completed_ = executor.next_timeline_;
  auto complete       = allocator.process_completions(memory, executor);
  REQUIRE(complete.completed_);
  REQUIRE(complete.arenas_removed_ == 1);
  REQUIRE(memory.active_ == 1);
  allocator.validate_integrity();

  for (auto index : {1U, 3U, 5U, 7U})
  {
    verify(memory, allocator, allocations[index].get_allocation_id());
    allocator.deallocate(allocations[index].get_allocation_id(), memory);
  }
  REQUIRE(memory.active_ == 0);
}

TEST_CASE("coalescing_arena_allocator: randomized allocation, evacuation and failure injection",
          "[coalescing_arena_allocator][default]")
{
  uint32_t seed = Catch::getSeed() | 1U;
  auto     next = [&seed]() -> uint32_t
  {
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    return seed;
  };

  constexpr uint32_t block_size = 64 * 1024;

  arena_memory                     memory;
  test_executor                    executor{.memory_ = &memory};
  ouly::coalescing_arena_allocator allocator{block_size};

  struct live_allocation
  {
    ouly::allocation_id id_;
    uint32_t            size_;
    uint32_t            alignment_;
    uint32_t            memory_class_;
  };
  std::vector<live_allocation> live;

  auto const check_all = [&](bool contents)
  {
    allocator.validate_integrity();
    for (auto const& entry : live)
    {
      CHECK(allocator.get_size(entry.id_) == entry.size_);
      CHECK(allocator.get_offset(entry.id_) % entry.alignment_ == 0);
      CHECK(allocator.get_memory_class(entry.id_) == entry.memory_class_);
      if (contents)
      {
        auto const& data = memory.arenas_[allocator.get_arena(entry.id_).get()];
        auto const  base = data.data() + allocator.get_offset(entry.id_);
        bool        same = true;
        for (uint32_t index = 0; index < entry.size_; ++index)
        {
          same = same && base[index] == pattern(entry.id_.get(), index);
        }
        REQUIRE(same);
      }
    }
  };

  for (uint32_t step = 0; step < 3000; ++step)
  {
    auto const action = next() % 100;
    if (action < 45 || live.empty())
    {
      uint32_t const alignments[] = {1, 16, 256, 4096};
      auto const     alignment    = alignments[next() % 4];
      auto const     size         = 1 + (next() % (next() % 8 == 0 ? block_size : block_size / 16));
      auto const     options      = ouly::ca_allocation_options{.alignment_      = alignment,
                                                                .memory_class_   = next() % 2,
                                                                .resource_class_ = next() % 3,
                                                                .movable_        = next() % 16 != 0,
                                                                .dedicated_      = next() % 64 == 0};
      auto const     allocation   = allocator.allocate(size, memory, options);
      REQUIRE(allocation.get_offset() % alignment == 0);
      fill(memory, allocation, size);
      live.push_back({allocation.get_allocation_id(), size, alignment, options.memory_class_});
    }
    else if (action < 80)
    {
      auto const index = next() % live.size();
      if (allocator.try_deallocate(live[index].id_, memory))
      {
        live[index] = live.back();
        live.pop_back();
      }
      else
      {
        REQUIRE(allocator.is_relocating(live[index].id_));
      }
    }
    else if (action < 90)
    {
      executor.fail_schedule_ = next() % 20 == 0;
      executor.fail_switch_   = next() % 20 == 0;
      allocator.defragment(
       memory, executor,
       ouly::ca_defrag_budget{.max_copy_bytes_ = next() % (4 * block_size), .max_move_count_ = 1 + (next() % 8)});
    }
    else
    {
      executor.fail_switch_ = next() % 20 == 0;
      executor.completed_ += next() % 3;
      allocator.process_completions(memory, executor);
    }
    check_all(step % 128 == 0);
  }

  executor.fail_schedule_ = false;
  executor.fail_switch_   = false;
  for (uint32_t round = 0; round < 64; ++round)
  {
    executor.completed_ = executor.next_timeline_;
    allocator.defragment(memory, executor);
  }
  executor.completed_ = executor.next_timeline_;
  allocator.process_completions(memory, executor);
  executor.completed_ = executor.next_timeline_;
  allocator.process_completions(memory, executor);
  check_all(true);
  REQUIRE(allocator.statistics().active_evacuation_targets_ == 0);
  REQUIRE(allocator.statistics().moves_completed_ > 0);
  REQUIRE(allocator.statistics().blocks_released_ > 0);

  for (auto const& entry : live)
  {
    allocator.deallocate(entry.id_, memory);
  }
  allocator.validate_integrity();
  REQUIRE(memory.active_ == 0);
}

// NOLINTEND
