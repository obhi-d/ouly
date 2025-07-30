Getting Started
===============

This guide will help you get started with OULY, covering installation, basic concepts, and your first programs.

System Requirements
-------------------

**Compiler Support**
   * GCC 13.0 or later
   * Clang 17.0 or later  
   * MSVC 2022 (Visual Studio 17.0) or later
   * Apple Clang 15.0 or later

**Build System**
   * CMake 3.20 or later
   * Ninja (recommended) or Make

**Operating Systems**
   * Linux (Ubuntu 20.04+, RHEL 8+)
   * macOS 11.0+
   * Windows 10/11

Installation
------------

OULY is a header-only library that can be integrated into your project in several ways:

Using CMake FetchContent (Recommended)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cmake

   cmake_minimum_required(VERSION 3.20)
   project(MyProject)

   include(FetchContent)
   FetchContent_Declare(
     ouly
     GIT_REPOSITORY https://github.com/obhi-d/ouly.git
     GIT_TAG        main  # or specific version tag
   )
   FetchContent_MakeAvailable(ouly)

   add_executable(my_app main.cpp)
   target_link_libraries(my_app PRIVATE ouly::ouly)
   target_compile_features(my_app PRIVATE cxx_std_20)

Git Submodule
~~~~~~~~~~~~~

.. code-block:: bash

   # Add as submodule
   git submodule add https://github.com/obhi-d/ouly.git third_party/ouly
   git submodule update --init --recursive

Then in your CMakeLists.txt:

.. code-block:: cmake

   add_subdirectory(third_party/ouly)
   target_link_libraries(your_target PRIVATE ouly::ouly)

Manual Installation
~~~~~~~~~~~~~~~~~~~

1. Download or clone the repository
2. Copy the ``include/`` directory to your project
3. Add the include path to your compiler

.. code-block:: bash

   git clone https://github.com/obhi-d/ouly.git
   cp -r ouly/include/ouly /your/project/include/

Building from Source
--------------------

If you want to build tests, examples, or documentation:

.. code-block:: bash

   git clone https://github.com/obhi-d/ouly.git
   cd ouly
   
   # Configure with default preset
   cmake --preset=macos-default  # or linux-default
   
   # Build
   cmake --build build/macos-default
   
   # Run tests
   cd build/macos-default && ctest

Available CMake Presets
~~~~~~~~~~~~~~~~~~~~~~~

* ``macos-default`` - Debug build with tests (macOS)
* ``macos-release`` - Release build (macOS)
* ``linux-default`` - Debug build with tests (Linux)
* ``linux-release`` - Release build (Linux)

CMake Options
~~~~~~~~~~~~~

.. code-block:: cmake

   option(OULY_BUILD_TESTS "Build unit tests" OFF)
   option(OULY_BUILD_DOCS "Build documentation" OFF)
   option(OULY_ENABLE_ASAN "Enable AddressSanitizer" OFF)

Basic Concepts
--------------

OULY is organized into several core modules:

**Allocators** (``ouly/allocators/``)
   Memory management components for efficient allocation strategies.

**Containers** (``ouly/containers/``)  
   High-performance containers with STL-compatible interfaces.

**ECS** (``ouly/ecs/``)
   Entity Component System for data-oriented design.

**Scheduler** (``ouly/scheduler/``)
   Task scheduling and parallel execution framework.

**Serializers** (``ouly/serializers/``)
   Binary and text-based serialization utilities.

**Utilities** (``ouly/utility/``)
   Helper classes and meta-programming utilities.

First Program
-------------

Here's a simple program demonstrating basic OULY usage:

.. code-block:: cpp

   #include <ouly/allocators/linear_arena_allocator.hpp>
   #include <ouly/containers/small_vector.hpp>
   #include <ouly/ecs/registry.hpp>
   #include <ouly/scheduler/scheduler_v2.hpp>
   #include <iostream>

   int main() {
       // Memory allocator example
       ouly::linear_arena_allocator<> allocator(1024);
       void* memory = allocator.allocate(256);
       std::cout << "Allocated 256 bytes\n";

       // Container example  
       ouly::small_vector<int, 8> numbers = {1, 2, 3, 4, 5};
       numbers.push_back(6);
       
       std::cout << "Vector contents: ";
       for (auto num : numbers) {
           std::cout << num << " ";
       }
       std::cout << "\n";

       // ECS example
       ouly::ecs::registry<> registry;
       auto entity = registry.emplace();
       std::cout << "Created entity: " << entity.value() << "\n";

       // Scheduler example
       ouly::scheduler scheduler(2);
       auto workgroup = scheduler.create_workgroup();
       scheduler.begin_execution();
       
       auto future = scheduler.submit(workgroup, []() {
           return 42;
       });
       
       std::cout << "Task result: " << future.get() << "\n";
       
       scheduler.end_execution();
       scheduler.shutdown();

       return 0;
   }

Next Steps
----------

* Read the :doc:`tutorials/index` for guided examples
* Explore the :doc:`user_guide/index` for detailed component documentation  
* Check out :doc:`examples/index` for real-world usage patterns
* Review :doc:`performance/index` for optimization guidance
