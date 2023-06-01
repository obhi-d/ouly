#include <acl/allocators/arena_allocator.hpp>
#include <catch2/catch_all.hpp>
#include <iostream>
#include <unordered_set>

struct alloc_mem_manager
{
  using arena_data_t = acl::vector<char>;
  using alloc_info   = acl::alloc_info<uint32_t>;
  struct allocation
  {
    alloc_info  info;
    std::size_t size = 0;
    allocation()     = default;
    allocation(alloc_info const& iinfo, std::size_t isize) : info(iinfo), size(isize) {}
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
      arenas[l.info.harena][s + l.info.offset] = static_cast<char>(generator(gen));
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

      auto source_data = backup_arenas[src.info.harena].data() + src.info.offset;
      auto dest_data   = arenas[dst.info.harena].data() + dst.info.offset;
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

  void rebind_alloc([[maybe_unused]] acl::uhandle halloc, alloc_info info)
  {
    allocs[halloc].info = info;
  }

  void move_memory([[maybe_unused]] acl::uhandle src_arena, [[maybe_unused]] acl::uhandle dst_arena,
                   [[maybe_unused]] std::size_t from, [[maybe_unused]] std::size_t to, std::size_t size)
  {
    ACL_ASSERT(arenas[dst_arena].size() >= to + size);
    ACL_ASSERT(arenas[src_arena].size() >= from + size);
    std::memmove(arenas[dst_arena].data() + to, arenas[src_arena].data() + from, size);
  }
};

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
  using allocator_t =
    acl::arena_allocator<acl::options<acl::opt::strategy<TestType>, acl::opt::manager<alloc_mem_manager>,
                                      acl::opt::basic_size_type<uint32_t>, acl::opt::compute_stats>>;

  std::random_device rd;
  unsigned int       seed = rd();
  std::cout << " Seed : " << acl::type_name<TestType>() << " : " << seed << std::endl;
  std::minstd_rand                        gen(seed);
  std::bernoulli_distribution             dice(0.7);
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
      acl::fixed_alloc_desc<std::uint32_t, TestType::min_granularity> desc(generator(gen) * TestType::min_granularity,
                                                                           static_cast<acl::uhandle>(mgr.allocs.size()),
                                                                           acl::alloc_option_bits::f_defrag);
      auto                                                            info = allocator.allocate(desc);
      mgr.allocs.emplace_back(info, desc.size());
      mgr.fill(mgr.allocs.back());
      mgr.valids.push_back(desc.huser());
    }
    else
    {
      std::uniform_int_distribution<std::uint32_t> choose(0, (uint32_t)mgr.valids.size() - 1);
      std::size_t                                  chosen = choose(gen);
      auto                                         handle = mgr.valids[chosen];
      allocator.deallocate(mgr.allocs[handle].info.halloc);
      mgr.allocs[handle].size = 0;
      mgr.valids.erase(mgr.valids.begin() + chosen);
    }
#ifdef ACL_VALIDITY_CHECKS
    allocator.validate_integrity();
#endif
  }
}

TEMPLATE_TEST_CASE("Validate arena_allocator seed1542249547 init bug", "[arena_allocator.strat]",
                   // clang-format off
  (acl::strat::slotted_v2<acl::opt::fallback_start<acl::strat::best_fit_tree<>>>)
                   // clang-format on
)
{
  using allocator_t =
    acl::arena_allocator<acl::options<acl::opt::strategy<TestType>, acl::opt::manager<alloc_mem_manager>,
                                      acl::opt::basic_size_type<uint32_t>, acl::opt::compute_stats>>;

  std::random_device rd;
  unsigned int       seed = 1542249547;
  // rd();
  std::cout << " Seed : " << acl::type_name<TestType>() << " : " << seed << std::endl;
  std::minstd_rand                        gen(seed);
  std::bernoulli_distribution             dice(0.7);
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
      acl::fixed_alloc_desc<std::uint32_t, TestType::min_granularity> desc(generator(gen) * TestType::min_granularity,
                                                                           static_cast<acl::uhandle>(mgr.allocs.size()),
                                                                           acl::alloc_option_bits::f_defrag);
      auto                                                            info = allocator.allocate(desc);
      mgr.allocs.emplace_back(info, desc.size());
      mgr.fill(mgr.allocs.back());
      mgr.valids.push_back(desc.huser());
    }
    else
    {
      std::uniform_int_distribution<std::uint32_t> choose(0, (uint32_t)mgr.valids.size() - 1);
      std::size_t                                  chosen = choose(gen);
      auto                                         handle = mgr.valids[chosen];
      allocator.deallocate(mgr.allocs[handle].info.halloc);
      mgr.allocs[handle].size = 0;
      mgr.valids.erase(mgr.valids.begin() + chosen);
    }
#ifdef ACL_VALIDITY_CHECKS
    allocator.validate_integrity();
#endif
  }
}
