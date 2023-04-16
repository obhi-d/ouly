#include <acl/link_container.hpp>
#include <catch2/catch_all.hpp>
#include <iostream>
#include <string>

struct traits_1
{
  static constexpr uint32_t pool_size  = 2;
  static constexpr bool     use_sparse = false;
};

struct traits_2
{
  static constexpr uint32_t pool_size  = 2;
  static constexpr bool     use_sparse = true;
};

TEMPLATE_TEST_CASE("link_container: Validate", "[link_container][trivial]", traits_1, traits_2)
{
  typename acl::link_container<int, TestType>::registry registry;
  typename acl::link_container<int, TestType>           table;

  auto e1 = registry.emplace();
  auto e2  = registry.emplace();
  auto e3  = registry.emplace();
  auto e4  = registry.emplace();

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

TEMPLATE_TEST_CASE("link_container: Validate", "[link_container][nontrivial]", traits_1, traits_2)
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
