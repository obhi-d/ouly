
#include "catch2/catch_all.hpp"
#include "ouly/dsl/microexpr.hpp"
#include <unordered_map>

// NOLINTBEGIN
TEST_CASE("Validate basic expressions", "[microexpr]")
{
  std::unordered_map<std::string_view, int> table = {
   { "FIRST",  1},
   {"SECOND", 10},
   { "THIRD",  0},
   {"FOURTH", 20},
   { "FIFTH",  2},
  };

  ouly::microexpr expr(
   [&table](std::string_view v) -> std::optional<int>
   {
     auto it = table.find(v);
     if (it != table.end())
       return {it->second};
     return {};
   });

  REQUIRE(expr.evaluate("$FIRST && $SECOND") == true);
  REQUIRE(expr.evaluate("$ANY || $OTHER") == false);
  REQUIRE(expr.evaluate("$THIRD || $OTHER") == true);
  REQUIRE(expr.evaluate("$THIRD | $OTHER") == true);
  REQUIRE(expr.evaluate("FIRST | FIFTH == 3") == true);
  REQUIRE(expr.evaluate("-FIRST == -1") == true);
  REQUIRE(expr.evaluate("~THIRD == 0xffffffffffffffff") == true);
  REQUIRE(expr.evaluate("FIRST > 1") == false);
  REQUIRE(expr.evaluate("FIRST == 1") == true);
  REQUIRE(expr.evaluate("$THIRD == 1") == true);
  REQUIRE(expr.evaluate("$THIRD") == true);
  REQUIRE(expr.evaluate("THIRD == 0") == true);
  REQUIRE(expr.evaluate("THIRD") == false);
  REQUIRE(expr.evaluate("THIRD + FIRST + SECOND==11") == true);
  REQUIRE(expr.evaluate("THIRD + FIRST - SECOND==-9") == true);
  REQUIRE(expr.evaluate("THIRD + FIRST - SECOND>=-9") == true);
  REQUIRE(expr.evaluate("THIRD + FIRST - SECOND>-9") == false);
  REQUIRE(expr.evaluate("FOURTH - FIRST - SECOND==9") == true);
  REQUIRE(expr.evaluate("FOURTH - (FIRST - SECOND)==29") == true);
  REQUIRE(expr.evaluate("SECOND ^ (FIFTH)==8") == true);
  REQUIRE(expr.evaluate("SECOND * (FIFTH) / FIRST==20") == true);
  REQUIRE(expr.evaluate("SECOND / (FIFTH) <= 5") == true);
  REQUIRE(expr.evaluate("SECOND / (FIFTH) >= 5") == true);
  REQUIRE(expr.evaluate("SECOND / (FIFTH) < 5") == false);
  REQUIRE(expr.evaluate("SECOND / (FIFTH) > 5") == false);
  REQUIRE(expr.evaluate("SECOND / (FIFTH) > 5 ? 0 : 1") == true);
  REQUIRE(expr.evaluate("SECOND / (SECOND * FIFTH) > 5 ? 0 : 1") == true);
}
TEST_CASE("microexpr: logical operators, negation and identifiers", "[microexpr]")
{
  std::unordered_map<std::string_view, int> table = {
   {      "A", 1},
   {      "B", 0},
   {"MY_FLAG", 3},
  };

  ouly::microexpr expr(
   [&table](std::string_view v) -> std::optional<int>
   {
     auto it = table.find(v);
     if (it != table.end())
       return {it->second};
     return {};
   });

  // the right operand of && / || must be consumed even when the result is already known
  REQUIRE(expr.evaluate("0 && 1 ? 5 : 0") == false);
  REQUIRE(expr.evaluate("0 && 1 ? 0 : 5") == true);
  REQUIRE(expr.evaluate("(1 || B)") == true);
  REQUIRE(expr.evaluate("(A || B)") == true);
  REQUIRE(expr.evaluate("(B && A) == 0") == true);

  REQUIRE(expr.evaluate("!B") == true);
  REQUIRE(expr.evaluate("!A") == false);
  REQUIRE(expr.evaluate("!$UNDEFINED") == true);
  REQUIRE(expr.evaluate("$(A) && !$(UNDEFINED)") == true);
  REQUIRE(expr.evaluate("$( A ) && $(B)") == true);

  REQUIRE(expr.evaluate("MY_FLAG == 3") == true);
  REQUIRE(expr.evaluate("$MY_FLAG") == true);

  // malformed input must not read past the end
  REQUIRE(expr.evaluate("(1") == false);
  REQUIRE(expr.evaluate("$(A") == false);
  REQUIRE(expr.evaluate("") == false);
}
// NOLINTEND
