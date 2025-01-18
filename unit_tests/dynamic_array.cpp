
#include "acl/containers/array_types.hpp"
#include "catch2/catch_all.hpp"
#include <string>

// NOLINTBEGIN
TEST_CASE("dynamic_array: Validate use of dynamic_array")
{
  acl::dynamic_array<std::string> table;

  table = acl::dynamic_array<std::string>(10, "something");
  for (auto& t : table)
    REQUIRE(t == "something");
  table = acl::dynamic_array<std::string>(10, "something_else");
  for (auto& t : table)
    REQUIRE(t == "something_else");

  auto other = acl::dynamic_array<std::string>(10, "second");
  for (auto& t : other)
    REQUIRE(t == "second");
  table = other;
  for (auto& t : other)
    REQUIRE(t == "second");
  for (auto& t : table)
    REQUIRE(t == "second");
}

TEST_CASE("dynamic_array: Validate use of fixed size  dynamic_array")
{
  using darray = acl::fixed_array<std::string, 10>;
  darray table;

  table = darray(10, "something");
  for (auto& t : table)
    REQUIRE(t == "something");
  table = darray(10, "something_else");
  for (auto& t : table)
    REQUIRE(t == "something_else");

  auto other = darray(10, "second");
  for (auto& t : other)
    REQUIRE(t == "second");
  table = other;
  for (auto& t : other)
    REQUIRE(t == "second");
  for (auto& t : table)
    REQUIRE(t == "second");
}
// NOLINTEND