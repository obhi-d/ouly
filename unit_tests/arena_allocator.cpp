#include "ouly/allocators/arena_allocator.hpp"
#include "catch2/catch_all.hpp"
#include "ouly/allocators/strat/best_fit_tree.hpp"
#include "ouly/allocators/strat/best_fit_v0.hpp"
#include "ouly/allocators/strat/best_fit_v1.hpp"
#include "ouly/allocators/strat/best_fit_v2.hpp"
#include "ouly/allocators/strat/greedy_v0.hpp"
#include "ouly/allocators/strat/greedy_v1.hpp"
#include <iostream>
#include <random>
#include <unordered_set>

// NOLINTBEGIN
struct alloc_mem_manager
{
  using arena_data_t = ouly::vector<char>;
  struct allocation
  {
    std::uint32_t arena_;
    std::uint32_t alloc_id_;
    std::size_t   offset_;
    std::size_t   size_ = 0;
    allocation()        = default;
    allocation(std::uint32_t iarena, std::uint32_t ialloc, std::size_t ioffset, std::size_t isize)
        : arena_(iarena), alloc_id_(ialloc), offset_(ioffset), size_(isize)
    {}
  };
  ouly::vector<arena_data_t>  arenas_;
  ouly::vector<arena_data_t>  backup_arenas_;
  ouly::vector<allocation>    allocs_;
  ouly::vector<allocation>    backup_allocs_;
  ouly::vector<std::uint32_t> valids_;

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
      arenas_[l.arena_][s + l.offset_] = static_cast<char>(generator(gen));
  }

  std::uint32_t add_arena([[maybe_unused]] std::uint32_t id, [[maybe_unused]] std::size_t size_)
  {
    arena_data_t arena_;
    arena_.resize(size_, 0x17);
    arenas_.emplace_back(std::move(arena_));
    return static_cast<std::uint32_t>(arenas_.size() - 1);
  }

  void remove_arena(std::uint32_t h)
  {
    arenas_[h].clear();
    arenas_[h].shrink_to_fit();
  }

  template <typename Allocator>
  void begin_defragment([[maybe_unused]] Allocator& allocator)
  {
    backup_arenas_ = arenas_;
    backup_allocs_ = allocs_;
  }

  template <typename Allocator>
  void end_defragment([[maybe_unused]] Allocator& allocator)
  {
    // assert validity
    for (std::size_t i = 0; i < allocs_.size(); ++i)
    {
      auto& src = backup_allocs_[i];
      auto& dst = allocs_[i];

      [[maybe_unused]] auto source_data = backup_arenas_[src.arena_].data() + src.offset_;
      [[maybe_unused]] auto dest_data   = arenas_[dst.arena_].data() + dst.offset_;
      OULY_ASSERT(!std::memcmp(source_data, dest_data, src.size_));
    }

    backup_arenas_.clear();
    backup_arenas_.shrink_to_fit();
    backup_allocs_.clear();
    backup_allocs_.shrink_to_fit();
#ifdef OULY_VALIDITY_CHECKS
    allocator.validate_integrity();
#endif
  }

  void rebind_alloc([[maybe_unused]] std::uint32_t halloc, std::uint32_t arena_, std::uint32_t allocid,
                    std::size_t offset_)
  {
    allocs_[halloc].arena_    = arena_;
    allocs_[halloc].alloc_id_ = allocid;
    allocs_[halloc].offset_   = offset_;
  }

  void move_memory([[maybe_unused]] std::uint32_t src_arena, [[maybe_unused]] std::uint32_t dst_arena,
                   [[maybe_unused]] std::size_t from, [[maybe_unused]] std::size_t to, std::size_t size_)
  {
    OULY_ASSERT(arenas_[dst_arena].size() >= to + size_);
    OULY_ASSERT(arenas_[src_arena].size() >= from + size_);
    std::memmove(arenas_[dst_arena].data() + to, arenas_[src_arena].data() + from, size_);
  }
};
template <typename TestType>
void run_test(unsigned int seed)
{
  using allocator_t =
   ouly::arena_allocator<ouly::config<ouly::cfg::strategy<TestType>, ouly::cfg::manager<alloc_mem_manager>,
                                      ouly::cfg::basic_size_type<uint32_t>, ouly::cfg::compute_stats>>;

  // static_assert(allocator_t::can_defragment, "Has defragment");

  static_assert(std::same_as<alloc_mem_manager, typename allocator_t::arena_manager>, "Managers are not equal");
  std::cout << " Seed : " << (std::string_view)ouly::type_name<TestType>() << " : " << seed << std::endl;
  std::minstd_rand                        gen(seed);
  std::bernoulli_distribution             dice(0.7);
  std::bernoulli_distribution             biased_dice(0.05);
  std::uniform_int_distribution<uint32_t> generator(1, 10);
  enum action
  {
    e_allocate,
    e_deallocate
  };
  alloc_mem_manager mgr;
  allocator_t       allocator(256 * 256, mgr);
  for (std::uint32_t allocs_ = 0; allocs_ < 100; ++allocs_)
  {
    if (dice(gen) || mgr.valids_.size() == 0)
    {
      // ouly::fixed_alloc_desc<std::uint32_t, TestType::min_granularity> desc(generator(gen) *
      // TestType::min_granularity,
      // static_cast<std::uint32_t>(mgr.allocs_.size()),
      //                                                                      ouly::alloc_option_bits::f_defrag);
      if (biased_dice(gen))
        allocator.defragment();
      auto huser                     = static_cast<std::uint32_t>(mgr.allocs_.size());
      auto size_                     = generator(gen) * TestType::min_granularity;
      auto [arena_, halloc, offset_] = allocator.allocate(size_, {}, huser);
      mgr.allocs_.emplace_back(arena_, halloc, offset_, size_);
      mgr.fill(mgr.allocs_.back());
      mgr.valids_.push_back(huser);
    }
    else
    {
      std::uniform_int_distribution<std::uint32_t> choose(0, (uint32_t)mgr.valids_.size() - 1);
      std::size_t                                  chosen = choose(gen);
      auto                                         handle = mgr.valids_[chosen];
      allocator.deallocate(mgr.allocs_[handle].alloc_id_);
      mgr.allocs_[handle].size_ = 0;
      mgr.valids_.erase(mgr.valids_.begin() + chosen);
    }
#ifdef OULY_VALIDITY_CHECKS
    allocator.validate_integrity();
#endif
  }
}

TEST_CASE("arena_allocator without memory manager", "[arena_allocator][default]")
{
  ouly::arena_allocator<> allocator(1024);
  auto [loc, offset_] = allocator.allocate(256);
  REQUIRE(offset_ == 0);
  auto [nloc, noffset] = allocator.allocate(256);
  REQUIRE(offset_ + 256 == noffset);
  allocator.deallocate(loc);
  auto [tloc, toffset] = allocator.allocate(256);
  REQUIRE(toffset == 0);
  [[maybe_unused]] auto [sloc, soffset] = allocator.allocate(256);
  auto [uloc, uoffset]                  = allocator.allocate(256);
  auto [vloc, voffset]                  = allocator.allocate(256);
  REQUIRE(vloc == allocator.null());
  auto [wloc, woffset] = allocator.allocate(256);
  REQUIRE(wloc == allocator.null());
  allocator.deallocate(uloc);
  auto [xloc, xoffset] = allocator.allocate(256);
  REQUIRE(xoffset != 0);
}

TEMPLATE_TEST_CASE("Validate arena_allocator", "[arena_allocator.strat]",

                   (ouly::strat::best_fit_v1<ouly::cfg::bsearch_min2>),
                   (ouly::strat::best_fit_v1<ouly::cfg::bsearch_min0>),
                   (ouly::strat::best_fit_v1<ouly::cfg::bsearch_min1>),
                   (ouly::strat::best_fit_v2<ouly::cfg::bsearch_min0>),
                   (ouly::strat::best_fit_v2<ouly::cfg::bsearch_min1>),
                   (ouly::strat::best_fit_v2<ouly::cfg::bsearch_min2>), (ouly::strat::greedy_v1<>),
                   (ouly::strat::greedy_v0<>), (ouly::strat::best_fit_tree<>), (ouly::strat::best_fit_v0<>)

)
{
  std::random_device rd;
  run_test<TestType>(rd());
}

TEMPLATE_TEST_CASE("Validate arena_allocator : 1542249547 init bug", "[arena_allocator.strat]",

                   (ouly::strat::best_fit_v1<ouly::cfg::bsearch_min2>),
                   (ouly::strat::best_fit_v1<ouly::cfg::bsearch_min0>),
                   (ouly::strat::best_fit_v1<ouly::cfg::bsearch_min1>),
                   (ouly::strat::best_fit_v2<ouly::cfg::bsearch_min0>),
                   (ouly::strat::best_fit_v2<ouly::cfg::bsearch_min1>),
                   (ouly::strat::best_fit_v2<ouly::cfg::bsearch_min2>), (ouly::strat::greedy_v1<>),
                   (ouly::strat::greedy_v0<>), (ouly::strat::best_fit_tree<>), (ouly::strat::best_fit_v0<>)

)
{
  run_test<TestType>(1542249547);
}

// NOLINTEND
