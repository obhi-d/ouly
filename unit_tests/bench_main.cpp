#define ANKERL_NANOBENCH_IMPLEMENT
#include <nanobench.h>
#include <acl/arena_allocator.hpp>
#include <string_view>

template <typename strat>
void bench_arena(uint32_t size);

int main(int argc, char* argv[]) 
{

  if (argc < 2)
    return -1;

  if (std::string_view(argv[1]) == "bf-tree")
    bench_arena<acl::strat::best_fit_tree<uint32_t>>(256);
  else if (std::string_view(argv[1]) == "bf-v0")
    bench_arena<acl::strat::best_fit_v0<uint32_t>>(256);
  else if (std::string_view(argv[1]) == "bf-v1")
    bench_arena<acl::strat::best_fit_v1<uint32_t>>(256);
  else if (std::string_view(argv[1]) == "bf-v2")
    bench_arena<acl::strat::best_fit_v2<uint32_t, false, false>>(256);
  else if (std::string_view(argv[1]) == "bf-v2-p")
    bench_arena<acl::strat::best_fit_v2<uint32_t, true, false>>(256);
  else if (std::string_view(argv[1]) == "slot")
    bench_arena<acl::strat::slotted<uint32_t, 256, 256, false, false>>(256);
  else if (std::string_view(argv[1]) == "slot-p")
    bench_arena<acl::strat::slotted<uint32_t, 256, 256, true, false>>(256);

  return 0;
}