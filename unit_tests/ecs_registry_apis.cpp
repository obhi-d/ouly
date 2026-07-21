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

TEST_CASE("registry: shrink with drained free list", "[ecs][registry]")
{
  ouly::ecs::registry<> reg;
  // emplace() on an empty free list drives the free-slot counter negative;
  // shrink() must clamp instead of resizing to a huge size
  auto e1 = reg.emplace();
  reg.shrink();
  REQUIRE(reg.size() == 1);

  reg.erase(e1);
  reg.shrink();
  REQUIRE(reg.size() == 0);

  auto e2 = reg.emplace(); // reuses the freed slot, drains the free list again
  auto e3 = reg.emplace();
  reg.shrink();
  REQUIRE(reg.size() == 2);
  (void)e2;
  (void)e3;
}
// NOLINTEND
