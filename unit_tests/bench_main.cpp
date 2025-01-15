#define ANKERL_NANOBENCH_IMPLEMENT
#include <acl/allocators/arena_allocator.hpp>
#include <acl/allocators/strat/best_fit_tree.hpp>
#include <acl/allocators/strat/best_fit_v0.hpp>
#include <acl/allocators/strat/best_fit_v1.hpp>
#include <acl/allocators/strat/best_fit_v2.hpp>
#include <acl/allocators/strat/greedy_v0.hpp>
#include <acl/allocators/strat/greedy_v1.hpp>
#include <nanobench.h>
#include <string_view>

// NOLINTBEGIN
struct alloc_mem_manager
{

  uint32_t arena_nb = 0;

  alloc_mem_manager() {}

  bool drop_arena([[maybe_unused]] std::uint32_t id)
  {
    return true;
  }

  std::uint32_t add_arena([[maybe_unused]] std::uint32_t id, [[maybe_unused]] std::size_t size)
  {
    return arena_nb++;
  }

  void remove_arena(std::uint32_t h) {}
};

struct rand_device
{
  uint32_t update()
  {
    uint32_t x = seed;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return seed = x;
  }

  uint32_t seed = 2147483647;
};

template <typename T>
void bench_arena(uint32_t size, std::string_view name)
{
  using allocator_t = acl::arena_allocator<
   acl::config<acl::cfg::strategy<T>, acl::cfg::manager<alloc_mem_manager>, acl::cfg::basic_size_type<uint32_t>>>;
  constexpr uint32_t         nbatch = 200000;
  alloc_mem_manager          mgr;
  std::vector<std::uint32_t> allocations;
  allocations.reserve(nbatch);
  ankerl::nanobench::Bench bench;
  bench.output(&std::cout);
  bench.minEpochIterations(15);
  bench.batch(nbatch).run(std::string{name},
                          [&]
                          {
                            rand_device dev;
                            allocator_t allocator(size, mgr);
                            for (std::uint32_t allocs = 0; allocs < nbatch; ++allocs)
                            {
                              if ((dev.update() & 0x1) || allocations.empty())
                              {
                                auto alloc_size            = (dev.update() % 100) + 4;
                                auto [arena, halloc, size] = allocator.allocate(alloc_size * T::min_granularity);
                                allocations.push_back(halloc);
                              }
                              else
                              {
                                allocator.deallocate(allocations.back());
                                allocations.pop_back();
                              }
                            }
                            allocations.clear();
                          });
}

int main(int argc, char* argv[])
{
  constexpr uint32_t size = 256 * 256;

  //  (acl::strat::slotted_v0<uint32_t, 256, 255, 4, acl::strat::best_fit_tree<uint32_t>>),
  //  (acl::strat::slotted_v1<uint32_t, 256, 255, 4, acl::strat::best_fit_tree<uint32_t>>),
  //  (acl::strat::slotted_v2<uint32_t, 256, 255, 8, 4, acl::strat::best_fit_tree<uint32_t>>)
  bench_arena<acl::strat::greedy_v0<>>(size, "greedy-v0");
  bench_arena<acl::strat::greedy_v0<>>(size, "greedy-v0");
  bench_arena<acl::strat::greedy_v1<>>(size, "greedy-v1");
  bench_arena<acl::strat::best_fit_tree<>>(size, "bf-tree");
  bench_arena<acl::strat::best_fit_v0<>>(size, "bf-v0");
  bench_arena<acl::strat::best_fit_v1<acl::cfg::bsearch_min0>>(size, "bf-v1-min0");
  bench_arena<acl::strat::best_fit_v1<acl::cfg::bsearch_min1>>(size, "bf-v1-min1");
  bench_arena<acl::strat::best_fit_v1<acl::cfg::bsearch_min2>>(size, "bf-v1-min2");
  bench_arena<acl::strat::best_fit_v2<acl::cfg::bsearch_min0>>(size, "bf-v2-min0");
  bench_arena<acl::strat::best_fit_v2<acl::cfg::bsearch_min1>>(size, "bf-v2-min1");
  bench_arena<acl::strat::best_fit_v2<acl::cfg::bsearch_min2>>(size, "bf-v2-min2");

  return 0;
}
// NOLINTEND