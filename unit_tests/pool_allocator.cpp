#include "ouly/allocators/pool_allocator.hpp"
#include "catch2/catch_all.hpp"
#include "ouly/allocators/std_allocator_wrapper.hpp"
#include <random>

// NOLINTBEGIN
TEST_CASE("Validate pool_allocator", "[pool_allocator]")
{
  using namespace ouly;
  using allocator_t = pool_allocator<ouly::config<ouly::cfg::compute_stats>>;
  struct trivial_object
  {
    std::uint8_t value[16];
  };
  struct record
  {
    trivial_object* data;
    std::uint32_t   count;
  };
  constexpr std::uint32_t                      k_atom_count = 1000;
  ouly::vector<record>                         records;
  std::minstd_rand                             gen;
  std::bernoulli_distribution                  dice(0.6);
  std::uniform_int_distribution<std::uint32_t> generator(1, k_atom_count / 2);

  allocator_t allocator(sizeof(trivial_object), k_atom_count);

  auto validate = [&]()
  {
    return allocator.validate(records);
  };

  for (std::uint32_t allocs = 0; allocs < 10000; ++allocs)
  {
    if (dice(gen) || records.size() == 0)
    {
      record r;

      r.count = dice(gen) ? 1 : generator(gen);
      r.data  = reinterpret_cast<trivial_object*>(allocator.allocate(r.count * sizeof(trivial_object)));

      records.push_back(r);
      CHECK(validate());
    }
    else
    {
      std::uniform_int_distribution<std::uint32_t> choose(0, static_cast<std::uint32_t>(records.size() - 1));
      std::uint32_t                                chosen = choose(gen);
      allocator.deallocate(records[chosen].data, records[chosen].count * sizeof(trivial_object));
      records.erase(records.begin() + chosen);
      CHECK(validate());
    }
  }
}

TEST_CASE("Validate pool_allocator with alignment", "[pool_allocator]")
{
  using namespace ouly;
  using allocator_t = pool_allocator<ouly::config<ouly::cfg::compute_stats>>;
  struct trivial_object
  {
    std::uint8_t value[16];
  };
  struct record
  {
    trivial_object* data;
    std::uint32_t   count;
  };
  constexpr std::uint32_t                      k_atom_count = 1000;
  ouly::vector<record>                         records;
  std::minstd_rand                             gen;
  std::bernoulli_distribution                  dice(0.6);
  std::uniform_int_distribution<std::uint32_t> generator(1, k_atom_count / 2);

  allocator_t allocator(sizeof(trivial_object), k_atom_count);

  auto validate = [&]()
  {
    return allocator.validate(records);
  };

  for (std::uint32_t allocs = 0; allocs < 10000; ++allocs)
  {
    if (dice(gen) || records.size() == 0)
    {
      record r;

      r.count = dice(gen) ? 1 : generator(gen);
      r.data  = reinterpret_cast<trivial_object*>(
       allocator.allocate(r.count * sizeof(trivial_object), ouly::alignarg<trivial_object>));

      records.push_back(r);
      OULY_ASSERT(validate());
      CHECK(validate());
    }
    else
    {
      std::uniform_int_distribution<std::uint32_t> choose(0, static_cast<std::uint32_t>(records.size() - 1));
      std::uint32_t                                chosen = choose(gen);
      allocator.deallocate(records[chosen].data, records[chosen].count * sizeof(trivial_object),
                           ouly::alignarg<trivial_object>);
      records.erase(records.begin() + chosen);
      OULY_ASSERT(validate());
      CHECK(validate());
    }
  }
}

TEST_CASE("Validate std_allocator", "[std_allocator]")
{

  using namespace ouly;
  ouly::pool_allocator<> pool_allocator(8, 1000);
  {
    using std_allocator = ouly::allocator_ref<std::uint64_t, ouly::pool_allocator<>>;
    std::vector<std::uint64_t, std_allocator> vlist =
     std::vector<std::uint64_t, std_allocator>(std_allocator(pool_allocator));

    for (std::uint64_t i = 0; i < 1000; ++i)
      vlist.push_back(i);
  }
}
TEST_CASE("pool_allocator: release into an exhausted array list", "[pool_allocator]")
{
  // two 2-atom blocks consume the whole 4-atom arena, leaving no free array to compare against
  ouly::pool_allocator<> allocator(16, 4);
  auto                   p = allocator.allocate(32);
  auto                   q = allocator.allocate(32);
  REQUIRE(p != q);
  allocator.deallocate(q, 32);
  allocator.deallocate(p, 32);
  auto r = allocator.allocate(48);
  REQUIRE(r != nullptr);
  allocator.deallocate(r, 48);
}

TEST_CASE("pool_allocator: non power of two atom size", "[pool_allocator]")
{
  ouly::pool_allocator<> allocator(24, 16);
  auto                   a = static_cast<std::uint8_t*>(allocator.allocate(24));
  auto                   b = static_cast<std::uint8_t*>(allocator.allocate(48));
  auto                   c = static_cast<std::uint8_t*>(allocator.allocate(25));
  REQUIRE(a != nullptr);
  REQUIRE(b != nullptr);
  REQUIRE(c != nullptr);
  allocator.deallocate(c, 25);
  allocator.deallocate(b, 48);
  allocator.deallocate(a, 24);
}
// NOLINTEND
