#include "catch2/catch_all.hpp" // NOLINT(misc-include-cleaner)
#include "ouly/ecs/registry.hpp"

// NOLINTBEGIN
TEST_CASE("registry: size and empty APIs", "[ecs][registry]")
{
  ouly::ecs::registry<> reg;
  REQUIRE(reg.empty());
  REQUIRE(reg.size() == 0);

  auto e1 = reg.emplace();
  auto e2 = reg.emplace();
  REQUIRE_FALSE(reg.empty());
  REQUIRE(reg.size() == 2);

  reg.erase(e1);
  REQUIRE(reg.size() == 1);

  auto e3 = reg.emplace();
  (void)e3;
  REQUIRE(reg.size() == 2);

  reg.erase(e2);
  REQUIRE(reg.size() == 1);
}
// NOLINTEND
