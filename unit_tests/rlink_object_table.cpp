#include "test_common.hpp"
#include <acl/containers/rlink_object_table.hpp>
#include <acl/containers/rlink_registry.hpp>
#include <catch2/catch_all.hpp>
#include <string>
#include <unordered_map>
#include <unordered_set>

TEST_CASE("rlink_object_table: Validate", "[rlink_object_table][nontrivial]")
{
  using container = acl::rlink_registry<>;
  using clink     = container::link;

  acl::rlink_object_table<std::string> names;

  container reg;

  auto entity1 = reg.emplace();
  auto entity2 = reg.emplace();

  names.emplace_at(entity1, "Entity1");

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
}

TEST_CASE("rlink_object_table: fuzz", "[rlink_object_table][nontrivial]")
{
  using container = acl::rlink_registry<>;
  using clink     = container::link;

  acl::rlink_object_table<std::string>      names;
  std::vector<clink>                        bones;
  std::vector<clink>                        deleted;
  std::unordered_map<uint32_t, std::string> map;
  std::unordered_set<std::string>           strings;

  container reg;

  uint32_t fixed_seed = Catch::rngSeed();

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