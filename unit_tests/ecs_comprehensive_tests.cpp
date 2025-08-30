#include "catch2/catch_all.hpp"
#include "ouly/ecs/collection.hpp"
#include "ouly/ecs/components.hpp"
#include "ouly/ecs/map.hpp"
#include "ouly/ecs/registry.hpp"
#include "test_common.hpp"
#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// NOLINTBEGIN

// Test configurations for different storage strategies
struct sparse_config
{
  static constexpr uint32_t pool_size_v  = 2;
  static constexpr bool     use_sparse_v = true;
};

struct dense_config
{
  static constexpr uint32_t pool_size_v  = 2;
  static constexpr bool     use_sparse_v = false;
};

struct direct_mapping_config
{
  static constexpr bool use_direct_mapping_v = true;
};

struct indirect_mapping_config
{
  static constexpr bool use_direct_mapping_v = false;
};

struct large_pool_config
{
  static constexpr uint32_t pool_size_v = 1024;
};

struct small_pool_config
{
  static constexpr uint32_t pool_size_v = 2;
};

// Custom types for testing different scenarios
struct non_trivial_type
{
  std::string data;
  int         counter = 0;

  non_trivial_type() = default;
  non_trivial_type(std::string s, int c) : data(std::move(s)), counter(c) {}
  non_trivial_type(const non_trivial_type&)            = default;
  non_trivial_type(non_trivial_type&&)                 = default;
  non_trivial_type& operator=(const non_trivial_type&) = default;
  non_trivial_type& operator=(non_trivial_type&&)      = default;

  bool operator==(const non_trivial_type& other) const noexcept
  {
    return data == other.data && counter == other.counter;
  }
};

struct move_only_type
{
  std::unique_ptr<int> data;

  move_only_type() : data(std::make_unique<int>(0)) {}
  move_only_type(int value) : data(std::make_unique<int>(value)) {}
  move_only_type(const move_only_type&)            = delete;
  move_only_type(move_only_type&&)                 = default;
  move_only_type& operator=(const move_only_type&) = delete;
  move_only_type& operator=(move_only_type&&)      = default;

  bool operator==(const move_only_type& other) const noexcept
  {
    return data && other.data && *data == *other.data;
  }
};

struct self_referencing_type
{
  uint32_t    self_index = 0;
  std::string name;

  self_referencing_type() = default;
  self_referencing_type(const std::string& n) : name(n) {}
};

namespace ouly::cfg
{
template <>
struct member<&self_referencing_type::self_index>
{
  using class_type  = self_referencing_type;
  using member_type = uint32_t;

  static auto get(class_type& obj) noexcept -> member_type&
  {
    return obj.self_index;
  }

  static auto get(const class_type& obj) noexcept -> const member_type&
  {
    return obj.self_index;
  }
};
} // namespace ouly::cfg

TEMPLATE_TEST_CASE("registry: basic entity creation and destruction", "[registry][basic]", ouly::ecs::registry<>,
                   ouly::ecs::rxregistry<>)
{
  TestType registry;

  SECTION("Empty registry state")
  {
    REQUIRE(registry.max_size() == 1); // starts at 1
  }

  SECTION("Create single entity")
  {
    auto entity = registry.emplace();
    REQUIRE(entity.get() == 1);
    REQUIRE(registry.max_size() == 2);
  }

  SECTION("Create multiple entities")
  {
    std::vector<typename TestType::type> entities;
    for (int i = 0; i < 10; ++i)
    {
      entities.push_back(registry.emplace());
    }

    REQUIRE(registry.max_size() == 11);

    // Check entities have consecutive indices
    for (size_t i = 0; i < entities.size(); ++i)
    {
      REQUIRE(entities[i].get() == i + 1);
    }
  }

  SECTION("Entity reuse after deletion")
  {
    auto e1 = registry.emplace();
    auto e2 = registry.emplace();
    auto e3 = registry.emplace();

    REQUIRE(registry.max_size() == 4);

    // Delete middle entity
    registry.erase(e2);

    // Create new entity - should reuse e2's slot
    auto e4 = registry.emplace();
    REQUIRE(e4.get() == e2.get());

    if constexpr (!std::is_same_v<typename TestType::type::revision_type, void>)
    {
      REQUIRE(e4.revision() != e2.revision());
    }
  }
}

TEMPLATE_TEST_CASE("registry: revision tracking", "[registry][revision]", ouly::ecs::rxregistry<>)
{
  TestType registry;

  SECTION("Revision increment on reuse")
  {
    auto e1                = registry.emplace();
    auto original_revision = e1.revision();

    registry.erase(e1);
    auto e2 = registry.emplace();

    REQUIRE(e2.get() == e1.get());
    REQUIRE(e2.revision() == original_revision + 1);
    REQUIRE(registry.is_valid(e1) == false);
    REQUIRE(registry.is_valid(e2) == true);
  }

  SECTION("Multiple reuses increment revision correctly")
  {
    auto e1   = registry.emplace();
    auto slot = e1.get();

    for (int i = 0; i < 5; ++i)
    {
      registry.erase(e1);
      e1 = registry.emplace();
      REQUIRE(e1.get() == slot);
      REQUIRE(registry.get_revision(slot) == i + 1);
    }
  }

  SECTION("get_revision for non-existent entities")
  {
    // Should return 0 for out-of-bounds indices
    REQUIRE(registry.get_revision(1000) == 0);
  }
}

TEMPLATE_TEST_CASE("registry: for_each_index functionality", "[registry][iteration]", ouly::ecs::registry<>,
                   ouly::ecs::rxregistry<>)
{
  TestType registry;

  SECTION("Empty registry iteration")
  {
    int count = 0;
    registry.for_each_index(
     [&count](auto)
     {
       count++;
     });
    REQUIRE(count == 0);
  }

  SECTION("Simple iteration over entities")
  {
    std::vector<typename TestType::type> entities;
    for (int i = 0; i < 5; ++i)
    {
      entities.push_back(registry.emplace());
    }

    std::vector<uint32_t> visited_indices;
    registry.for_each_index(
     [&visited_indices](uint32_t idx)
     {
       visited_indices.push_back(idx);
     });

    REQUIRE(visited_indices.size() == 5);
    std::sort(visited_indices.begin(), visited_indices.end());
    for (size_t i = 0; i < visited_indices.size(); ++i)
    {
      REQUIRE(visited_indices[i] == i + 1);
    }
  }

  SECTION("Iteration with gaps from deletions")
  {
    auto e1 = registry.emplace(); // index 1
    auto e2 = registry.emplace(); // index 2
    auto e3 = registry.emplace(); // index 3
    auto e4 = registry.emplace(); // index 4

    registry.erase(e2); // Remove index 2
    registry.erase(e4); // Remove index 4

    std::vector<uint32_t> visited_indices;
    registry.for_each_index(
     [&visited_indices](uint32_t idx)
     {
       visited_indices.push_back(idx);
     });

    REQUIRE(visited_indices.size() == 2);
    std::sort(visited_indices.begin(), visited_indices.end());
    REQUIRE(visited_indices[0] == 1);
    REQUIRE(visited_indices[1] == 3);
  }

  SECTION("Const iteration")
  {
    const auto& const_registry = registry;

    auto e1 = registry.emplace();
    auto e2 = registry.emplace();

    int count = 0;
    const_registry.for_each_index(
     [&count](uint32_t)
     {
       count++;
     });
    REQUIRE(count == 2);
  }
}

TEMPLATE_TEST_CASE("registry: free list management", "[registry][free_list]", ouly::ecs::registry<>,
                   ouly::ecs::rxregistry<>)
{
  TestType registry;

  SECTION("Free list LIFO behavior")
  {
    auto e1 = registry.emplace();
    auto e2 = registry.emplace();
    auto e3 = registry.emplace();

    registry.erase(e1);
    registry.erase(e2);
    registry.erase(e3);

    // Should reuse in LIFO order
    auto new_e1 = registry.emplace();
    auto new_e2 = registry.emplace();
    auto new_e3 = registry.emplace();

    REQUIRE(new_e1.get() == e3.get()); // Last erased, first reused
    REQUIRE(new_e2.get() == e2.get());
    REQUIRE(new_e3.get() == e1.get());
  }

  SECTION("Shrink functionality")
  {
    auto e1 = registry.emplace();
    auto e2 = registry.emplace();

    registry.erase(e1);
    registry.erase(e2);

    registry.shrink();

    // Should still work after shrinking
    auto new_e = registry.emplace();
    REQUIRE(new_e.get() == e2.get());
  }
}

TEMPLATE_TEST_CASE("components: basic operations with different storage strategies", "[components][storage]",
                   sparse_config, dense_config)
{
  using registry_type   = ouly::ecs::rxregistry<>;
  using components_type = ouly::ecs::components<int, ouly::ecs::rxentity<>, TestType>;

  registry_type   registry;
  components_type components;

  SECTION("Empty components")
  {
    REQUIRE(components.size() == 0);
    REQUIRE(components.range() == 0);
  }

  SECTION("Basic emplace_at operations")
  {
    auto e1 = registry.emplace();
    auto e2 = registry.emplace();

    components.set_max(registry.max_size());

    auto& val1 = components.emplace_at(e1, 42);
    REQUIRE(val1 == 42);

    auto& val2 = components.emplace_at(e2, 84);
    REQUIRE(val2 == 84);
    REQUIRE(components.at(e1) == 42);
    REQUIRE(components.at(e2) == 84);
    REQUIRE(components.contains(e1) == true);
    REQUIRE(components.contains(e2) == true);
  }

  SECTION("Component modification")
  {
    auto e1 = registry.emplace();
    components.set_max(registry.max_size());

    components.emplace_at(e1, 42);
    REQUIRE(components.at(e1) == 42);

    components.at(e1) = 100;
    REQUIRE(components.at(e1) == 100);

    auto opt_ref = components.find(e1);
    REQUIRE(opt_ref.has_value());
    REQUIRE(opt_ref.get() == 100);
  }

  SECTION("Component replacement")
  {
    auto e1 = registry.emplace();
    components.set_max(registry.max_size());

    components.emplace_at(e1, 42);
    auto& replaced = components.replace(e1, 100);
    REQUIRE(replaced == 100);
    REQUIRE(components.at(e1) == 100);
  }

  SECTION("get_ref creates component if not exists")
  {
    auto e1 = registry.emplace();
    components.set_max(registry.max_size());

    REQUIRE(components.contains(e1) == false);
    auto& ref = components.get_ref(e1);
    REQUIRE(components.contains(e1) == true);
    REQUIRE(ref == 0); // Default constructed

    ref = 42;
    REQUIRE(components.at(e1) == 42);
  }
}

TEMPLATE_TEST_CASE("components: direct vs indirect mapping", "[components][mapping]", direct_mapping_config,
                   indirect_mapping_config)
{
  using registry_type   = ouly::ecs::rxregistry<>;
  using components_type = ouly::ecs::components<std::string, ouly::ecs::rxentity<>, TestType>;

  registry_type   registry;
  components_type components;

  auto e1 = registry.emplace();
  auto e2 = registry.emplace();
  auto e3 = registry.emplace();

  components.set_max(registry.max_size());

  SECTION("Basic operations work with both mapping strategies")
  {
    components.emplace_at(e1, "first");
    components.emplace_at(e2, "second");
    components.emplace_at(e3, "third");

    REQUIRE(components.at(e1) == "first");
    REQUIRE(components.at(e2) == "second");
    REQUIRE(components.at(e3) == "third");
    REQUIRE(components.size() == 3);
  }

  SECTION("Component deletion preserves remaining components")
  {
    components.emplace_at(e1, "first");
    components.emplace_at(e2, "second");
    components.emplace_at(e3, "third");

    components.erase(e2);

    REQUIRE(components.contains(e1) == true);
    REQUIRE(components.contains(e2) == false);
    REQUIRE(components.contains(e3) == true);
    REQUIRE(components.at(e1) == "first");
    REQUIRE(components.at(e3) == "third");

    if constexpr (!TestType::use_direct_mapping_v)
    {
      REQUIRE(components.size() == 2); // Dense storage shrinks
    }
  }

  SECTION("Key access for indirect mapping")
  {
    if constexpr (!TestType::use_direct_mapping_v)
    {
      components.emplace_at(e1, "first");
      components.emplace_at(e2, "second");

      auto key1 = components.key(e1);
      auto key2 = components.key(e2);

      REQUIRE(key1 != std::numeric_limits<uint32_t>::max()); // Not tombstone
      REQUIRE(key2 != std::numeric_limits<uint32_t>::max());
      REQUIRE(key1 != key2);
    }
  }
}

TEST_CASE("components: complex component types", "[components][complex_types]")
{
  using registry_type = ouly::ecs::rxregistry<>;

  SECTION("Non-trivial types")
  {
    registry_type                                                  registry;
    ouly::ecs::components<non_trivial_type, ouly::ecs::rxentity<>> components;

    auto e1 = registry.emplace();
    components.set_max(registry.max_size());

    components.emplace_at(e1, "test", 42);
    auto& comp = components.at(e1);
    REQUIRE(comp.data == "test");
    REQUIRE(comp.counter == 42);

    // Test modification
    comp.data    = "modified";
    comp.counter = 100;
    REQUIRE(components.at(e1).data == "modified");
    REQUIRE(components.at(e1).counter == 100);
  }

  SECTION("Move-only types")
  {
    registry_type                                                registry;
    ouly::ecs::components<move_only_type, ouly::ecs::rxentity<>> components;

    auto e1 = registry.emplace();
    components.set_max(registry.max_size());

    components.emplace_at(e1, 42);
    REQUIRE(*components.at(e1).data == 42);

    // Test replacement with move
    move_only_type new_val(100);
    components.replace(e1, std::move(new_val));
    REQUIRE(*components.at(e1).data == 100);
  }

  SECTION("Self-referencing types")
  {
    registry_type registry;
    ouly::ecs::components<self_referencing_type, ouly::ecs::rxentity<>,
                          ouly::config<ouly::cfg::self_index_member<&self_referencing_type::self_index>>>
     components;

    auto e1 = registry.emplace();
    auto e2 = registry.emplace();
    components.set_max(registry.max_size());

    components.emplace_at(e1, "first");
    components.emplace_at(e2, "second");

    REQUIRE(components.at(e1).name == "first");
    REQUIRE(components.at(e2).name == "second");

    // Test deletion by entity
    components.erase(e1);

    REQUIRE(components.contains(e1) == false);
    REQUIRE(components.contains(e2) == true);
  }
}

TEST_CASE("components: iteration and for_each", "[components][iteration]")
{
  using registry_type   = ouly::ecs::rxregistry<>;
  using components_type = ouly::ecs::components<std::string, ouly::ecs::rxentity<>>;

  registry_type   registry;
  components_type components;

  auto e1 = registry.emplace();
  auto e2 = registry.emplace();
  auto e3 = registry.emplace();

  components.set_max(registry.max_size());
  components.emplace_at(e1, "first");
  components.emplace_at(e2, "second");
  components.emplace_at(e3, "third");

  SECTION("for_each with entity and value")
  {
    std::vector<std::pair<uint32_t, std::string>> collected;

    components.for_each(
     [&collected](typename registry_type::type entity, const std::string& value)
     {
       collected.emplace_back(entity.get(), value);
     });

    REQUIRE(collected.size() == 3);

    // Sort by entity index for consistent checking
    std::sort(collected.begin(), collected.end());
    REQUIRE(collected[0].first == e1.get());
    REQUIRE(collected[0].second == "first");
    REQUIRE(collected[1].first == e2.get());
    REQUIRE(collected[1].second == "second");
    REQUIRE(collected[2].first == e3.get());
    REQUIRE(collected[2].second == "third");
  }

  SECTION("for_each value only")
  {
    std::vector<std::string> values;
    components.for_each(
     [&values](const std::string& value)
     {
       values.push_back(value);
     });

    REQUIRE(values.size() == 3);
    // Can't guarantee order without entity information
    std::sort(values.begin(), values.end());
    REQUIRE(values[0] == "first");
    REQUIRE(values[1] == "second");
    REQUIRE(values[2] == "third");
  }

  SECTION("for_each with range")
  {
    std::vector<std::string> values;
    components.for_each(0, 2,
                        [&values](const std::string& value)
                        {
                          values.push_back(value);
                        });

    REQUIRE(values.size() == 2);
  }

  SECTION("const for_each")
  {
    const auto& const_components = components;
    int         count            = 0;

    const_components.for_each(
     [&count](typename registry_type::type, const std::string&)
     {
       count++;
     });

    REQUIRE(count == 3);
  }
}

TEST_CASE("components: edge cases and error conditions", "[components][edge_cases]")
{
  using registry_type   = ouly::ecs::rxregistry<>;
  using components_type = ouly::ecs::components<int, ouly::ecs::rxentity<>>;

  registry_type   registry;
  components_type components;

  SECTION("Operations on non-existent entities")
  {
    auto e1 = registry.emplace();
    components.set_max(registry.max_size());

    // Find should return empty optional
    auto opt = components.find(e1);
    REQUIRE(opt.has_value() == false);

    // contains should return false
    REQUIRE(components.contains(e1) == false);
  }

  SECTION("Large entity indices")
  {
    // Create entity with large index
    for (int i = 0; i < 1000; ++i)
    {
      registry.emplace();
    }

    auto large_entity = registry.emplace();
    components.set_max(registry.max_size());

    components.emplace_at(large_entity, 42);
    REQUIRE(components.at(large_entity) == 42);
  }

  SECTION("Multiple deletions and additions")
  {
    std::vector<typename registry_type::type> entities;
    components.set_max(1000);

    // Create many entities
    for (int i = 0; i < 100; ++i)
    {
      auto e = registry.emplace();
      entities.push_back(e);
      components.emplace_at(e, i);
    }

    // Delete every other entity
    for (size_t i = 0; i < entities.size(); i += 2)
    {
      components.erase(entities[i]);
    }

    // Verify remaining entities
    for (size_t i = 1; i < entities.size(); i += 2)
    {
      REQUIRE(components.contains(entities[i]) == true);
      REQUIRE(components.at(entities[i]) == static_cast<int>(i));
    }

    // Add new entities
    for (int i = 0; i < 50; ++i)
    {
      auto e = registry.emplace();
      components.emplace_at(e, 1000 + i);
      REQUIRE(components.at(e) == 1000 + i);
    }
  }
}

TEST_CASE("components: memory and resource management", "[components][memory]")
{
  using registry_type = ouly::ecs::rxregistry<>;

  SECTION("Destruction tracking")
  {
    tracker track('A');

    {
      registry_type                                                 registry;
      ouly::ecs::components<destroy_tracker, ouly::ecs::rxentity<>> components;

      auto e1 = registry.emplace();
      auto e2 = registry.emplace();
      components.set_max(registry.max_size());

      components.emplace_at(e1, track);
      components.emplace_at(e2, track);

      REQUIRE(track.tracking == 2);

      components.erase(e1);
      REQUIRE(track.tracking == 1);

      // Components destructor should clean up remaining
    }

    REQUIRE(track.tracking == 0);
  }

  SECTION("Clear and validate_integrity")
  {
    registry_type                                                            registry;
    ouly::ecs::components<std::string, ouly::ecs::rxentity<>, sparse_config> components;

    auto e1 = registry.emplace();
    auto e2 = registry.emplace();
    components.set_max(registry.max_size());

    components.emplace_at(e1, "first");
    components.emplace_at(e2, "second");

    REQUIRE(components.size() == 2);

    components.clear();
    REQUIRE(components.size() == 0);
    REQUIRE(components.contains(e1) == false);
    REQUIRE(components.contains(e2) == false);

    // Should be able to add again
    components.emplace_at(e1, "new_first");
    REQUIRE(components.at(e1) == "new_first");
  }
}

TEMPLATE_TEST_CASE("collection: basic operations", "[collection][basic]", ouly::ecs::entity<>, ouly::ecs::rxentity<>)
{
  using collection_type = ouly::ecs::collection<TestType>;

  collection_type collection;

  SECTION("Empty collection")
  {
    REQUIRE(collection.size() == 0);
    REQUIRE(collection.empty() == true);
  }

  SECTION("Entity insertion")
  {
    TestType e1(1);
    TestType e2(2);
    TestType e3(3);

    collection.emplace(e1);
    collection.emplace(e2);
    collection.emplace(e3);

    REQUIRE(collection.size() == 3);
    REQUIRE(collection.empty() == false);
    REQUIRE(collection.contains(e1) == true);
    REQUIRE(collection.contains(e2) == true);
    REQUIRE(collection.contains(e3) == true);
  }

  SECTION("Entity removal")
  {
    TestType e1(1);
    TestType e2(2);
    TestType e3(3);

    collection.emplace(e1);
    collection.emplace(e2);
    collection.emplace(e3);

    collection.erase(e2);

    REQUIRE(collection.size() == 2);
    REQUIRE(collection.contains(e1) == true);
    REQUIRE(collection.contains(e2) == false);
    REQUIRE(collection.contains(e3) == true);
  }

  SECTION("Duplicate insertion")
  {
    TestType e1(1);

    collection.emplace(e1);
    collection.emplace(e1); // Should not add duplicate

    REQUIRE(collection.size() == 1);
    REQUIRE(collection.contains(e1) == true);
  }
}

TEST_CASE("collection: iteration with components", "[collection][iteration]")
{
  using registry_type   = ouly::ecs::rxregistry<>;
  using collection_type = ouly::ecs::collection<ouly::ecs::rxentity<>>;
  using components_type =
   ouly::ecs::components<int, ouly::ecs::rxentity<>, ouly::config<ouly::cfg::use_direct_mapping>>;

  registry_type   registry;
  collection_type collection;
  components_type data;

  auto e1 = registry.emplace();
  auto e2 = registry.emplace();
  auto e3 = registry.emplace();
  auto e4 = registry.emplace();

  data.set_max(registry.max_size());

  // Add entities to collection
  collection.emplace(e1);
  collection.emplace(e2);
  collection.emplace(e4);

  // Add components
  data.emplace_at(e1, 10);
  data.emplace_at(e2, 20);
  data.emplace_at(e3, 30); // Not in collection
  data.emplace_at(e4, 40);

  SECTION("for_each with collection")
  {
    std::vector<int> values;

    collection.for_each(data,
                        [&values](ouly::ecs::rxentity<> /* entity */, const int& value)
                        {
                          values.push_back(value);
                        });

    REQUIRE(values.size() == 3);
    std::sort(values.begin(), values.end());
    REQUIRE(values[0] == 10);
    REQUIRE(values[1] == 20);
    REQUIRE(values[2] == 40);
  }

  SECTION("Collection membership filtering")
  {
    int sum = 0;
    data.for_each(
     [&sum, &collection](typename registry_type::type entity, const int& value)
     {
       if (collection.contains(entity))
       {
         sum += value;
       }
     });

    REQUIRE(sum == 70); // 10 + 20 + 40
  }
}

TEST_CASE("collection: large scale operations", "[collection][scale]")
{
  using collection_type = ouly::ecs::collection<ouly::ecs::entity<>>;

  collection_type collection;

  SECTION("Large number of entities")
  {
    std::vector<ouly::ecs::entity<>> entities;

    for (uint32_t i = 1; i <= 10000; ++i)
    {
      ouly::ecs::entity<> e(i);
      entities.push_back(e);
      collection.emplace(e);
    }

    REQUIRE(collection.size() == 10000);

    // Check all entities are present
    for (const auto& e : entities)
    {
      REQUIRE(collection.contains(e) == true);
    }

    // Remove every other entity
    for (size_t i = 0; i < entities.size(); i += 2)
    {
      collection.erase(entities[i]);
    }

    REQUIRE(collection.size() == 5000);

    // Check correct entities remain
    for (size_t i = 0; i < entities.size(); ++i)
    {
      bool should_exist = (i % 2 == 1);
      REQUIRE(collection.contains(entities[i]) == should_exist);
    }
  }
}

TEST_CASE("collection: copy and move operations", "[collection][copy_move]")
{
  using collection_type = ouly::ecs::collection<ouly::ecs::entity<>>;

  SECTION("Copy constructor")
  {
    collection_type original;

    ouly::ecs::entity<> e1(1);
    ouly::ecs::entity<> e2(2);

    original.emplace(e1);
    original.emplace(e2);

    collection_type copy(original);

    REQUIRE(copy.size() == 2);
    REQUIRE(copy.contains(e1) == true);
    REQUIRE(copy.contains(e2) == true);

    // Modify original, copy should be unaffected
    original.erase(e1);
    REQUIRE(copy.contains(e1) == true);
  }

  SECTION("Copy assignment")
  {
    collection_type original;
    collection_type target;

    ouly::ecs::entity<> e1(1);
    ouly::ecs::entity<> e2(2);
    ouly::ecs::entity<> e3(3);

    original.emplace(e1);
    original.emplace(e2);

    target.emplace(e3);

    target = original;

    REQUIRE(target.size() == 2);
    REQUIRE(target.contains(e1) == true);
    REQUIRE(target.contains(e2) == true);
    REQUIRE(target.contains(e3) == false);
  }

  SECTION("Move constructor")
  {
    collection_type original;

    ouly::ecs::entity<> e1(1);
    ouly::ecs::entity<> e2(2);

    original.emplace(e1);
    original.emplace(e2);

    collection_type moved(std::move(original));

    REQUIRE(moved.size() == 2);
    REQUIRE(moved.contains(e1) == true);
    REQUIRE(moved.contains(e2) == true);
  }
}

TEST_CASE("map: basic entity mapping", "[map][basic]")
{
  using map_type = ouly::ecs::map<ouly::ecs::entity<>>;

  map_type entity_map;

  SECTION("Empty map")
  {
    REQUIRE(entity_map.size() == 0);
    REQUIRE(entity_map.empty() == true);
  }

  SECTION("Entity insertion and lookup")
  {
    ouly::ecs::entity<> e1(10);
    ouly::ecs::entity<> e2(20);
    ouly::ecs::entity<> e3(30);

    auto idx1 = entity_map.emplace(e1);
    auto idx2 = entity_map.emplace(e2);
    auto idx3 = entity_map.emplace(e3);

    REQUIRE(entity_map.size() == 3);
    REQUIRE(idx1 == 0);
    REQUIRE(idx2 == 1);
    REQUIRE(idx3 == 2);

    REQUIRE(entity_map.key(e1) == idx1);
    REQUIRE(entity_map.key(e2) == idx2);
    REQUIRE(entity_map.key(e3) == idx3);

    REQUIRE(entity_map[e1] == idx1);
    REQUIRE(entity_map[e2] == idx2);
    REQUIRE(entity_map[e3] == idx3);

    REQUIRE(entity_map.contains(e1) == true);
    REQUIRE(entity_map.contains(e2) == true);
    REQUIRE(entity_map.contains(e3) == true);
  }

  SECTION("Entity value retrieval")
  {
    ouly::ecs::entity<> e1(10);
    ouly::ecs::entity<> e2(20);

    entity_map.emplace(e1);
    entity_map.emplace(e2);

    REQUIRE(entity_map.entity_at(0) == e1);
    REQUIRE(entity_map.entity_at(1) == e2);
  }

  SECTION("Non-existent entity lookup")
  {
    ouly::ecs::entity<> e1(10);
    ouly::ecs::entity<> e_nonexistent(999);

    entity_map.emplace(e1);

    REQUIRE(entity_map.contains(e_nonexistent) == false);
    auto key = entity_map.key(e_nonexistent);
    REQUIRE(key == std::numeric_limits<uint32_t>::max()); // tombstone
  }
}

TEST_CASE("map: entity removal and swapping", "[map][removal]")
{
  using map_type = ouly::ecs::map<ouly::ecs::entity<>>;

  map_type entity_map;

  SECTION("Basic entity removal")
  {
    ouly::ecs::entity<> e1(10);
    ouly::ecs::entity<> e2(20);
    ouly::ecs::entity<> e3(30);

    entity_map.emplace(e1);
    entity_map.emplace(e2);
    entity_map.emplace(e3);

    auto swap_idx = entity_map.erase_and_get_swap_index(e2);

    REQUIRE(entity_map.size() == 2);
    REQUIRE(entity_map.contains(e2) == false);
    REQUIRE(entity_map.contains(e1) == true);
    REQUIRE(entity_map.contains(e3) == true);

    // e3 should have been swapped to e2's position
    REQUIRE(entity_map.key(e3) == 1); // e2's old index
    REQUIRE(entity_map.entity_at(1) == e3);
    REQUIRE(swap_idx == 1);
  }

  SECTION("Remove last entity")
  {
    ouly::ecs::entity<> e1(10);
    ouly::ecs::entity<> e2(20);
    ouly::ecs::entity<> e3(30);

    entity_map.emplace(e1);
    entity_map.emplace(e2);
    entity_map.emplace(e3);

    auto swap_idx = entity_map.erase_and_get_swap_index(e3);

    REQUIRE(entity_map.size() == 2);
    REQUIRE(entity_map.contains(e3) == false);
    REQUIRE(swap_idx == 2); // No swap needed
  }

  SECTION("Automatic value swapping")
  {
    ouly::ecs::entity<> e1(10);
    ouly::ecs::entity<> e2(20);
    ouly::ecs::entity<> e3(30);

    entity_map.emplace(e1);
    entity_map.emplace(e2);
    entity_map.emplace(e3);

    std::vector<std::string> values = {"first", "second", "third"};

    entity_map.erase_and_swap_values(e2, values);

    REQUIRE(entity_map.size() == 2);
    REQUIRE(values.size() == 2);
    REQUIRE(values[0] == "first");
    REQUIRE(values[1] == "third"); // Swapped from position 2

    // Verify entity mapping is correct
    REQUIRE(entity_map.key(e1) == 0);
    REQUIRE(entity_map.key(e3) == 1);
  }
}

TEST_CASE("map: large scale operations", "[map][scale]")
{
  using map_type = ouly::ecs::map<ouly::ecs::entity<>>;

  map_type entity_map;

  SECTION("Large number of entities")
  {
    std::vector<ouly::ecs::entity<>> entities;

    for (uint32_t i = 1; i <= 1000; ++i)
    {
      ouly::ecs::entity<> e(i);
      entities.push_back(e);
      auto idx = entity_map.emplace(e);
      REQUIRE(idx == i - 1); // Should be consecutive
    }

    REQUIRE(entity_map.size() == 1000);

    // Random removal test
    uint32_t          seed = 42;
    std::vector<bool> removed(entities.size(), false);

    for (int i = 0; i < 500; ++i)
    {
      seed       = xorshift32(seed);
      size_t idx = seed % entities.size();

      if (!removed[idx])
      {
        entity_map.erase_and_get_swap_index(entities[idx]);
        removed[idx] = true;
      }
    }

    // Verify remaining entities are still accessible
    for (size_t i = 0; i < entities.size(); ++i)
    {
      bool should_exist = !removed[i];
      REQUIRE(entity_map.contains(entities[i]) == should_exist);
    }
  }
}

TEST_CASE("map: iteration and access patterns", "[map][iteration]")
{
  using map_type = ouly::ecs::map<ouly::ecs::entity<>>;

  map_type entity_map;

  SECTION("Dense iteration after sparse operations")
  {
    std::vector<ouly::ecs::entity<>> entities;

    // Create entities with sparse indices
    for (uint32_t i : {5, 10, 15, 20, 25})
    {
      ouly::ecs::entity<> e(i);
      entities.push_back(e);
      entity_map.emplace(e);
    }

    // Should be densely packed 0, 1, 2, 3, 4
    for (size_t i = 0; i < entities.size(); ++i)
    {
      REQUIRE(entity_map.key(entities[i]) == i);
      REQUIRE(entity_map.entity_at(i) == entities[i]);
    }

    // Remove some entities
    entity_map.erase_and_get_swap_index(entities[1]); // Remove entity(10)
    entity_map.erase_and_get_swap_index(entities[3]); // Remove entity(20)

    REQUIRE(entity_map.size() == 3);

    // Remaining entities should still be densely packed
    std::set<uint32_t> remaining_indices;
    for (const auto& e : entities)
    {
      if (entity_map.contains(e))
      {
        remaining_indices.insert(entity_map.key(e));
      }
    }

    REQUIRE(remaining_indices.size() == 3);
    REQUIRE(*remaining_indices.begin() == 0);
    REQUIRE(*remaining_indices.rbegin() == 2);
  }
}

// Comprehensive stress test combining all components
TEST_CASE("ecs: comprehensive integration test", "[ecs][integration]")
{
  using registry_type       = ouly::ecs::rxregistry<>;
  using position_components = ouly::ecs::components<non_trivial_type, ouly::ecs::rxentity<>>;
  using velocity_components = ouly::ecs::components<int, ouly::ecs::rxentity<>, sparse_config>;
  using entity_collection   = ouly::ecs::collection<ouly::ecs::rxentity<>>;
  using entity_map          = ouly::ecs::map<ouly::ecs::rxentity<>>;

  registry_type       registry;
  position_components positions;
  velocity_components velocities;
  entity_collection   active_entities;
  entity_map          entity_lookup;

  SECTION("Complex ECS scenario")
  {
    std::vector<typename registry_type::type> entities;

    // Create a variety of entities
    for (int i = 0; i < 100; ++i)
    {
      auto entity = registry.emplace();
      entities.push_back(entity);

      positions.set_max(registry.max_size());
      velocities.set_max(registry.max_size());

      // Add position to all entities
      positions.emplace_at(entity, "entity_" + std::to_string(i), i);

      // Add velocity to some entities
      if (i % 2 == 0)
      {
        velocities.emplace_at(entity, i * 10);
        active_entities.emplace(entity);
      }

      entity_lookup.emplace(entity);
    }

    REQUIRE(entities.size() == 100);
    REQUIRE(positions.size() == 100);
    REQUIRE(velocities.size() == 50);
    REQUIRE(active_entities.size() == 50);
    REQUIRE(entity_lookup.size() == 100);

    // Simulate some deletions
    for (int i = 10; i < 20; ++i)
    {
      auto& entity = entities[i];

      if (positions.contains(entity))
        positions.erase(entity);
      if (velocities.contains(entity))
        velocities.erase(entity);
      if (active_entities.contains(entity))
        active_entities.erase(entity);
      if (entity_lookup.contains(entity))
        entity_lookup.erase_and_get_swap_index(entity);

      registry.erase(entity);
    }

    // Verify consistency
    int valid_entity_count = 0;
    for (const auto& entity : entities)
    {
      if (registry.is_valid(entity))
      {
        valid_entity_count++;
        REQUIRE(positions.contains(entity) == true);
        REQUIRE(entity_lookup.contains(entity) == true);
      }
      else
      {
        REQUIRE(positions.contains(entity) == false);
        REQUIRE(velocities.contains(entity) == false);
        REQUIRE(active_entities.contains(entity) == false);
        REQUIRE(entity_lookup.contains(entity) == false);
      }
    }

    REQUIRE(valid_entity_count == 90);

    // Test iteration over active entities with both components
    int active_with_velocity = 0;
    active_entities.for_each(velocities,
                             [&](typename registry_type::type entity, int /* vel */)
                             {
                               REQUIRE(positions.contains(entity) == true);
                               active_with_velocity++;
                             });

    REQUIRE(active_with_velocity == 45); // 50 - 5 deleted from range 10-19

    // Create new entities to test reuse
    for (int i = 0; i < 10; ++i)
    {
      auto new_entity = registry.emplace();
      positions.set_max(registry.max_size());

      positions.emplace_at(new_entity, "new_entity_" + std::to_string(i), 1000 + i);
      entity_lookup.emplace(new_entity);

      REQUIRE(positions.contains(new_entity) == true);
      REQUIRE(entity_lookup.contains(new_entity) == true);
    }

    REQUIRE(registry.max_size() == 101); // Should reuse some slots
  }
}

// NOLINTEND
