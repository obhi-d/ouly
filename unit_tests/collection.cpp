#include "test_common.hpp"
#include <acl/collection.hpp>
#include <acl/packed_table.hpp>
#include <catch2/catch_all.hpp>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

TEST_CASE("collection: Validate packed_table emplace", "[packed_table][emplace]")
{
  static_assert(acl::options<acl::pool_size<>>::pool_size_v == 4096, "Default pool size");
  static_assert(acl::detail::log2(acl::options<acl::pool_size<>>::pool_size_v) == 12, "Default pool size");
  acl::packed_table<int> table;
  acl::collection<int>   collection;

  auto e10 = table.emplace(10);
  auto e20 = table.emplace(20);
  auto e30 = table.emplace(30);

  REQUIRE(table.at(e10) == 10);
  REQUIRE(table.at(e20) == 20);
  REQUIRE(table.at(e30) == 30);

  collection.emplace(e10);
  collection.emplace(e20);
  collection.emplace(e30);

  std::uint32_t value = 0;
  collection.for_each(table,
                      [&](auto link, auto const& v)
                      {
                        value += v;
                      });

  REQUIRE(value == 60);
  REQUIRE(collection.contains(e10) == true);
  REQUIRE(collection.contains(e20) == true);
  REQUIRE(collection.contains(e30) == true);

  collection.erase(e20);
  collection.for_each(table,
                      [&](auto link, auto const& v)
                      {
                        value -= v;
                      });
  REQUIRE(value == 20);
  REQUIRE(collection.contains(e10) == true);
  REQUIRE(collection.contains(e20) == false);
  REQUIRE(collection.contains(e30) == true);
}
