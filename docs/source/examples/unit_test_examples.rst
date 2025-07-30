Unit Test Examples
==================

The OULY unit tests serve as comprehensive examples of proper API usage and best practices.

Allocator Examples
------------------

**Linear Allocator Usage**

The linear allocator tests demonstrate sequential allocation patterns:

.. code-block:: cpp

   // From unit_tests/allocators/test_linear_allocator.cpp
   TEST_CASE("linear_allocator basic usage", "[allocators]") {
       ouly::linear_allocator<> allocator(1024);
       
       // Sequential allocations
       void* ptr1 = allocator.allocate(256);
       void* ptr2 = allocator.allocate(128);
       
       REQUIRE(ptr1 != nullptr);
       REQUIRE(ptr2 != nullptr);
       REQUIRE(allocator.used() == 384);
       
       // LIFO deallocation
       allocator.deallocate(ptr2, 128);
       allocator.deallocate(ptr1, 256);
       REQUIRE(allocator.used() == 0);
   }

**Pool Allocator Examples**

Pool allocators for fixed-size object allocation:

.. code-block:: cpp

   TEST_CASE("pool_allocator object management", "[allocators]") {
       struct TestObject {
           int value;
           float data;
       };
       
       ouly::pool_allocator<> pool(sizeof(TestObject), 100);
       
       std::vector<TestObject*> objects;
       
       // Allocate multiple objects
       for (int i = 0; i < 50; ++i) {
           auto* obj = static_cast<TestObject*>(
               pool.allocate(sizeof(TestObject))
           );
           new(obj) TestObject{i, static_cast<float>(i * 2)};
           objects.push_back(obj);
       }
       
       // Cleanup
       for (auto* obj : objects) {
           obj->~TestObject();
           pool.deallocate(obj, sizeof(TestObject));
       }
   }

Container Examples
------------------

**Small Vector Usage**

Stack-optimized vector examples:

.. code-block:: cpp

   // From unit_tests/containers/test_small_vector.cpp
   TEST_CASE("small_vector stack optimization", "[containers]") {
       ouly::small_vector<int, 8> vec;
       
       // These use stack storage
       for (int i = 0; i < 8; ++i) {
           vec.push_back(i);
       }
       REQUIRE(!vec.is_heap_allocated());
       
       // This triggers heap allocation
       vec.push_back(8);
       REQUIRE(vec.is_heap_allocated());
   }

**SoA Vector Examples**

Structure of Arrays container usage:

.. code-block:: cpp

   TEST_CASE("soavector aggregate expansion", "[containers]") {
       struct Particle {
           float x, y, z;
           int life;
       };
       
       ouly::soavector<Particle> particles;
       
       // Add particles
       particles.emplace_back(1.0f, 2.0f, 3.0f, 100);
       particles.emplace_back(4.0f, 5.0f, 6.0f, 50);
       
       // Access individual arrays for SIMD operations
       auto& x_coords = particles.get<0>();
       auto& y_coords = particles.get<1>();
       auto& lifetimes = particles.get<3>();
       
       // Vectorized update
       for (size_t i = 0; i < particles.size(); ++i) {
           x_coords[i] += 1.0f;
           lifetimes[i] -= 1;
       }
   }

ECS Examples
------------

**Entity and Component Management**

.. code-block:: cpp

   // From unit_tests/ecs/test_registry.cpp
   TEST_CASE("ecs basic entity management", "[ecs]") {
       ouly::ecs::registry<> registry;
       ouly::ecs::components<Position> positions;
       ouly::ecs::components<Velocity> velocities;
       
       // Create entities
       auto entity1 = registry.emplace();
       auto entity2 = registry.emplace();
       
       // Add components
       positions.emplace_at(entity1, 10.0f, 20.0f, 0.0f);
       velocities.emplace_at(entity1, 1.0f, 0.0f, 0.0f);
       
       positions.emplace_at(entity2, 5.0f, 15.0f, 0.0f);
       // entity2 has no velocity component
       
       // System processing
       positions.for_each(velocities, [](auto entity, auto& pos, auto& vel) {
           pos.x += vel.dx;
           pos.y += vel.dy;
           pos.z += vel.dz;
       });
   }

**Component Storage Configuration**

.. code-block:: cpp

   TEST_CASE("ecs storage strategies", "[ecs]") {
       // Dense storage (default)
       ouly::ecs::components<Position> dense_positions;
       
       // Sparse storage for rarely used components
       using SparseConfig = ouly::cfg::use_sparse<>;
       ouly::ecs::components<SpecialAbility, ouly::ecs::entity<>, SparseConfig> abilities;
       
       // Direct mapping for frequently accessed components
       using DirectConfig = ouly::cfg::use_direct_mapping<>;
       ouly::ecs::components<Transform, ouly::ecs::entity<>, DirectConfig> transforms;
   }

Scheduler Examples
------------------

**Basic Task Execution**

.. code-block:: cpp

   // From unit_tests/scheduler/test_scheduler.cpp
   TEST_CASE("scheduler basic task execution", "[scheduler]") {
       ouly::scheduler scheduler(2);
       
       // Create workgroup BEFORE begin_execution
       auto workgroup = scheduler.create_workgroup();
       
       // Begin execution
       scheduler.begin_execution();
       
       // Submit tasks
       auto future1 = scheduler.submit(workgroup, []() { return 42; });
       auto future2 = scheduler.submit(workgroup, []() { return 24; });
       
       // Wait for results
       REQUIRE(future1.get() == 42);
       REQUIRE(future2.get() == 24);
       
       // Cleanup
       scheduler.end_execution();
       scheduler.shutdown();
   }

**Parallel Algorithms**

.. code-block:: cpp

   TEST_CASE("scheduler parallel processing", "[scheduler]") {
       ouly::scheduler scheduler(4);
       auto workgroup = scheduler.create_workgroup();
       
       scheduler.begin_execution();
       
       std::vector<int> data(1000);
       std::iota(data.begin(), data.end(), 1);
       
       // Parallel processing using task context
       scheduler.submit(workgroup, [&](auto& ctx) {
           // Split work across available workers
           size_t chunk_size = data.size() / 4;
           std::vector<std::future<void>> futures;
           
           for (size_t i = 0; i < 4; ++i) {
               size_t start = i * chunk_size;
               size_t end = (i == 3) ? data.size() : start + chunk_size;
               
               auto future = ouly::async(ctx, ctx.current_workgroup(), 
                   [&data, start, end]() {
                       for (size_t j = start; j < end; ++j) {
                           data[j] *= 2;  // Double each element
                       }
                   });
               futures.push_back(std::move(future));
           }
           
           // Wait for all chunks to complete
           for (auto& f : futures) {
               f.wait();
           }
       }).wait();
       
       scheduler.end_execution();
       scheduler.shutdown();
   }

Serialization Examples
----------------------

**Binary Serialization**

.. code-block:: cpp

   // From unit_tests/serializers/test_binary_serializer.cpp
   TEST_CASE("binary serialization basic types", "[serializers]") {
       struct TestData {
           int id;
           std::string name;
           std::vector<float> values;
       };
       
       TestData original{42, "test", {1.0f, 2.0f, 3.0f}};
       
       // Serialize
       ouly::binary_stream stream;
       ouly::write<std::endian::little>(stream, original);
       
       // Deserialize
       ouly::binary_stream input(stream.data(), stream.size());
       TestData loaded;
       ouly::read<std::endian::little>(input, loaded);
       
       // Verify
       REQUIRE(loaded.id == original.id);
       REQUIRE(loaded.name == original.name);
       REQUIRE(loaded.values == original.values);
   }

**YAML Serialization**

.. code-block:: cpp

   TEST_CASE("yaml serialization configuration", "[serializers]") {
       struct Config {
           std::string app_name;
           int version;
           std::vector<std::string> modules;
           std::map<std::string, int> settings;
       };
       
       Config original;
       original.app_name = "TestApp";
       original.version = 1;
       original.modules = {"core", "graphics", "audio"};
       original.settings["max_fps"] = 60;
       original.settings["resolution_x"] = 1920;
       
       // Serialize to YAML
       std::string yaml = ouly::yml::to_string(original);
       
       // Deserialize from YAML
       Config loaded;
       ouly::yml::from_string(loaded, yaml);
       
       // Verify
       REQUIRE(loaded.app_name == original.app_name);
       REQUIRE(loaded.modules == original.modules);
       REQUIRE(loaded.settings == original.settings);
   }

Running Specific Examples
-------------------------

To run and study specific examples:

.. code-block:: bash

   # Build with tests
   cmake --preset=macos-default
   cmake --build build/macos-default

   # Run specific test categories
   cd build/macos-default
   
   # All allocator examples
   ctest -R allocator -V
   
   # All container examples  
   ctest -R container -V
   
   # All ECS examples
   ctest -R ecs -V
   
   # All scheduler examples
   ctest -R scheduler -V
   
   # All serialization examples
   ctest -R serializer -V

The ``-V`` flag provides verbose output showing the test execution details.
