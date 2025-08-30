#include "catch2/catch_all.hpp" // NOLINT(misc-include-cleaner)
#include "ouly/ecs/components.hpp"
#include "ouly/ecs/registry.hpp"

// NOLINTBEGIN
TEST_CASE("components: value iterator direct mapping", "[ecs][components][iterators]")
{
  using E = ouly::ecs::entity<>;
  using C = ouly::ecs::components<int, E, ouly::cfg::use_direct_mapping>;

  ouly::ecs::registry<> reg;
  C                     comp;

  auto                  e1   = reg.emplace();
  [[maybe_unused]] auto hole = reg.emplace(); // create a hole
  auto                  e3   = reg.emplace();

  comp.set_max(reg.max_size());
  comp.emplace_at(e1, 10);
  comp.emplace_at(e3, 30);
  // Note: e2 left empty to create a hole

  std::vector<int> values;
  for (int v : comp)
  {
    values.push_back(v);
  }
  REQUIRE(values.size() == 2);
  REQUIRE(values[0] == 10);
  REQUIRE(values[1] == 30);

  // Entity pair iteration
  std::vector<uint32_t> entities;
  for (auto it = comp.begin_entities(); it != comp.end_entities(); ++it)
  {
    auto pr = *it;
    entities.push_back(pr.e_.value());
  }
  REQUIRE(entities.size() == 2);
  REQUIRE(entities[0] == e1.value());
  REQUIRE(entities[1] == e3.value());
}

TEST_CASE("components: value iterator packed (indirect)", "[ecs][components][iterators]")
{
  using E = ouly::ecs::entity<>;
  using C = ouly::ecs::components<int, E>;

  ouly::ecs::registry<> reg;
  C                     comp;

  auto e1 = reg.emplace();
  auto e2 = reg.emplace();
  auto e3 = reg.emplace();

  comp.emplace_at(e1, 10);
  comp.emplace_at(e2, 20);
  comp.emplace_at(e3, 30);

  std::vector<int> values;
  for (int v : comp)
  {
    values.push_back(v);
  }
  REQUIRE(values.size() == 3);
  REQUIRE(values[0] == 10);
  REQUIRE(values[1] == 20);
  REQUIRE(values[2] == 30);

  // Entity pairs
  std::vector<uint32_t> entities;
  for (auto pr_it = comp.begin_entities(); pr_it != comp.end_entities(); ++pr_it)
  {
    auto pr = *pr_it;
    entities.push_back(pr.e_.value());
  }
  REQUIRE(entities.size() == 3);
  REQUIRE(entities[0] == e1.value());
  REQUIRE(entities[1] == e2.value());
  REQUIRE(entities[2] == e3.value());
}
// NOLINTEND
