#include "catch2/catch_all.hpp"
#include "ouly/ecs/entity.hpp"
#include "ouly/ecs/map.hpp"
#include "test_common.hpp"
#include <algorithm>
#include <limits>
#include <string>
#include <vector>

// NOLINTBEGIN
// Test component data for testing with external value arrays
struct TestComponent
{
  int         value;
  std::string name;

  TestComponent() = default;
  TestComponent(int v, std::string n) : value(v), name(std::move(n)) {}

  bool operator==(const TestComponent& other) const
  {
    return value == other.value && name == other.name;
  }
};

TEST_CASE("ecs::map: Basic construction and properties", "[ecs][map]")
{
  SECTION("Default construction")
  {
    ouly::ecs::map<> map;
    REQUIRE(map.size() == 0);
    REQUIRE(map.empty());
  }

  SECTION("Copy and move construction")
  {
    ouly::ecs::map<>    original;
    ouly::ecs::entity<> e1{42};
    ouly::ecs::entity<> e2{100};

    original.emplace(e1);
    original.emplace(e2);

    // Copy construction
    auto copied = original;
    REQUIRE(copied.size() == 2);
    REQUIRE(copied.contains(e1));
    REQUIRE(copied.contains(e2));

    // Move construction
    auto moved = std::move(original);
    REQUIRE(moved.size() == 2);
    REQUIRE(moved.contains(e1));
    REQUIRE(moved.contains(e2));
  }
}

TEST_CASE("ecs::map: Basic operations", "[ecs][map]")
{
  ouly::ecs::map<>    map;
  ouly::ecs::entity<> e1{42};
  ouly::ecs::entity<> e2{100};
  ouly::ecs::entity<> e3{200};

  SECTION("Emplace and size")
  {
    REQUIRE(map.size() == 0);

    auto idx1 = map.emplace(e1);
    REQUIRE(map.size() == 1);
    REQUIRE(idx1 == 0);

    auto idx2 = map.emplace(e2);
    REQUIRE(map.size() == 2);
    REQUIRE(idx2 == 1);

    auto idx3 = map.emplace(e3);
    REQUIRE(map.size() == 3);
    REQUIRE(idx3 == 2);
  }

  SECTION("Key lookup")
  {
    auto idx1 = map.emplace(e1);
    auto idx2 = map.emplace(e2);

    REQUIRE(map.key(e1) == idx1);
    REQUIRE(map.key(e2) == idx2);

    // Non-existent entity should return tombstone
    ouly::ecs::entity<> non_existent{999};
    auto                tombstone = std::numeric_limits<decltype(map)::size_type>::max();
    REQUIRE(map.key(non_existent) == tombstone);
  }

  SECTION("Contains check")
  {
    map.emplace(e1);
    map.emplace(e2);

    REQUIRE(map.contains(e1));
    REQUIRE(map.contains(e2));

    ouly::ecs::entity<> non_existent{999};
    REQUIRE_FALSE(map.contains(non_existent));
  }

  SECTION("Operator[] access")
  {
    auto idx1 = map.emplace(e1);
    auto idx2 = map.emplace(e2);

    REQUIRE(map[e1] == idx1);
    REQUIRE(map[e2] == idx2);
  }

  SECTION("At access")
  {
    auto idx1 = map.emplace(e1);
    auto idx2 = map.emplace(e2);

    REQUIRE(map.at(e1) == idx1);
    REQUIRE(map.at(e2) == idx2);
  }
}

TEST_CASE("ecs::map: Entity value access", "[ecs][map]")
{
  ouly::ecs::map<>    map;
  ouly::ecs::entity<> e1{42};
  ouly::ecs::entity<> e2{100};
  ouly::ecs::entity<> e3{200};

  map.emplace(e1);
  map.emplace(e2);
  map.emplace(e3);

  SECTION("Get entity at dense index")
  {
    REQUIRE(map.get_entity_at(0) == e1.value());
    REQUIRE(map.get_entity_at(1) == e2.value());
    REQUIRE(map.get_entity_at(2) == e3.value());
  }

  SECTION("Iteration over dense array")
  {
    std::vector<typename ouly::ecs::entity<>::size_type> entity_values;
    for (uint32_t i = 0; i < map.size(); ++i)
    {
      entity_values.push_back(map.get_entity_at(i));
    }

    REQUIRE(entity_values.size() == 3);
    REQUIRE(entity_values[0] == e1.value());
    REQUIRE(entity_values[1] == e2.value());
    REQUIRE(entity_values[2] == e3.value());
  }
}

TEST_CASE("ecs::map: Erase operations", "[ecs][map]")
{
  ouly::ecs::map<>    map;
  ouly::ecs::entity<> e1{42};
  ouly::ecs::entity<> e2{100};
  ouly::ecs::entity<> e3{200};

  map.emplace(e1);
  map.emplace(e2);
  map.emplace(e3);

  SECTION("Legacy erase method")
  {
    REQUIRE(map.size() == 3);

    // Erase middle element (e2 at index 1)
    auto swap_idx = map.erase(e2);
    REQUIRE(swap_idx == 1); // Index that was swapped
    REQUIRE(map.size() == 2);
    REQUIRE_FALSE(map.contains(e2));

    // e3 should have been moved to index 1 (swap-and-pop)
    REQUIRE(map.get_entity_at(1) == e3.value());
    REQUIRE(map.key(e3) == 1);
  }

  SECTION("New erase_and_get_swap_index method")
  {
    REQUIRE(map.size() == 3);

    // Erase middle element (e2 at index 1)
    auto swap_idx = map.erase_and_get_swap_index(e2);
    REQUIRE(swap_idx == 1);
    REQUIRE(map.size() == 2);
    REQUIRE_FALSE(map.contains(e2));

    // Verify the swap happened correctly
    REQUIRE(map.get_entity_at(1) == e3.value());
    REQUIRE(map.key(e3) == 1);
  }
}

TEST_CASE("ecs::map: External value array management", "[ecs][map]")
{
  ouly::ecs::map<>           map;
  std::vector<TestComponent> components;

  ouly::ecs::entity<> e1{42};
  ouly::ecs::entity<> e2{100};
  ouly::ecs::entity<> e3{200};

  SECTION("Parallel insertion")
  {
    // Insert entities and components in parallel
    auto idx1 = map.emplace(e1);
    components.resize(map.size());
    components[idx1] = TestComponent{1, "Component1"};

    auto idx2 = map.emplace(e2);
    components.resize(map.size());
    components[idx2] = TestComponent{2, "Component2"};

    auto idx3 = map.emplace(e3);
    components.resize(map.size());
    components[idx3] = TestComponent{3, "Component3"};

    REQUIRE(components.size() == 3);
    REQUIRE(components[map.key(e1)] == TestComponent{1, "Component1"});
    REQUIRE(components[map.key(e2)] == TestComponent{2, "Component2"});
    REQUIRE(components[map.key(e3)] == TestComponent{3, "Component3"});
  }

  SECTION("Manual swap after erase")
  {
    // Set up initial state
    auto idx1 = map.emplace(e1);
    auto idx2 = map.emplace(e2);
    auto idx3 = map.emplace(e3);

    components.resize(map.size());
    components[idx1] = TestComponent{1, "Component1"};
    components[idx2] = TestComponent{2, "Component2"};
    components[idx3] = TestComponent{3, "Component3"};

    // Erase e2 and manually handle component array
    auto swap_idx = map.erase_and_get_swap_index(e2);
    if (swap_idx < components.size() - 1)
    {
      components[swap_idx] = std::move(components.back());
    }
    components.pop_back();

    REQUIRE(components.size() == 2);
    REQUIRE(components[map.key(e1)] == TestComponent{1, "Component1"});
    REQUIRE(components[map.key(e3)] == TestComponent{3, "Component3"});
  }

  SECTION("Automatic swap with erase_and_swap_values")
  {
    // Set up initial state
    auto idx1 = map.emplace(e1);
    auto idx2 = map.emplace(e2);
    auto idx3 = map.emplace(e3);

    components.resize(map.size());
    components[idx1] = TestComponent{1, "Component1"};
    components[idx2] = TestComponent{2, "Component2"};
    components[idx3] = TestComponent{3, "Component3"};

    // Use convenience method
    map.erase_and_swap_values(e2, components);

    REQUIRE(components.size() == 2);
    REQUIRE(components[map.key(e1)] == TestComponent{1, "Component1"});
    REQUIRE(components[map.key(e3)] == TestComponent{3, "Component3"});
  }
}

TEST_CASE("ecs::map: Edge cases", "[ecs][map]")
{
  ouly::ecs::map<> map;

  SECTION("Erase last element")
  {
    ouly::ecs::entity<> e1{42};
    map.emplace(e1);

    auto swap_idx = map.erase_and_get_swap_index(e1);
    REQUIRE(swap_idx == 0);
    REQUIRE(map.size() == 0);
    REQUIRE(map.empty());
  }

  SECTION("Erase from single element map")
  {
    ouly::ecs::entity<> e1{42};
    std::vector<int>    values;

    map.emplace(e1);
    values.push_back(100);

    map.erase_and_swap_values(e1, values);
    REQUIRE(map.size() == 0);
    REQUIRE(values.size() == 0);
  }

  SECTION("Re-insert after erase")
  {
    ouly::ecs::entity<> e1{42};
    ouly::ecs::entity<> e2{100};

    auto idx1 = map.emplace(e1);
    auto idx2 = map.emplace(e2);

    REQUIRE(idx1 == 0);
    REQUIRE(idx2 == 1);

    map.erase_and_get_swap_index(e1);
    REQUIRE_FALSE(map.contains(e1));

    // Re-insert e1
    auto new_idx = map.emplace(e1);
    REQUIRE(map.contains(e1));
    REQUIRE(new_idx == 1); // Should get the next available index
  }
}

TEST_CASE("ecs::map: Clear and reset operations", "[ecs][map]")
{
  ouly::ecs::map<>    map;
  ouly::ecs::entity<> e1{42};
  ouly::ecs::entity<> e2{100};

  map.emplace(e1);
  map.emplace(e2);

  SECTION("Clear operation")
  {
    REQUIRE(map.size() == 2);
    map.clear();
    REQUIRE(map.size() == 0);
    REQUIRE(map.empty());
    REQUIRE_FALSE(map.contains(e1));
    REQUIRE_FALSE(map.contains(e2));
  }

  SECTION("Set max size")
  {
    map.set_max(1000);
    // Should not crash and should still work
    REQUIRE(map.size() == 2);
    REQUIRE(map.contains(e1));
    REQUIRE(map.contains(e2));
  }
}

TEST_CASE("ecs::map: Integrity validation", "[ecs][map]")
{
  ouly::ecs::map<>    map;
  ouly::ecs::entity<> e1{42};
  ouly::ecs::entity<> e2{100};
  ouly::ecs::entity<> e3{200};

  map.emplace(e1);
  map.emplace(e2);
  map.emplace(e3);

  SECTION("Validate integrity after operations")
  {
    // Should not assert/crash
    map.validate_integrity();

    // After erase
    map.erase_and_get_swap_index(e2);
    map.validate_integrity();

    // After clear
    map.clear();
    map.validate_integrity();
  }
}

TEST_CASE("ecs::map: Multiple erases with external arrays", "[ecs][map]")
{
  ouly::ecs::map<>           map;
  std::vector<TestComponent> components;

  // Create multiple entities
  std::vector<ouly::ecs::entity<>> entities;
  for (uint32_t i = 0; i < 10; ++i)
  {
    entities.emplace_back(i * 10);
  }

  // Insert all entities with components
  for (uint32_t i = 0; i < static_cast<uint32_t>(entities.size()); ++i)
  {
    auto idx = map.emplace(entities[i]);
    components.resize(map.size());
    components[idx] = TestComponent{static_cast<int>(i), "Component" + std::to_string(i)};
  }

  REQUIRE(map.size() == 10);
  REQUIRE(components.size() == 10);

  SECTION("Remove every other entity")
  {
    // Remove entities at indices 1, 3, 5, 7, 9
    for (int32_t i = 9; i >= 1; i -= 2)
    {
      if (i < 0)
      {
        continue;
      }

      map.erase_and_swap_values(entities[static_cast<uint32_t>(i)], components);
    }

    REQUIRE(map.size() == 5);
    REQUIRE(components.size() == 5);

    // Verify remaining entities are still correct
    for (uint32_t i = 0; i < 10; i += 2)
    {
      REQUIRE(map.contains(entities[i]));
      auto idx = map.key(entities[i]);
      REQUIRE(idx < components.size());
    }
  }
}

TEST_CASE("ecs::map: Performance characteristics", "[ecs][map]")
{
  ouly::ecs::map<> map;

  SECTION("Large number of insertions")
  {
    const uint32_t                   count = 10000;
    std::vector<ouly::ecs::entity<>> entities;
    entities.reserve(count);

    for (uint32_t i = 0; i < count; ++i)
    {
      entities.emplace_back(i);
      map.emplace(entities.back());
    }

    REQUIRE(map.size() == count);

    // Verify all entities are accessible
    for (size_t i = 0; i < count; ++i)
    {
      REQUIRE(map.contains(entities[i]));
      REQUIRE(map.key(entities[i]) == i);
    }
  }
}
// NOLINTEND