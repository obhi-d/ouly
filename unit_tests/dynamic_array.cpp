
#include "catch2/catch_all.hpp"
#include "ouly/containers/array_types.hpp"
#include <string>

// NOLINTBEGIN
TEST_CASE("dynamic_array: Validate use of dynamic_array")
{
  ouly::dynamic_array<std::string> table;

  table = ouly::dynamic_array<std::string>(10, "something");
  for (auto& t : table)
    REQUIRE(t == "something");
  table = ouly::dynamic_array<std::string>(10, "something_else");
  for (auto& t : table)
    REQUIRE(t == "something_else");

  auto other = ouly::dynamic_array<std::string>(10, "second");
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
  using darray = ouly::fixed_array<std::string, 10>;
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
TEST_CASE("fixed_array: partial fill and range construction stay in bounds", "[fixed_array]")
{
  // fewer values than N: the tail is value-initialized inside the array (it was written past the end)
  ouly::fixed_array<std::string, 4> filled(2, std::string("x"));
  REQUIRE(filled[0] == "x");
  REQUIRE(filled[1] == "x");
  REQUIRE(filled[2].empty());
  REQUIRE(filled[3].empty());

  std::string const                 values[] = {"a", "b"};
  ouly::fixed_array<std::string, 4> ranged(std::begin(values), std::end(values));
  REQUIRE(ranged[1] == "b");
  REQUIRE(ranged[3].empty());

  // copying an empty (default constructed) array must not read through a null pointer
  ouly::fixed_array<std::string, 4> empty;
  ouly::fixed_array<std::string, 4> copy(empty);
  REQUIRE(copy.data() == nullptr);
  copy = ranged;
  REQUIRE(copy[0] == "a");
  copy = empty;
  REQUIRE(copy.data() == nullptr);
}
// NOLINTEND
