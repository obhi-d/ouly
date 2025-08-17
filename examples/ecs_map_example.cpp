/**
 * @file ecs_map_example.cpp
 * @brief Example demonstrating usage of ouly::ecs::map for entity-component systems
 *
 * This example shows how to use the ecs::map class to efficiently manage
 * entities and their components in a typical ECS architecture.
 */

#include "ouly/ecs/entity.hpp"
#include "ouly/ecs/map.hpp"
#include <iostream>
#include <string>
#include <vector>

// NOLINTBEGIN
// Example component types
struct Position
{
  float x, y, z;
  Position(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}

  void print() const
  {
    std::cout << "Position(" << x << ", " << y << ", " << z << ")";
  }
};

struct Velocity
{
  float dx, dy, dz;
  Velocity(float dx = 0, float dy = 0, float dz = 0) : dx(dx), dy(dy), dz(dz) {}

  void print() const
  {
    std::cout << "Velocity(" << dx << ", " << dy << ", " << dz << ")";
  }
};

struct Name
{
  std::string value;
  Name() noexcept = default;
  Name(std::string v) : value(std::move(v)) {}

  void print() const
  {
    std::cout << "Name(\"" << value << "\")";
  }
};

int main()
{
  std::cout << "=== OULY ECS Map Example ===\n\n";

  // Create entity map and component arrays
  ouly::ecs::map<>      entity_map;
  std::vector<Position> positions;
  std::vector<Velocity> velocities;
  std::vector<Name>     names;

  // Create some entities
  ouly::ecs::entity<> player{100};
  ouly::ecs::entity<> enemy1{200};
  ouly::ecs::entity<> enemy2{300};
  ouly::ecs::entity<> powerup{400};

  std::cout << "1. Creating entities and adding components:\n";

  // Add entities and their components
  auto player_idx = entity_map.emplace(player);
  positions.resize(entity_map.size());
  velocities.resize(entity_map.size());
  names.resize(entity_map.size());
  positions[player_idx]  = Position{10.0f, 5.0f, 0.0f};
  velocities[player_idx] = Velocity{1.0f, 0.0f, 0.0f};
  names[player_idx]      = Name{"Player"};
  std::cout << "  Added Player at dense index " << player_idx << "\n";

  auto enemy1_idx = entity_map.emplace(enemy1);
  positions.resize(entity_map.size());
  velocities.resize(entity_map.size());
  names.resize(entity_map.size());
  positions[enemy1_idx]  = Position{-5.0f, 10.0f, 0.0f};
  velocities[enemy1_idx] = Velocity{-0.5f, -1.0f, 0.0f};
  names[enemy1_idx]      = Name{"Enemy1"};
  std::cout << "  Added Enemy1 at dense index " << enemy1_idx << "\n";

  auto enemy2_idx = entity_map.emplace(enemy2);
  positions.resize(entity_map.size());
  velocities.resize(entity_map.size());
  names.resize(entity_map.size());
  positions[enemy2_idx]  = Position{0.0f, -8.0f, 2.0f};
  velocities[enemy2_idx] = Velocity{0.8f, 0.3f, -0.1f};
  names[enemy2_idx]      = Name{"Enemy2"};
  std::cout << "  Added Enemy2 at dense index " << enemy2_idx << "\n";

  auto powerup_idx = entity_map.emplace(powerup);
  positions.resize(entity_map.size());
  velocities.resize(entity_map.size());
  names.resize(entity_map.size());
  positions[powerup_idx]  = Position{15.0f, 0.0f, 1.0f};
  velocities[powerup_idx] = Velocity{0.0f, 0.0f, 0.5f};
  names[powerup_idx]      = Name{"PowerUp"};
  std::cout << "  Added PowerUp at dense index " << powerup_idx << "\n";

  std::cout << "\n2. Current state (total entities: " << entity_map.size() << "):\n";

  // Demonstrate efficient iteration over all entities
  for (uint32_t i = 0; i < entity_map.size(); ++i)
  {
    auto                entity_value = entity_map.get_entity_at(i);
    ouly::ecs::entity<> entity(entity_value);

    std::cout << "  [" << i << "] Entity " << entity.value() << ": ";
    names[i].print();
    std::cout << ", ";
    positions[i].print();
    std::cout << ", ";
    velocities[i].print();
    std::cout << "\n";
  }

  std::cout << "\n3. Looking up specific entities:\n";

  // Demonstrate entity lookup
  if (entity_map.contains(player))
  {
    auto idx = entity_map.key(player);
    std::cout << "  Player found at index " << idx << ": ";
    names[idx].print();
    std::cout << "\n";
  }

  if (entity_map.contains(enemy1))
  {
    auto idx = entity_map[enemy1]; // Same as entity_map.key(enemy1)
    std::cout << "  Enemy1 found at index " << idx << ": ";
    positions[idx].print();
    std::cout << "\n";
  }

  std::cout << "\n4. Removing an entity (Enemy1):\n";

  // Remove enemy1 using the convenient automatic method
  std::cout << "  Before removal: " << entity_map.size() << " entities\n";
  entity_map.erase_and_swap_values(enemy1, positions);
  entity_map.erase_and_swap_values(enemy1, velocities);
  entity_map.erase_and_swap_values(enemy1, names);
  std::cout << "  After removal: " << entity_map.size() << " entities\n";

  std::cout << "  Enemy1 still exists in map: " << (entity_map.contains(enemy1) ? "true" : "false") << "\n";

  std::cout << "\n5. State after removal:\n";

  // Show the state after removal - note how the arrays stay packed
  for (uint32_t i = 0; i < entity_map.size(); ++i)
  {
    auto                entity_value = entity_map.get_entity_at(i);
    ouly::ecs::entity<> entity(entity_value);

    std::cout << "  [" << i << "] Entity " << entity.value() << ": ";
    names[i].print();
    std::cout << ", ";
    positions[i].print();
    std::cout << ", ";
    velocities[i].print();
    std::cout << "\n";
  }

  std::cout << "\n6. Manual removal with swap control:\n";

  // Demonstrate manual removal for performance-critical scenarios
  std::cout << "  Removing PowerUp manually...\n";
  auto swap_idx = entity_map.erase_and_get_swap_index(powerup);
  std::cout << "  Swap index returned: " << swap_idx << "\n";

  // Manually handle the swap for each component array
  if (swap_idx < positions.size() - 1)
  {
    positions[swap_idx]  = std::move(positions.back());
    velocities[swap_idx] = std::move(velocities.back());
    names[swap_idx]      = std::move(names.back());
  }
  positions.pop_back();
  velocities.pop_back();
  names.pop_back();

  std::cout << "\n7. Final state:\n";

  for (uint32_t i = 0; i < entity_map.size(); ++i)
  {
    auto                entity_value = entity_map.get_entity_at(i);
    ouly::ecs::entity<> entity(entity_value);

    std::cout << "  [" << i << "] Entity " << entity.value() << ": ";
    names[i].print();
    std::cout << ", ";
    positions[i].print();
    std::cout << ", ";
    velocities[i].print();
    std::cout << "\n";
  }

  std::cout << "\n=== Performance Benefits ===\n";
  std::cout << "- Sparse entity IDs (100, 200, 300, 400) mapped to dense indices (0, 1, 2, 3)\n";
  std::cout << "- Component arrays remain packed for cache-friendly iteration\n";
  std::cout << "- O(1) entity lookup and removal\n";
  std::cout << "- Memory usage scales with number of active entities, not entity ID range\n";

  return 0;
}
// NOLINTEND