#include "test_common.hpp"
#include <acl/ecs/collection.hpp>
#include <acl/ecs/components.hpp>
#include <acl/ecs/registry.hpp>
#include <catch2/catch_all.hpp>
#include <string>
#include <unordered_map>

// NOLINTBEGIN
struct link_traits_1
{
  static constexpr uint32_t pool_size_v  = 2;
  static constexpr bool     use_sparse_v = false;
};

struct link_traits_2
{
  static constexpr uint32_t pool_size_v  = 2;
  static constexpr bool     use_sparse_v = true;
};

TEST_CASE("component: emplace_at", "[components][void]")
{
  acl::ecs::registry<>      registry;
  auto                      e1 = registry.emplace();
  acl::ecs::components<int> table;
  table.set_max(registry.max_size());
  table.emplace_at(e1, 100);
  REQUIRE(table.at(e1) == 100);
}

TEMPLATE_TEST_CASE("components: validate pod with pool traits", "[components][trivial]", link_traits_1, link_traits_2)
{
  acl::ecs::rxregistry<>                                    registry;
  acl::ecs::components<int, acl::ecs::rxentity<>, TestType> table;

  auto e1 = registry.emplace();
  auto e2 = registry.emplace();
  auto e3 = registry.emplace();
  auto e4 = registry.emplace();

  table.set_max(registry.max_size());
  table.emplace_at(e1, 100);
  table.emplace_at(e2, 200);
  table.emplace_at(e3, 300);
  table.emplace_at(e4, 400);

  REQUIRE(table.at(e1) == 100);
  REQUIRE(table.at(e2) == 200);
  REQUIRE(table.at(e3) == 300);
  REQUIRE(table.at(e4) == 400);

  table.at(e3) = 600;
  REQUIRE(table.at(e3) == 600);

  table.erase(e1);
  registry.erase(e1);

  auto e10 = registry.emplace();
  REQUIRE(e1.get() == e10.get());
#ifndef NDEBUG
  REQUIRE(e1.revision() != e10.revision());
#endif

  table.set_max(registry.max_size());
  table.emplace_at(e10, 1300);
  REQUIRE(table.at(e10) == 1300);
}

TEMPLATE_TEST_CASE("components: validate non-pod with pool traits", "[components][nontrivial]", link_traits_1,
                   link_traits_2)
{
  acl::ecs::rxregistry<>                                            registry;
  acl::ecs::components<std::string, acl::ecs::rxentity<>, TestType> table;

  auto e1 = registry.emplace();
  auto e2 = registry.emplace();
  auto e3 = registry.emplace();
  auto e4 = registry.emplace();

  table.set_max(registry.max_size());
  table.emplace_at(e1, "100");
  table.emplace_at(e2, "200");
  table.emplace_at(e3, "300");
  table.emplace_at(e4, "400");

  REQUIRE(table.at(e1) == "100");
  REQUIRE(table.at(e2) == "200");
  REQUIRE(table.at(e3) == "300");
  REQUIRE(table.at(e4) == "400");

  table.at(e3) = "600";
  REQUIRE(table.at(e3) == "600");

  table.erase(e1);
  registry.erase(e1);

  auto e10 = registry.emplace();
  REQUIRE(e1.get() == e10.get());
#ifndef NDEBUG
  REQUIRE(e1.revision() != e10.revision());
#endif

  table.set_max(registry.max_size());
  table.emplace_at(e10, "1300");
  REQUIRE(table.at(e10) == "1300");
}

TEST_CASE("components: emplace", "[components][nontrivial]")
{
  acl::ecs::rxregistry<>   string_reg;
  std::vector<std::string> string_values;

  auto first  = string_reg.emplace();
  auto second = string_reg.emplace();

  string_values.push_back("0");
  string_values.insert(string_values.begin() + first.get(), "First");
  string_values.insert(string_values.begin() + second.get(), "Second");

  REQUIRE(string_values[1] == "First");
  REQUIRE(string_values[2] == "Second");
  REQUIRE(string_reg.max_size() == 3);

  string_reg.erase(first);

  auto third = string_reg.emplace();

  string_values.insert(string_values.begin() + third.get(), "Third");

  REQUIRE(string_values[1] == "Third");

  REQUIRE(third.get() == first.get());
  REQUIRE(string_reg.is_valid(third) == true);
  REQUIRE(string_reg.is_valid(first) == false);
  REQUIRE(string_reg.get_revision(first) == 1);
  REQUIRE(string_reg.get_revision(third) == 1);
}

TEST_CASE("registry: random test", "[components][nontrivial]")
{
  using clink = acl::ecs::rxentity<>;
  acl::ecs::rxregistry<> string_reg;
  std::vector<clink>     clink_goes_my_bones;
  std::vector<clink>     deleted_ones;

  uint32_t fixed_seed = Catch::rngSeed();

  auto seed = xorshift32(fixed_seed);
  auto end  = seed % 100;
  for (uint32_t i = 0; i < end; ++i)
  {
    if ((seed = xorshift32(seed)) % 4 == 0 && !clink_goes_my_bones.empty())
    {
      string_reg.erase(clink_goes_my_bones.back());
      deleted_ones.push_back(clink_goes_my_bones.back());
      clink_goes_my_bones.pop_back();
    }
    else
    {
      clink_goes_my_bones.emplace_back(string_reg.emplace());
    }
  }

  string_reg.for_each_index(
   [&](uint32_t index)
   {
     auto test = clink(index, string_reg.get_revision(index));
     auto it   = std::ranges::find(deleted_ones, test);
     assert(std::ranges::find(clink_goes_my_bones, test) != clink_goes_my_bones.end());
     REQUIRE(it == deleted_ones.end());
   });

  for (auto d : deleted_ones)
  {
    REQUIRE(string_reg.is_valid(d) == false);
  }
}

TEST_CASE("components: validate named objects", "[rlink_object_table][nontrivial]")
{
  using container = acl::ecs::rxregistry<>;
  using clink     = acl::ecs::rxregistry<>::type;

  acl::ecs::components<std::string, acl::ecs::rxentity<>> names;

  container reg;

  auto entity1 = reg.emplace();
  auto entity2 = reg.emplace();

  names.emplace_at(entity1, "Entity1");
  REQUIRE(names.key(entity1) == 0);

  REQUIRE(names[entity1] == "Entity1");
  REQUIRE(names.contains(entity2) == false);

  names.emplace_at(entity2, "Entity2");
  names.replace(entity1, "Entity1.1");
  REQUIRE(names.contains(entity2) == true);

  REQUIRE(names[entity2] == "Entity2");
  REQUIRE(names[entity1] == "Entity1.1");

  auto entity3 = reg.emplace();
  auto entity4 = reg.emplace();
  auto entity5 = reg.emplace();

  names.emplace_at(entity3, "Entity3");
  names.emplace_at(entity4, "Entity4");
  names.emplace_at(entity5, "Entity5");

  REQUIRE(names[entity2] == "Entity2");
  REQUIRE(names[entity1] == "Entity1.1");
  REQUIRE(names[entity3] == "Entity3");
  REQUIRE(names[entity4] == "Entity4");

  REQUIRE(names.size() == 5);

  reg.erase(entity4);
  names.erase(entity4);

  REQUIRE(names.size() == 4);

  REQUIRE(names.contains(entity4) == false);

  auto entity6 = reg.emplace();
  names.emplace_at(entity6, "Entity6");

  REQUIRE(names.size() == 5);
  REQUIRE(names[entity6] == "Entity6");

  auto entity7 = reg.emplace();
  auto entity8 = reg.emplace();

  names.get_ref(entity8) = "Entity8";
  REQUIRE(names[entity8] == "Entity8");

  names.replace(entity8, "Entity9");
  REQUIRE(names[entity8] == "Entity9");

  auto it = names.find(entity7);
  REQUIRE(it.has_value() == false);

  it = names.find(entity8);
  REQUIRE(it.has_value() == true);
  REQUIRE(it.get() == "Entity9");

  [&](auto const& lookup)
  {
    REQUIRE(lookup.at(entity8) == "Entity9");
    REQUIRE(lookup[entity8] == "Entity9");
  }(names);
}

TEST_CASE("rlink_object_table: fuzz", "[rlink_object_table][nontrivial]")
{
  using container = acl::ecs::rxregistry<>;
  using clink     = acl::ecs::rxregistry<>::type;

  acl::ecs::components<std::string, acl::ecs::rxentity<>> names;

  std::vector<clink>                        bones;
  std::vector<clink>                        deleted;
  std::unordered_map<uint32_t, std::string> map;
  std::unordered_set<std::string>           strings;

  container reg;

  uint32_t fixed_seed = Catch::rngSeed();
  // fixed_seed          = 591856913;

  auto seed = xorshift32(fixed_seed);
  auto end  = seed % 200;
  for (uint32_t i = 0; i < end; ++i)
  {
    if ((seed = xorshift32(seed)) % 4 == 0 && !bones.empty())
    {
      reg.erase(bones.back());
      deleted.push_back(bones.back());
      REQUIRE(strings.find(names[bones.back()]) != strings.end());
      strings.erase(names[bones.back()]);
      map.erase(bones.back().value());
      names.erase(bones.back());
      bones.pop_back();
    }
    else
    {
      bones.emplace_back(reg.emplace());
      names.get_ref(bones.back()) = std::to_string(i);
      map[bones.back().value()]   = names[bones.back()];
      strings.emplace(names[bones.back()]);
    }

    names.validate_integrity();
  }

  std::unordered_set<std::string> visit;
  names.for_each(
   [&](clink lk, std::string const& s)
   {
     auto mapIt = map.find(lk.value());
     REQUIRE(mapIt != map.end());
     REQUIRE(mapIt->second == s);
     visit.emplace(mapIt->second);
   });

  REQUIRE(visit.size() == strings.size());
}

TEST_CASE("collection: validate collection", "[packed_table][emplace]")
{
  static_assert(acl::config<acl::cfg::pool_size<>>::pool_size_v == 4096, "Default pool size");
  static_assert(acl::detail::log2(acl::config<acl::cfg::pool_size<>>::pool_size_v) == 12, "Default pool size");
  acl::ecs::rxregistry<>                                                        registry;
  acl::ecs::collection<acl::ecs::rxentity<>>                                    collection;
  acl::ecs::components<int, acl::ecs::rxentity<>, acl::cfg::use_direct_mapping> data;

  auto e10 = registry.emplace();
  auto e20 = registry.emplace();
  auto e30 = registry.emplace();

  collection.emplace(e10);
  collection.emplace(e20);
  collection.emplace(e30);

  data.emplace_at(e10, 7);
  data.emplace_at(e20, 5);
  data.emplace_at(e30, 11);

  std::uint32_t value = 0;
  collection.for_each(data,
                      [&](auto link, auto const& v)
                      {
                        value += v;
                      });

  REQUIRE(value == 23);
  REQUIRE(collection.contains(e10) == true);
  REQUIRE(collection.contains(e20) == true);
  REQUIRE(collection.contains(e30) == true);

  collection.erase(e20);
  collection.for_each(data,
                      [&](auto link, auto const& v)
                      {
                        value -= v;
                      });
  REQUIRE(value == 5);
  REQUIRE(collection.contains(e10) == true);
  REQUIRE(collection.contains(e20) == false);
  REQUIRE(collection.contains(e30) == true);
}
// NOLINTEND