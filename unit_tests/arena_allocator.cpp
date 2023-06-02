#include <acl/allocators/arena_allocator.hpp>
#include <catch2/catch_all.hpp>
#include <iostream>
#include <unordered_set>

struct alloc_mem_manager
{
  using arena_data_t = acl::vector<char>;
  struct allocation
  {
    acl::uhandle arena;
    acl::uhandle alloc_id;
    std::size_t  offset;
    std::size_t  size = 0;
    allocation()      = default;
    allocation(acl::uhandle iarena, acl::uhandle ialloc, std::size_t ioffset, std::size_t isize)
        : arena(iarena), alloc_id(ialloc), offset(ioffset), size(isize)
    {}
  };
  acl::vector<arena_data_t> arenas;
  acl::vector<arena_data_t> backup_arenas;
  acl::vector<allocation>   allocs;
  acl::vector<allocation>   backup_allocs;
  acl::vector<acl::uhandle> valids;

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
      arenas[l.arena][s + l.offset] = static_cast<char>(generator(gen));
  }

  acl::uhandle add_arena([[maybe_unused]] acl::ihandle id, [[maybe_unused]] std::size_t size)
  {
    arena_data_t arena;
    arena.resize(size, 0x17);
    arenas.emplace_back(std::move(arena));
    return static_cast<acl::uhandle>(arenas.size() - 1);
  }

  void remove_arena(acl::uhandle h)
  {
    arenas[h].clear();
    arenas[h].shrink_to_fit();
  }

  template <typename Allocator>
  void begin_defragment(Allocator& allocator)
  {
    backup_arenas = arenas;
    backup_allocs = allocs;
  }

  template <typename Allocator>
  void end_defragment(Allocator& allocator)
  {
    // ACL_ASSERT validity
    for (std::size_t i = 0; i < allocs.size(); ++i)
    {
      auto& src = backup_allocs[i];
      auto& dst = allocs[i];

      auto source_data = backup_arenas[src.arena].data() + src.offset;
      auto dest_data   = arenas[dst.arena].data() + dst.offset;
      ACL_ASSERT(!std::memcmp(source_data, dest_data, src.size));
    }

    backup_arenas.clear();
    backup_arenas.shrink_to_fit();
    backup_allocs.clear();
    backup_allocs.shrink_to_fit();
#ifdef ACL_VALIDITY_CHECKS
    allocator.validate_integrity();
#endif
  }

  void rebind_alloc([[maybe_unused]] acl::uhandle halloc, acl::uhandle arena, acl::uhandle allocid, std::size_t offset)
  {
    allocs[halloc].arena    = arena;
    allocs[halloc].alloc_id = allocid;
    allocs[halloc].offset   = offset;
  }

  void move_memory([[maybe_unused]] acl::uhandle src_arena, [[maybe_unused]] acl::uhandle dst_arena,
                   [[maybe_unused]] std::size_t from, [[maybe_unused]] std::size_t to, std::size_t size)
  {
    ACL_ASSERT(arenas[dst_arena].size() >= to + size);
    ACL_ASSERT(arenas[src_arena].size() >= from + size);
    std::memmove(arenas[dst_arena].data() + to, arenas[src_arena].data() + from, size);
  }
};
template <typename TestType>
void run_test(unsigned int seed)
{
  using allocator_t =
    acl::arena_allocator<acl::options<acl::opt::strategy<TestType>, acl::opt::manager<alloc_mem_manager>,
                                      acl::opt::basic_size_type<uint32_t>, acl::opt::compute_stats>>;

  // static_assert(allocator_t::can_defragment, "Has defragment");

  static_assert(std::same_as<alloc_mem_manager, typename allocator_t::arena_manager>, "Managers are not equal");
  std::cout << " Seed : " << acl::type_name<TestType>() << " : " << seed << std::endl;
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
  for (std::uint32_t allocs = 0; allocs < 10000; ++allocs)
  {
    if (dice(gen) || mgr.valids.size() == 0)
    {
      // acl::fixed_alloc_desc<std::uint32_t, TestType::min_granularity> desc(generator(gen) *
      // TestType::min_granularity,
      //                                                                      static_cast<acl::uhandle>(mgr.allocs.size()),
      //                                                                      acl::alloc_option_bits::f_defrag);
      if (biased_dice(gen))
        allocator.defragment();
      auto huser                   = static_cast<acl::uhandle>(mgr.allocs.size());
      auto size                    = generator(gen) * TestType::min_granularity;
      auto [arena, halloc, offset] = allocator.allocate(size, {}, huser);
      mgr.allocs.emplace_back(arena, halloc, offset, size);
      mgr.fill(mgr.allocs.back());
      mgr.valids.push_back(huser);
    }
    else
    {
      std::uniform_int_distribution<std::uint32_t> choose(0, (uint32_t)mgr.valids.size() - 1);
      std::size_t                                  chosen = choose(gen);
      auto                                         handle = mgr.valids[chosen];
      allocator.deallocate(mgr.allocs[handle].alloc_id);
      mgr.allocs[handle].size = 0;
      mgr.valids.erase(mgr.valids.begin() + chosen);
    }
#ifdef ACL_VALIDITY_CHECKS
    allocator.validate_integrity();
#endif
  }
}

TEST_CASE("arena_allocator without memory manager", "[arena_allocator][default]")
{
  acl::arena_allocator<> allocator(1024);
  auto [loc, offset] = allocator.allocate(256);
  REQUIRE(offset == 0);
  auto [nloc, noffset] = allocator.allocate(256);
  REQUIRE(offset + 256 == noffset);
  allocator.deallocate(loc);
  auto [tloc, toffset] = allocator.allocate(256);
  REQUIRE(toffset == 0);
  auto [sloc, soffset] = allocator.allocate(256);
  auto [uloc, uoffset] = allocator.allocate(256);
  auto [vloc, voffset] = allocator.allocate(256);
  REQUIRE(vloc == allocator.null());
  auto [wloc, woffset] = allocator.allocate(256);
  REQUIRE(wloc == allocator.null());
  allocator.deallocate(uloc);
  auto [xloc, xoffset] = allocator.allocate(256);
  REQUIRE(xoffset != 0);
}

TEMPLATE_TEST_CASE("Validate arena_allocator", "[arena_allocator.strat]",
                   // clang-format off

    (acl::strat::best_fit_v1<acl::opt::bsearch_min2>),
    (acl::strat::best_fit_v1<acl::opt::bsearch_min0>),
    (acl::strat::best_fit_v1<acl::opt::bsearch_min1>),
    (acl::strat::best_fit_v2<acl::opt::bsearch_min0>),
    (acl::strat::best_fit_v2<acl::opt::bsearch_min1>),
    (acl::strat::best_fit_v2<acl::opt::bsearch_min2>),
    (acl::strat::greedy_v1<>),
    (acl::strat::greedy_v0<>),
    (acl::strat::best_fit_tree<>),
    (acl::strat::best_fit_v0<>),
    (acl::strat::slotted_v0<>),
    (acl::strat::slotted_v1<>),
    (acl::strat::slotted_v2<>),
    (acl::strat::slotted_v0<acl::opt::fallback_start<acl::strat::best_fit_tree<>>>),
    (acl::strat::slotted_v1<acl::opt::fallback_start<acl::strat::best_fit_tree<>>>),
    (acl::strat::slotted_v2<acl::opt::fallback_start<acl::strat::best_fit_tree<>>>)
                   // clang-format on
)
{
  std::random_device rd;
  run_test<TestType>(rd());
}

TEMPLATE_TEST_CASE("Validate arena_allocator : 1542249547 init bug", "[arena_allocator.strat]",
                   // clang-format off

    (acl::strat::best_fit_v1<acl::opt::bsearch_min2>),
    (acl::strat::best_fit_v1<acl::opt::bsearch_min0>),
    (acl::strat::best_fit_v1<acl::opt::bsearch_min1>),
    (acl::strat::best_fit_v2<acl::opt::bsearch_min0>),
    (acl::strat::best_fit_v2<acl::opt::bsearch_min1>),
    (acl::strat::best_fit_v2<acl::opt::bsearch_min2>),
    (acl::strat::greedy_v1<>),
    (acl::strat::greedy_v0<>),
    (acl::strat::best_fit_tree<>),
    (acl::strat::best_fit_v0<>),
    (acl::strat::slotted_v0<>),
    (acl::strat::slotted_v1<>),
    (acl::strat::slotted_v2<>),
    (acl::strat::slotted_v0<acl::opt::fallback_start<acl::strat::best_fit_tree<>>>),
    (acl::strat::slotted_v1<acl::opt::fallback_start<acl::strat::best_fit_tree<>>>),
    (acl::strat::slotted_v2<acl::opt::fallback_start<acl::strat::best_fit_tree<>>>)
                   // clang-format on
)
{
  run_test<TestType>(1542249547);
}
