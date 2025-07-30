OULY Documentation
==================

.. image:: https://img.shields.io/badge/C%2B%2B-20-blue.svg
   :target: https://isocpp.org/std/the-standard
   :alt: C++20

.. image:: https://img.shields.io/badge/License-MIT-yellow.svg
   :target: https://opensource.org/licenses/MIT
   :alt: License: MIT

.. image:: https://github.com/obhi-d/ouly/actions/workflows/ci.yml/badge.svg
   :target: https://github.com/obhi-d/ouly/actions/workflows/ci.yml
   :alt: CI

OULY (Optimized Utility Library) is a modern C++20 high-performance utility library providing memory allocators, containers, ECS (Entity Component System), task scheduler, and serialization components for performance-critical applications.

.. toctree::
   :maxdepth: 2
   :caption: Contents:

   getting_started
   tutorials/index
   user_guide/index
   api_reference/index
   examples/index
   performance/index
   contributing
   changelog

Quick Start
-----------

Installation
~~~~~~~~~~~~

OULY is a header-only library that can be easily integrated into your project:

.. code-block:: cmake

   # Using CMake FetchContent
   include(FetchContent)
   FetchContent_Declare(
     ouly
     GIT_REPOSITORY https://github.com/obhi-d/ouly.git
     GIT_TAG        main
   )
   FetchContent_MakeAvailable(ouly)

   # Link to your target
   target_link_libraries(your_target PRIVATE ouly::ouly)

Basic Usage
~~~~~~~~~~~

.. code-block:: cpp

   #include <ouly/ouly.hpp>

   int main() {
       // Memory allocators
       ouly::linear_arena_allocator<> allocator(1024 * 1024);
       void* data = allocator.allocate(1024);

       // Containers
       ouly::small_vector<int, 16> vec = {1, 2, 3, 4};
       vec.push_back(5);

       // ECS
       ouly::ecs::registry<> registry;
       auto entity = registry.emplace();
       
       // Task scheduler
       ouly::scheduler scheduler(4);
       auto workgroup = scheduler.create_workgroup();
       scheduler.begin_execution();
       
       scheduler.submit(workgroup, []() {
           return 42;
       }).wait();
       
       scheduler.end_execution();
       scheduler.shutdown();
       
       return 0;
   }

Key Features
------------

🧠 **Memory Management**
   Efficient memory allocators for different use cases including linear, arena, pool, and thread-safe variants.

📦 **Containers** 
   Modern containers with STL-like interfaces but improved performance including small_vector, dynamic_array, and more.

🎮 **Entity Component System**
   High-performance ECS framework for game development and simulation applications.

⚡ **Task Scheduler**
   Work-stealing task scheduler with coroutine support for parallel processing.

💾 **Serialization**
   Flexible serialization framework supporting binary and YAML formats.

🔧 **Utilities**
   Various utility components including reflection, DSL helpers, and more.

Requirements
------------

* C++20 compatible compiler (GCC 10+, Clang 12+, MSVC 2019+)
* CMake 3.20 or later
* Optional: Doxygen for documentation generation

Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
