#include "catch2/catch_all.hpp" // NOLINT(misc-include-cleaner)
#include "ouly/containers/index_map.hpp"
#include "ouly/containers/sparse_vector.hpp"
#include "ouly/containers/table.hpp"
#include "ouly/ecs/collection.hpp"
#include "ouly/ecs/components.hpp"
#include "ouly/ecs/map.hpp"
#include "ouly/ecs/registry.hpp"
#include <set>
#include <string>
#include <vector>

// NOLINTBEGIN
TEMPLATE_TEST_CASE("collection: clear resets presence bits", "[ecs][collection]", ouly::ecs::entity<>,
                   ouly::ecs::rxentity<>)
{
  ouly::ecs::collection<TestType> col;

  col.emplace(TestType(1));
  col.emplace(TestType(2));
  col.emplace(TestType(70)); // second 64-bit word
  REQUIRE(col.size() == 3);

  col.clear();
  REQUIRE(col.size() == 0);
  REQUIRE(col.empty());
  // Stale bits must not survive a clear
  REQUIRE_FALSE(col.contains(TestType(1)));
  REQUIRE_FALSE(col.contains(TestType(2)));
  REQUIRE_FALSE(col.contains(TestType(70)));

  // Re-inserting after clear must count again
  col.emplace(TestType(1));
  REQUIRE(col.size() == 1);
  REQUIRE(col.contains(TestType(1)));
}

TEMPLATE_TEST_CASE("collection: shrink_to_fit is idempotent and collection is reusable", "[ecs][collection]",
                   ouly::ecs::entity<>, ouly::ecs::rxentity<>)
{
  ouly::ecs::collection<TestType> col;

  col.emplace(TestType(5));
  col.erase(TestType(5));
  REQUIRE(col.empty());

  // Repeated shrink_to_fit on an empty collection must not double free
  col.shrink_to_fit();
  col.shrink_to_fit();

  // Collection must remain usable after releasing pages
  col.emplace(TestType(5));
  REQUIRE(col.contains(TestType(5)));
  REQUIRE(col.size() == 1);
  // Destructor runs at scope end; sanitizers verify no double free
}

TEMPLATE_TEST_CASE("collection: sparse entity ids spanning many pools", "[ecs][collection]", ouly::ecs::entity<>,
                   ouly::ecs::rxentity<>)
{
  ouly::ecs::collection<TestType> col;

  // Jump far past the first pool; intermediate pools must not be touched incorrectly
  col.emplace(TestType(0));
  col.emplace(TestType(1000000));
  REQUIRE(col.size() == 2);
  REQUIRE(col.contains(TestType(0)));
  REQUIRE(col.contains(TestType(1000000)));
  // Ids inside skipped pools are absent
  REQUIRE_FALSE(col.contains(TestType(5000)));
  REQUIRE_FALSE(col.contains(TestType(999999)));

  // Copying a collection with unallocated (null) pages must work
  ouly::ecs::collection<TestType> copy(col);
  REQUIRE(copy.size() == 2);
  REQUIRE(copy.contains(TestType(0)));
  REQUIRE(copy.contains(TestType(1000000)));
  REQUIRE_FALSE(copy.contains(TestType(5000)));

  // Move must leave the source empty and reusable
  ouly::ecs::collection<TestType> moved(std::move(col));
  REQUIRE(moved.size() == 2);
  REQUIRE(moved.contains(TestType(1000000)));
  REQUIRE(col.size() == 0);
  col.emplace(TestType(3));
  REQUIRE(col.contains(TestType(3)));
}

TEMPLATE_TEST_CASE("collection: for_each visits exactly the inserted set across word and pool boundaries",
                   "[ecs][collection]", ouly::ecs::entity<>, ouly::ecs::rxentity<>)
{
  using E = TestType;
  ouly::ecs::collection<E>      col;
  ouly::ecs::components<int, E> comp;
  std::set<uint32_t> const      ids = {0, 1, 63, 64, 65, 127, 128, 200, 4095, 4096, 5000};

  for (auto id : ids)
  {
    col.emplace(E(id));
    comp.emplace_at(E(id), static_cast<int>(id));
  }

  std::set<uint32_t> visited;
  col.for_each(comp,
               [&](E e, int& v)
               {
                 REQUIRE(static_cast<uint32_t>(v) == e.get());
                 visited.insert(e.get());
               });
  REQUIRE(visited == ids);

  // Ranged iteration honors [first, last)
  std::set<uint32_t> ranged;
  col.for_each(comp, 63, 129,
               [&](E e, int&)
               {
                 ranged.insert(e.get());
               });
  REQUIRE(ranged == std::set<uint32_t>{63, 64, 65, 127, 128});

  // Erased entities disappear from iteration
  col.erase(E(64));
  col.erase(E(4096));
  std::set<uint32_t> after_erase;
  col.for_each(comp,
               [&](E e, int&)
               {
                 after_erase.insert(e.get());
               });
  std::set<uint32_t> expected = ids;
  expected.erase(64);
  expected.erase(4096);
  REQUIRE(after_erase == expected);

  // Const overload matches
  auto const&        ccol = col;
  std::set<uint32_t> cvisited;
  ccol.for_each(static_cast<ouly::ecs::components<int, E> const&>(comp),
                [&](E e, int const&)
                {
                  cvisited.insert(e.get());
                });
  REQUIRE(cvisited == expected);
}

TEST_CASE("components: direct mapping for_each skips holes across word boundaries", "[ecs][components]")
{
  using E = ouly::ecs::entity<>;
  ouly::ecs::components<int, E, ouly::cfg::use_direct_mapping> comp;

  std::set<uint32_t> const ids = {0, 5, 63, 64, 65, 126, 127, 128, 191, 192, 300};
  for (auto id : ids)
  {
    comp.emplace_at(E(id), static_cast<int>(id) * 2);
  }
  REQUIRE(comp.size() == ids.size());

  std::set<uint32_t> visited;
  comp.for_each(
   [&](E e, int& v)
   {
     REQUIRE(v == static_cast<int>(e.get()) * 2);
     visited.insert(e.get());
   });
  REQUIRE(visited == ids);

  // Subrange starting mid-word
  std::set<uint32_t> ranged;
  comp.for_each(64, 192,
                [&](E e, int&)
                {
                  ranged.insert(e.get());
                });
  REQUIRE(ranged == std::set<uint32_t>{64, 65, 126, 127, 128, 191});

  // Erase then re-iterate
  comp.erase(E(64));
  comp.erase(E(300));
  std::set<uint32_t> after;
  comp.for_each(
   [&](E e, int&)
   {
     after.insert(e.get());
   });
  std::set<uint32_t> expected = ids;
  expected.erase(64);
  expected.erase(300);
  REQUIRE(after == expected);
  REQUIRE(comp.size() == expected.size());

  // Value iterators agree with for_each
  std::multiset<int> iterated;
  for (int v : comp)
  {
    iterated.insert(v);
  }
  REQUIRE(iterated.size() == expected.size());
}

TEST_CASE("components: indirect mapping presence is consistent without bitfield", "[ecs][components]")
{
  using E = ouly::ecs::entity<>;
  ouly::ecs::components<std::string, E> comp;

  REQUIRE(comp.empty());

  comp.emplace_at(E(10), "ten");
  comp.emplace_at(E(3), "three");
  comp.emplace_at(E(77), "seventy-seven");
  REQUIRE_FALSE(comp.empty());
  REQUIRE(comp.size() == 3);
  REQUIRE(comp.contains(E(10)));
  REQUIRE(comp.contains(E(77)));
  REQUIRE_FALSE(comp.contains(E(11)));
  REQUIRE_FALSE(comp.contains(E(100000)));

  // Erase middle: swap-and-pop must keep the other entries reachable
  comp.erase(E(10));
  REQUIRE_FALSE(comp.contains(E(10)));
  REQUIRE(comp.find(E(10)).has_value() == false);
  REQUIRE(comp.contains(E(3)));
  REQUIRE(comp.at(E(3)) == "three");
  REQUIRE(comp.at(E(77)) == "seventy-seven");
  REQUIRE(comp.size() == 2);

  // replace() on an absent entity inserts
  comp.replace(E(10), "ten again");
  REQUIRE(comp.at(E(10)) == "ten again");

  // get_ref on an absent entity default-constructs
  auto& fresh = comp.get_ref(E(500));
  REQUIRE(fresh.empty());
  REQUIRE(comp.contains(E(500)));
  REQUIRE(comp.size() == 4);

  comp.clear();
  REQUIRE(comp.empty());
  REQUIRE_FALSE(comp.contains(E(3)));
}

TEST_CASE("map: erase_and_swap_values handles empty and single-element containers", "[ecs][map]")
{
  using E = ouly::ecs::entity<>;

  SECTION("single element")
  {
    ouly::ecs::map<E> m;
    std::vector<int>  values;
    auto              idx = m.emplace(E(42));
    values.resize(m.size());
    values[idx] = 7;

    m.erase_and_swap_values(E(42), values);
    REQUIRE(values.empty());
    REQUIRE(m.size() == 0);
    REQUIRE_FALSE(m.contains(E(42)));
  }

  SECTION("empty external container does not underflow")
  {
    ouly::ecs::map<E> m;
    std::vector<int>  values; // intentionally left empty / out of sync
    m.emplace(E(1));
    m.erase_and_swap_values(E(1), values);
    REQUIRE(values.empty());
    REQUIRE(m.size() == 0);
  }

  SECTION("swap from back updates mapping")
  {
    ouly::ecs::map<E> m;
    std::vector<int>  values;
    for (uint32_t i = 0; i < 5; ++i)
    {
      auto idx = m.emplace(E(i));
      values.resize(m.size());
      values[idx] = static_cast<int>(i * 10);
    }
    m.erase_and_swap_values(E(1), values);
    REQUIRE(values.size() == 4);
    for (uint32_t i = 0; i < 5; ++i)
    {
      if (i == 1)
      {
        REQUIRE_FALSE(m.contains(E(i)));
        continue;
      }
      REQUIRE(m.contains(E(i)));
      REQUIRE(values[m.key(E(i))] == static_cast<int>(i * 10));
    }
  }
}

namespace
{
struct tracked_int_cfg
{
  static constexpr std::uint32_t pool_size_v  = 4;
  static constexpr int           null_v       = -1;
  static constexpr bool          assume_pod_v = true;
};
} // namespace

TEST_CASE("sparse_vector: ensure materializes a slot without affecting size", "[sparse_vector]")
{
  ouly::sparse_vector<int, tracked_int_cfg> v;
  auto&                                     slot = v.ensure(9);
  REQUIRE(slot == -1); // null-filled page
  slot = 99;
  REQUIRE(*v.get_if(9) == 99);
  REQUIRE(v.size() == 0); // ensure does not grow logical size
}

TEST_CASE("sparse_vector: overwriting a slot does not inflate pool occupancy", "[sparse_vector]")
{
  ouly::sparse_vector<int, tracked_int_cfg> v;

  v.emplace_at(0, 10);
  v.emplace_at(1, 11);
  v.emplace_at(1, 12); // overwrite, must not count twice
  REQUIRE(v[1] == 12);

  v.erase(0);
  v.erase(1);
  // With balanced occupancy the first pool is reclaimed once empty
  REQUIRE(v.get_if(0) == nullptr);
  REQUIRE(v.get_if(1) == nullptr);
}

TEST_CASE("index_map: shifting base offset preserves existing mappings", "[index_map]")
{
  ouly::index_map<uint32_t, 4> m;

  m[100] = 1;
  REQUIRE(m.base_offset() == 100);

  // Small shift while under the offset limit (amount > current size)
  m[90] = 2;
  REQUIRE(m.base_offset() == 90);
  REQUIRE(m.find(100) == 1);
  REQUIRE(m.find(90) == 2);

  // Small shift with amount < current size (overlapping move)
  m[88] = 3;
  m[89] = 4;
  REQUIRE(m.find(100) == 1);
  REQUIRE(m.find(90) == 2);
  REQUIRE(m.find(88) == 3);
  REQUIRE(m.find(89) == 4);

  // Exceed the limit: the map fully grows to base zero
  m[2] = 5;
  REQUIRE(m.base_offset() == 0);
  REQUIRE(m.find(100) == 1);
  REQUIRE(m.find(90) == 2);
  REQUIRE(m.find(88) == 3);
  REQUIRE(m.find(89) == 4);
  REQUIRE(m.find(2) == 5);
  REQUIRE(m.find(50) == ouly::index_map<uint32_t, 4>::null);
}

TEST_CASE("table: recycled slots construct non-POD values in place", "[table]")
{
  ouly::table<std::string> t;

  auto i0 = t.emplace("first");
  auto i1 = t.emplace("second");
  REQUIRE(t.size() == 2);

  t.erase(i0);
  REQUIRE(t.size() == 1);

  auto i2 = t.emplace("third");
  REQUIRE(i2 == i0); // slot reuse
  REQUIRE(t[i2] == "third");
  REQUIRE(t[i1] == "second");
  REQUIRE(t.size() == 2);
}

TEST_CASE("registry: free list reuse and const iteration", "[ecs][registry]")
{
  ouly::ecs::registry<> reg;

  std::vector<ouly::ecs::registry<>::type> ents;
  for (int i = 0; i < 6; ++i)
  {
    ents.push_back(reg.emplace());
  }
  reg.erase(ents[1]);
  reg.erase(ents[4]);
  reg.erase(ents[2]);

  // Recycled slots come back from the free list
  auto               r1 = reg.emplace();
  auto               r2 = reg.emplace();
  auto               r3 = reg.emplace();
  std::set<uint32_t> recycled{r1.get(), r2.get(), r3.get()};
  REQUIRE(recycled == std::set<uint32_t>{ents[1].get(), ents[2].get(), ents[4].get()});
  REQUIRE(reg.size() == 6);

  reg.erase(r1);
  reg.erase(r3);

  // Const iteration over an unsorted free list must visit exactly the alive ids
  auto const&        creg = reg;
  std::set<uint32_t> alive;
  creg.for_each_index(
   [&](uint32_t id)
   {
     alive.insert(id);
   });
  REQUIRE(alive.size() == reg.size());
  REQUIRE_FALSE(alive.contains(r1.get()));
  REQUIRE_FALSE(alive.contains(r3.get()));

  // Mutable overload sorts in place and agrees
  std::set<uint32_t> alive2;
  reg.for_each_index(
   [&](uint32_t id)
   {
     alive2.insert(id);
   });
  REQUIRE(alive2 == alive);
}
// NOLINTEND
