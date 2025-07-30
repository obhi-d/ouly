Entity Component System Tutorial
===============================

OULY's ECS (Entity Component System) is designed for high-performance, data-oriented applications. This tutorial will guide you through building efficient ECS-based systems for games, simulations, and other performance-critical applications.

ECS Fundamentals
----------------

The Entity Component System pattern separates data (Components) from behavior (Systems) using unique identifiers (Entities):

* **Entities** - Unique IDs that represent game objects or simulation elements
* **Components** - Data structures that hold state (Position, Velocity, Health, etc.)
* **Systems** - Functions that operate on entities with specific component combinations

Basic Entity Management
-----------------------

Start with creating and managing entities:

.. code-block:: cpp

   #include <ouly/ecs/registry.hpp>
   #include <iostream>

   int main() {
       // Create an entity registry
       ouly::ecs::registry<> registry;
       
       // Create entities
       auto player = registry.emplace();
       auto enemy1 = registry.emplace();
       auto enemy2 = registry.emplace();
       
       std::cout << "Player entity: " << player.value() << "\n";
       std::cout << "Enemy1 entity: " << enemy1.value() << "\n";
       
       // Check if entities are valid
       std::cout << "Player valid: " << registry.is_valid(player) << "\n";
       
       // Destroy an entity
       registry.destroy(enemy1);
       std::cout << "Enemy1 valid after destroy: " << registry.is_valid(enemy1) << "\n";
       
       // Entity IDs are recycled
       auto new_entity = registry.emplace();
       std::cout << "New entity (recycled ID): " << new_entity.value() << "\n";
       
       return 0;
   }

Component Storage
-----------------

Components are stored separately from entities for optimal memory layout:

.. code-block:: cpp

   #include <ouly/ecs/registry.hpp>
   #include <ouly/ecs/components.hpp>
   #include <iostream>

   // Define component types
   struct Position {
       float x, y, z;
       Position(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
   };

   struct Velocity {
       float dx, dy, dz;
       Velocity(float dx = 0, float dy = 0, float dz = 0) : dx(dx), dy(dy), dz(dz) {}
   };

   struct Health {
       int current, maximum;
       Health(int max = 100) : current(max), maximum(max) {}
   };

   int main() {
       ouly::ecs::registry<> registry;
       
       // Create component storage
       ouly::ecs::components<Position> positions;
       ouly::ecs::components<Velocity> velocities;
       ouly::ecs::components<Health> health_components;
       
       // Create entities
       auto player = registry.emplace();
       auto npc = registry.emplace();
       
       // Add components to entities
       positions.emplace_at(player, 10.0f, 20.0f, 0.0f);
       velocities.emplace_at(player, 1.0f, 0.0f, 0.0f);
       health_components.emplace_at(player, 150);  // 150 max health
       
       positions.emplace_at(npc, 5.0f, 15.0f, 0.0f);
       health_components.emplace_at(npc, 100);     // 100 max health (no velocity)
       
       // Access components
       auto& player_pos = positions[player];
       auto& player_vel = velocities[player];
       
       std::cout << "Player position: (" << player_pos.x << ", " << player_pos.y << ")\n";
       std::cout << "Player velocity: (" << player_vel.dx << ", " << player_vel.dy << ")\n";
       
       // Check if entity has component
       if (positions.contains(npc)) {
           std::cout << "NPC has position component\n";
       }
       
       if (!velocities.contains(npc)) {
           std::cout << "NPC does not have velocity component\n";
       }
       
       return 0;
   }

Component Storage Configuration
-------------------------------

OULY provides different storage strategies for optimal performance:

.. code-block:: cpp

   #include <ouly/ecs/components.hpp>
   #include <ouly/ecs/config.hpp>

   // Sparse storage - memory efficient for sparse data
   using SparseConfig = ouly::cfg::use_sparse<>;
   ouly::ecs::components<Position, ouly::ecs::entity<>, SparseConfig> sparse_positions;

   // Dense storage - faster iteration (default)
   ouly::ecs::components<Velocity> dense_velocities;

   // Direct mapping - fastest access, more memory usage
   using DirectConfig = ouly::cfg::use_direct_mapping<>;
   ouly::ecs::components<Health, ouly::ecs::entity<>, DirectConfig> direct_health;

**Storage Strategy Guidelines:**

* **Sparse** - Use for components that exist on few entities (< 10% of total)
* **Dense** - Default choice, good balance of memory and performance  
* **Direct** - Use for components that exist on most entities (> 80% of total)

ECS Configuration Options
--------------------------

ECS components can be configured using options from the ``ouly::cfg`` namespace:

.. code-block:: cpp

   #include <ouly/ecs/components.hpp>
   #include <ouly/utility/config.hpp>

   // Custom storage configuration
   using CustomConfig = ouly::config<
       ouly::cfg::use_sparse,              // Use sparse storage
       ouly::cfg::pool_size<8192>,         // Set pool size for allocations
       ouly::cfg::custom_vector<MyVector>  // Use custom vector implementation
   >;
   
   ouly::ecs::components<Position, ouly::ecs::entity<>, CustomConfig> components;

**Available ECS Configuration Options:**

* ``ouly::cfg::use_sparse`` - Enable sparse storage strategy for memory efficiency
* ``ouly::cfg::use_direct_mapping`` - Enable direct mapping for fastest access
* ``ouly::cfg::pool_size<N>`` - Set pool size for internal memory management
* ``ouly::cfg::index_pool_size<N>`` - Pool size for entity index management  
* ``ouly::cfg::self_index_pool_size<N>`` - Pool size for self-referencing indexes
* ``ouly::cfg::keys_index_pool_size<N>`` - Pool size for key index management
* ``ouly::cfg::custom_vector<T>`` - Use custom vector implementation as storage backend
* ``ouly::cfg::basic_size_type<T>`` - Set size type for component storage (default: uint32_t)
* ``ouly::cfg::use_sparse_index`` - Use sparse indexing for entity lookups
* ``ouly::cfg::self_use_sparse_index`` - Enable sparse indexing for self-references
* ``ouly::cfg::keys_use_sparse_index`` - Enable sparse indexing for key management
* ``ouly::cfg::zero_out_memory`` - Zero-initialize allocated memory
* ``ouly::cfg::disable_pool_tracking`` - Disable pool usage tracking for performance

**Configuration Examples:**

.. code-block:: cpp

   // High-performance configuration for frequently accessed components
   using PerformanceConfig = ouly::config<
       ouly::cfg::use_direct_mapping,
       ouly::cfg::pool_size<16384>,
       ouly::cfg::disable_pool_tracking
   >;

   // Memory-efficient configuration for rarely used components  
   using MemoryConfig = ouly::config<
       ouly::cfg::use_sparse,
       ouly::cfg::pool_size<1024>,
       ouly::cfg::use_sparse_index
   >;

   // Debug configuration with safety features
   using DebugConfig = ouly::config<
       ouly::cfg::zero_out_memory,
       ouly::cfg::pool_size<4096>
   >;

   // Components configured for different access patterns
   ouly::ecs::components<Transform, ouly::ecs::entity<>, PerformanceConfig> transforms;
   ouly::ecs::components<AudioSource, ouly::ecs::entity<>, MemoryConfig> audio_sources;
   ouly::ecs::components<DebugInfo, ouly::ecs::entity<>, DebugConfig> debug_info;

Systems and Iteration
----------------------

Systems process entities with specific component combinations:

.. code-block:: cpp

   #include <ouly/ecs/registry.hpp>
   #include <ouly/ecs/components.hpp>

   // Movement system - updates positions based on velocity
   void movement_system(ouly::ecs::components<Position>& positions,
                       ouly::ecs::components<Velocity>& velocities,
                       float delta_time) {
       // Iterate over all entities that have both Position and Velocity
       positions.for_each(velocities, [delta_time](auto entity, auto& pos, auto& vel) {
           pos.x += vel.dx * delta_time;
           pos.y += vel.dy * delta_time;
           pos.z += vel.dz * delta_time;
       });
   }

   // Health regeneration system
   void health_regen_system(ouly::ecs::components<Health>& health_components,
                           float delta_time) {
       health_components.for_each([delta_time](auto entity, auto& health) {
           if (health.current < health.maximum) {
               health.current = std::min(health.maximum, 
                                       health.current + static_cast<int>(10 * delta_time));
           }
       });
   }

   int main() {
       ouly::ecs::registry<> registry;
       ouly::ecs::components<Position> positions;
       ouly::ecs::components<Velocity> velocities;
       ouly::ecs::components<Health> health_components;
       
       // Create some entities with components
       for (int i = 0; i < 100; ++i) {
           auto entity = registry.emplace();
           positions.emplace_at(entity, i * 10.0f, 0.0f, 0.0f);
           velocities.emplace_at(entity, 1.0f, 0.0f, 0.0f);
           health_components.emplace_at(entity, 80);  // Damaged entities
       }
       
       // Game loop
       float delta_time = 1.0f / 60.0f;  // 60 FPS
       for (int frame = 0; frame < 10; ++frame) {
           movement_system(positions, velocities, delta_time);
           health_regen_system(health_components, delta_time);
       }
       
       return 0;
   }

Collections for Entity Groups
------------------------------

Collections allow efficient processing of specific entity groups:

.. code-block:: cpp

   #include <ouly/ecs/collection.hpp>

   int main() {
       ouly::ecs::registry<> registry;
       ouly::ecs::components<Position> positions;
       ouly::ecs::components<Velocity> velocities;
       ouly::ecs::components<Health> health_components;
       
       // Collections for different entity types
       ouly::ecs::collection<ouly::ecs::entity<>> players;
       ouly::ecs::collection<ouly::ecs::entity<>> enemies;
       ouly::ecs::collection<ouly::ecs::entity<>> projectiles;
       
       // Create different entity types
       auto player = registry.emplace();
       positions.emplace_at(player, 0.0f, 0.0f, 0.0f);
       health_components.emplace_at(player, 150);
       players.emplace(player);
       
       for (int i = 0; i < 10; ++i) {
           auto enemy = registry.emplace();
           positions.emplace_at(enemy, i * 5.0f, 10.0f, 0.0f);
           velocities.emplace_at(enemy, 0.0f, -1.0f, 0.0f);
           health_components.emplace_at(enemy, 50);
           enemies.emplace(enemy);
       }
       
       // Process only enemies
       enemies.for_each(positions, velocities, [](auto entity, auto& pos, auto& vel) {
           // AI behavior for enemies
           if (pos.y < 0) {
               vel.dy = 1.0f;  // Bounce off bottom
           }
       });
       
       // Process only players  
       players.for_each(health_components, [](auto entity, auto& health) {
           std::cout << "Player health: " << health.current << "/" << health.maximum << "\n";
       });
       
       return 0;
   }

Thread-Safe ECS with Revisions
-------------------------------

For multi-threaded applications, use the revision-based registry:

.. code-block:: cpp

   #include <ouly/ecs/rxregistry.hpp>
   #include <thread>
   #include <vector>

   int main() {
       // Revision-based registry for thread safety
       ouly::ecs::rxregistry<> registry;
       ouly::ecs::components<Position> positions;
       
       std::vector<std::thread> threads;
       std::vector<ouly::ecs::rxentity<>> entities;
       
       // Create entities in main thread
       for (int i = 0; i < 100; ++i) {
           auto entity = registry.emplace();
           positions.emplace_at(entity, i * 1.0f, 0.0f, 0.0f);
           entities.push_back(entity);
       }
       
       // Launch worker threads
       for (int t = 0; t < 4; ++t) {
           threads.emplace_back([&registry, &positions, &entities, t]() {
               for (size_t i = t; i < entities.size(); i += 4) {
                   auto entity = entities[i];
                   
                   // Check if entity is still valid (might have been destroyed)
                   if (registry.is_valid(entity) && positions.contains(entity)) {
                       auto& pos = positions[entity];
                       pos.x += 0.1f;  // Move entity
                   }
               }
           });
       }
       
       // Wait for all threads
       for (auto& thread : threads) {
           thread.join();
       }
       
       return 0;
   }

Advanced ECS Patterns
---------------------

**Component Dependencies and Relationships**

.. code-block:: cpp

   struct Transform {
       float x, y, z;
       float rotation;
       float scale;
   };

   struct Parent {
       ouly::ecs::entity<> parent_entity;
   };

   struct Children {
       std::vector<ouly::ecs::entity<>> child_entities;
   };

   // System to update child transforms based on parent
   void transform_hierarchy_system(
       ouly::ecs::components<Transform>& transforms,
       ouly::ecs::components<Parent>& parents,
       ouly::ecs::components<Children>& children) {
       
       parents.for_each(transforms, [&](auto child_entity, auto& parent_ref, auto& child_transform) {
           if (transforms.contains(parent_ref.parent_entity)) {
               auto& parent_transform = transforms[parent_ref.parent_entity];
               // Apply parent transformation to child
               child_transform.x += parent_transform.x;
               child_transform.y += parent_transform.y;
               child_transform.rotation += parent_transform.rotation;
           }
       });
   }

**Tag Components**

.. code-block:: cpp

   // Tag components (zero-size)
   struct Player {};
   struct Enemy {};
   struct Dead {};

   int main() {
       ouly::ecs::registry<> registry;
       ouly::ecs::components<Player> player_tags;
       ouly::ecs::components<Enemy> enemy_tags;
       ouly::ecs::components<Dead> dead_tags;
       
       auto entity = registry.emplace();
       player_tags.emplace_at(entity);  // Mark as player
       
       // Check for tag
       if (player_tags.contains(entity)) {
           std::cout << "Entity is a player\n";
       }
       
       return 0;
   }

**Event System Integration**

.. code-block:: cpp

   #include <functional>
   #include <vector>

   struct DamageEvent {
       ouly::ecs::entity<> target;
       int damage;
       ouly::ecs::entity<> source;
   };

   class EventSystem {
       std::vector<DamageEvent> damage_events;
       
   public:
       void queue_damage(ouly::ecs::entity<> target, int damage, ouly::ecs::entity<> source) {
           damage_events.push_back({target, damage, source});
       }
       
       void process_damage_events(ouly::ecs::components<Health>& health_components,
                                 ouly::ecs::components<Dead>& dead_tags) {
           for (const auto& event : damage_events) {
               if (health_components.contains(event.target)) {
                   auto& health = health_components[event.target];
                   health.current -= event.damage;
                   
                   if (health.current <= 0) {
                       dead_tags.emplace_at(event.target);
                   }
               }
           }
           damage_events.clear();
       }
   };

Performance Optimization
------------------------

**Memory Layout Optimization**

.. code-block:: cpp

   // Pack components for better cache utilization
   struct PackedTransform {
       float position[3];    // x, y, z
       float rotation[4];    // quaternion
       float scale[3];       // sx, sy, sz
   };

   // Use Structure of Arrays for SIMD operations
   #include <ouly/containers/soavector.hpp>
   
   struct ParticleData {
       float pos_x, pos_y, pos_z;      // position
       float vel_x, vel_y, vel_z;      // velocity
       float life_time;                // lifetime
   };
   
   ouly::soavector<ParticleData> particles;
   
   // Access individual component arrays
   auto& x_positions = particles.get<0>();  // pos_x array
   auto& lifetimes = particles.get<6>();    // life_time array

**Batch Processing**

.. code-block:: cpp

   // Process components in batches for better cache utilization
   void batch_movement_system(ouly::ecs::components<Position>& positions,
                             ouly::ecs::components<Velocity>& velocities,
                             float delta_time) {
       constexpr size_t BATCH_SIZE = 64;
       
       positions.for_each_batch(velocities, BATCH_SIZE,
           [delta_time](auto begin, auto end, auto& pos_range, auto& vel_range) {
               // Process batch of entities
               for (auto it = begin; it != end; ++it) {
                   auto& pos = pos_range[it];
                   auto& vel = vel_range[it];
                   pos.x += vel.dx * delta_time;
                   pos.y += vel.dy * delta_time;
                   pos.z += vel.dz * delta_time;
               }
           });
   }

Best Practices
--------------

1. **Component Design**
   
   * Keep components simple and focused (Single Responsibility)
   * Use POD types when possible for better performance
   * Group related data into single components

2. **System Organization**
   
   * Systems should be stateless functions
   * Process components in logical order (input → logic → rendering)
   * Use collections to group entities by type/behavior

3. **Memory Management**
   
   * Choose appropriate storage strategy for each component type
   * Use custom allocators for better memory control
   * Consider memory alignment for SIMD operations

4. **Threading**
   
   * Use rxregistry for multi-threaded access
   * Partition work by entity ranges or component types
   * Avoid shared mutable state between threads

Next Steps
----------

* Learn about :doc:`scheduler_tutorial` for parallel ECS processing
* Explore :doc:`memory_management` for optimal ECS memory layout
* Check :doc:`../performance/index` for ECS optimization techniques
