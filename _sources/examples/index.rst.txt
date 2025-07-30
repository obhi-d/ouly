Examples
========

Working examples demonstrating OULY usage patterns.

.. toctree::
   :maxdepth: 2

   unit_test_examples

Overview
--------

OULY examples are currently found in the comprehensive unit test suite. These tests serve as practical examples showing how to use OULY components correctly in various scenarios.

**Unit Test Examples**
   The ``unit_tests/`` directory contains extensive examples for all OULY components, demonstrating proper usage patterns and best practices.

**Test Categories**
   * **Allocator Tests** - Memory management examples and usage patterns
   * **Container Tests** - High-performance container usage and configuration
   * **ECS Tests** - Entity-component system implementation examples
   * **Scheduler Tests** - Task scheduling and parallel processing examples
   * **Serialization Tests** - Binary and YAML serialization usage

Finding Examples
----------------

The unit tests are organized by component and provide comprehensive coverage:

.. code-block:: text

   unit_tests/
   ├── allocators/
   │   ├── test_linear_allocator.cpp
   │   ├── test_pool_allocator.cpp
   │   ├── test_arena_allocator.cpp
   │   └── ...
   ├── containers/
   │   ├── test_small_vector.cpp
   │   ├── test_dynamic_array.cpp
   │   ├── test_soavector.cpp
   │   └── ...
   ├── ecs/
   │   ├── test_registry.cpp
   │   ├── test_components.cpp
   │   └── ...
   ├── scheduler/
   │   ├── test_scheduler.cpp
   │   └── ...
   └── serializers/
       ├── test_binary_serializer.cpp
       ├── test_yaml_serializer.cpp
       └── ...

Building and Running Examples
-----------------------------

To build and run the unit test examples:

.. code-block:: bash

   # Clone the repository
   git clone https://github.com/obhi-d/ouly.git
   cd ouly

   # Configure with tests enabled
   cmake --preset=macos-default  # or linux-default
   
   # Build
   cmake --build build/macos-default

   # Run all tests
   cd build/macos-default && ctest

   # Run specific test categories
   ctest -R allocator  # Run allocator tests only
   ctest -R container  # Run container tests only
   ctest -R ecs        # Run ECS tests only

Learning from Unit Tests
------------------------

The unit tests demonstrate:

* **Correct API usage** - Proper initialization, configuration, and cleanup
* **Error handling** - How to handle edge cases and error conditions
* **Performance patterns** - Optimal usage for different scenarios
* **Integration examples** - How components work together

**Example Test Structure:**

Each test file follows a consistent pattern:

.. code-block:: cpp

   #include <catch2/catch_test_macros.hpp>
   #include <ouly/component/header.hpp>

   TEST_CASE("component basic usage", "[component]") {
       // Setup
       // Usage demonstration
       // Verification
   }

   TEST_CASE("component advanced features", "[component]") {
       // Advanced usage patterns
   }

**Key Test Files to Review:**

* ``test_linear_allocator.cpp`` - Memory allocation patterns
* ``test_small_vector.cpp`` - Container usage and performance
* ``test_registry.cpp`` - ECS entity management
* ``test_scheduler.cpp`` - Task submission and execution
* ``test_binary_serializer.cpp`` - Serialization patterns

Contributing Examples
---------------------

To contribute additional examples:

1. **Add to Unit Tests** - Extend existing test files with new usage patterns
2. **Document Use Cases** - Add comments explaining the example's purpose
3. **Follow Test Structure** - Use the established testing patterns
4. **Include Edge Cases** - Demonstrate error handling and boundary conditions

Example Template:

.. code-block:: cpp

   TEST_CASE("descriptive test name", "[category]") {
       SECTION("basic usage") {
           // Demonstrate basic functionality
       }
       
       SECTION("advanced usage") {
           // Show advanced patterns
       }
       
       SECTION("error handling") {
           // Demonstrate proper error handling
       }
   }

Future Examples
---------------

Standalone examples are planned for future releases:

* **Game Engine Example** - Complete ECS-based game
* **Scientific Computing** - Parallel numerical algorithms
* **Web Server** - High-performance HTTP server
* **Data Processing** - ETL pipeline with custom allocators

For now, the comprehensive unit test suite provides extensive examples of proper OULY usage patterns.
