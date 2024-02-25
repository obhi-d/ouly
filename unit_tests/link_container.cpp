#include "test_common.hpp"
#include <acl/containers/link_container.hpp>
#include <acl/containers/rlink_registry.hpp>
#include <catch2/catch_all.hpp>
#include <iostream>
#include <string>
#include <unordered_map>

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

TEST_CASE("link_container: Validate", "[link_container][void]")
{

  acl::link_registry<>     registry;
  auto                     e1 = registry.emplace();
  acl::link_container<int> table;
  table.sync(registry);
  table.emplace(e1, 100);
  REQUIRE(table.at(e1) == 100);
}

TEMPLATE_TEST_CASE("link_container: Validate", "[link_container][trivial]", link_traits_1, link_traits_2)
{
  typename acl::link_container<int, TestType>::registry registry;
  typename acl::link_container<int, TestType>           table;

  auto e1 = registry.emplace();
  auto e2 = registry.emplace();
  auto e3 = registry.emplace();
  auto e4 = registry.emplace();

  table.sync(registry);
  table.emplace(e1, 100);
  table.emplace(e2, 200);
  table.emplace(e3, 300);
  table.emplace(e4, 400);

  REQUIRE(table.at(e1) == 100);
  REQUIRE(table.at(e2) == 200);
  REQUIRE(table.at(e3) == 300);
  REQUIRE(table.at(e4) == 400);

  table.at(e3) = 600;
  REQUIRE(table.at(e3) == 600);

  table.erase(e1);
  registry.erase(e1);

  auto e10 = registry.emplace();
  REQUIRE(e1.as_index() == e10.as_index());
#ifndef NDEBUG
  REQUIRE(e1.revision() != e10.revision());
#endif

  table.sync(registry);
  table.emplace(e10, 1300);
  REQUIRE(table.at(e10) == 1300);
}

TEMPLATE_TEST_CASE("link_container: Validate", "[link_container][nontrivial]", link_traits_1, link_traits_2)
{
  typename acl::link_container<std::string, TestType>::registry registry;
  typename acl::link_container<std::string, TestType>           table;

  auto e1 = registry.emplace();
  auto e2 = registry.emplace();
  auto e3 = registry.emplace();
  auto e4 = registry.emplace();

  table.sync(registry);
  table.emplace(e1, "100");
  table.emplace(e2, "200");
  table.emplace(e3, "300");
  table.emplace(e4, "400");

  REQUIRE(table.at(e1) == "100");
  REQUIRE(table.at(e2) == "200");
  REQUIRE(table.at(e3) == "300");
  REQUIRE(table.at(e4) == "400");

  table.at(e3) = "600";
  REQUIRE(table.at(e3) == "600");

  table.erase(e1);
  registry.erase(e1);

  auto e10 = registry.emplace();
  REQUIRE(e1.as_index() == e10.as_index());
#ifndef NDEBUG
  REQUIRE(e1.revision() != e10.revision());
#endif

  table.sync(registry);
  table.emplace(e10, "1300");
  REQUIRE(table.at(e10) == "1300");
}

TEST_CASE("rlink_registry: emplace", "[rlink_registry][nontrivial]")
{
  acl::basic_rlink_registry<std::string> string_reg;
  std::vector<std::string>               string_values;

  auto first  = string_reg.emplace();
  auto second = string_reg.emplace();

  string_values.push_back("0");
  string_values.insert(string_values.begin() + first.as_index(), "First");
  string_values.insert(string_values.begin() + second.as_index(), "Second");

  REQUIRE(string_values[1] == "First");
  REQUIRE(string_values[2] == "Second");
  REQUIRE(string_reg.max_size() == 3);

  string_reg.erase(first);

  auto third = string_reg.emplace();

  string_values.insert(string_values.begin() + third.as_index(), "Third");

  REQUIRE(string_values[1] == "Third");

  REQUIRE(third.as_index() == first.as_index());
  REQUIRE(string_reg.is_valid(third) == true);
  REQUIRE(string_reg.is_valid(first) == false);
  REQUIRE(string_reg.get_revision(first) == 1);
  REQUIRE(string_reg.get_revision(third) == 1);
}

TEST_CASE("rlink_registry: random test", "[rlink_registry][nontrivial]")
{
  using container = acl::basic_rlink_registry<std::string>;
  using clink     = container::link;
  container          string_reg;
  std::vector<clink> clink_goes_my_bones;
  std::vector<clink> deleted_ones;

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
