Getting Started
===============

This page covers building OULY, using it from a CMake project, the build options and
configuration macros, and a first program.

Requirements
------------

* A C++20 compiler with concepts and coroutines. CI builds and tests GCC 16, Clang 18, Apple Clang
  (``macos-latest``) and MSVC (``windows-latest``).
* CMake 3.25 or newer. The presets use the Ninja generator.
* Linux, macOS or Windows.

OULY is a compiled library: the schedulers, the coalescing and thread-safe allocators and the DSL
parsers have source files in ``src/``. It builds as a static library by default and as a shared
library with ``-DBUILD_SHARED_LIBS=ON``.

Building from source
--------------------

.. code-block:: bash

   git clone https://github.com/obhi-d/ouly.git
   cd ouly

   cmake --preset=linux-default            # Debug build with unit tests
   cmake --build build/linux-default
   ctest --test-dir build/linux-default --output-on-failure

The presets in ``CMakePresets.json`` write to ``build/<preset>``:

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Preset
     - Configuration
   * - ``linux-default``
     - Debug, unit tests
   * - ``linux-release``
     - ``RelWithDebInfo``, unit tests
   * - ``linux-clang-tidy``
     - Debug with ``clang++``, unit tests, clang-tidy run on every library source
   * - ``macos-default`` / ``macos-release``
     - Debug / Release with ``clang++``, unit tests
   * - ``windows-test``, ``windows-gen``
     - Debug, unit tests
   * - ``windows-profiling`` / ``windows-release``
     - ``RelWithDebInfo``, unit tests
   * - ``windows-default``
     - Debug, library only
   * - ``third-party-release``
     - ``RelWithDebInfo``, library only, installs to ``../../Source/Ouly``

Without a preset, enable the tests explicitly:

.. code-block:: bash

   cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DOULY_BUILD_TESTS=ON
   cmake --build build
   ctest --test-dir build --output-on-failure

Using OULY from CMake
---------------------

Install the library and find it from your project:

.. code-block:: bash

   cmake -S ouly -B ouly-build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$HOME/ouly-install
   cmake --build ouly-build
   cmake --install ouly-build

.. code-block:: cmake

   cmake_minimum_required(VERSION 3.25)
   project(my_app LANGUAGES CXX)
   set(CMAKE_CXX_STANDARD 20)

   # configure with -DCMAKE_PREFIX_PATH=$HOME/ouly-install
   find_package(ouly REQUIRED)

   add_executable(my_app main.cpp)
   target_link_libraries(my_app PRIVATE ouly::ouly)

.. note::

   ``add_subdirectory`` and ``FetchContent`` do not work yet: the ``ouly`` target's include
   directory is derived from ``CMAKE_SOURCE_DIR``, which is the parent project's source directory
   when OULY is built as a subproject.

CMake options
~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - Option
     - Effect
   * - ``OULY_BUILD_TESTS`` (``OFF``)
     - Build the Catch2 unit tests in ``unit_tests/`` (with CTest's ``BUILD_TESTING``, on by
       default). Catch2, nlohmann_json, fast_float and GLM are fetched for the tests. On GCC and
       Clang the tests are always built with AddressSanitizer and coverage instrumentation.
   * - ``OULY_BUILD_BENCHMARKS`` (``OFF``)
     - Build the ``bench_*`` executables; fetches nanobench and oneTBB. See :doc:`performance/index`.
   * - ``OULY_ENABLE_CLANG_TIDY`` (``OFF``)
     - Run clang-tidy (``.clang-tidy``) on the library sources while compiling.
   * - ``OULY_SCHEDULER_PMR_INTERNAL_ALLOCATOR`` (``OFF``)
     - Scheduler-internal nodes come from a synchronized PMR pool instead of ``operator new``; see
       :doc:`tutorials/scheduler_tutorial`. Propagated to consumers as a compile definition.
   * - ``OULY_EXPORT_CC_TO_SRC_DIR``
     - Symlink ``compile_commands.json`` into the source directory.

The library itself is compiled with ``-Wall -Wextra -pedantic -Werror`` and a long list of extra
warnings on GCC and Clang, and ``/W4 /WX`` on MSVC. ``ASAN_ENABLED`` (set by several presets),
``OULY_REC_STATS`` and ``OULY_TEST_COVERAGE`` are declared but currently have no effect: nothing
reads the ``OULY_REC_STATS`` definition, and sanitizer and coverage flags for the tests are chosen
per compiler as described above.

Configuration macros
~~~~~~~~~~~~~~~~~~~~

Define these consistently for every translation unit, either as compile definitions or in an
``ouly_user_defines.hpp`` header on the include path, which ``ouly/utility/user_config.hpp``
includes automatically when it exists.

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - Macro
     - Effect
   * - ``OULY_ASSERT``
     - Assertion macro used internally; defaults to ``assert``.
   * - ``OULY_SCHEDULER_VERSION``
     - ``v1``, ``v2`` (default) or ``v3``: the generation behind ``ouly::scheduler`` and
       ``ouly::task_context``.
   * - ``OULY_SCHEDULER_PMR_INTERNAL_ALLOCATOR``
     - See the CMake option of the same name.
   * - ``OULY_CACHE_LINE_SIZE``
     - Cache line size used for padding (default 64).
   * - ``OULY_SAFE_BOUNDS``
     - Bounds-check internal container indexing.
   * - ``OULY_PACK_TAGGED_POINTER``
     - Make ``ouly::tagged_ptr`` pack its tag into the pointer.
   * - ``OULY_ANY_DEFAULT_INLINE_SIZE``
     - Inline buffer size of ``ouly::any`` (default 32).
   * - ``OULY_USE_LARGE_SIZE_TYPE``
     - Serialize container sizes as 64-bit instead of 32-bit values.
   * - ``OULY_PRINT_DEBUG``, ``OULY_PRINT_ERROR``
     - Functions used to print debug output and errors.

First program
-------------

.. code-block:: cpp

   #include <ouly/scheduler/parallel_for.hpp>
   #include <ouly/scheduler/scheduler.hpp>

   #include <cstdint>
   #include <iostream>
   #include <numeric>
   #include <vector>

   int main()
   {
     // Workgroup 0 spans worker threads [0, 4). Worker 0 is the thread that calls begin_execution().
     ouly::scheduler scheduler;
     scheduler.create_group(ouly::default_workgroup_id, 0, 4);
     scheduler.begin_execution();

     auto const& ctx = ouly::task_context::this_context::get();

     std::vector<std::uint32_t> values(100'000);
     std::iota(values.begin(), values.end(), 0U);

     // Split the range across the workgroup; returns once every element is processed.
     ouly::parallel_for([](std::uint32_t& value, ouly::task_context const&) { value *= 2; }, values, ctx);

     // A value task; get(ctx) runs queued work on this thread while it waits.
     auto sum = ouly::submit_task(ctx, [&values] { return std::accumulate(values.begin(), values.end(), std::uint64_t{0}); });
     std::cout << "sum = " << sum.get(ctx) << '\n';

     scheduler.end_execution();
   }

Build it with the ``CMakeLists.txt`` above. The program starts a scheduler with four workers (the
calling thread is worker 0), doubles 100,000 values in parallel and sums them in a task.

Library layout
--------------

.. list-table::
   :header-rows: 1
   :widths: 35 65

   * - Headers
     - Contents
   * - ``ouly/scheduler/``
     - Schedulers, tasks, ``parallel_for``, coroutines, flow graphs (:doc:`tutorials/scheduler_tutorial`)
   * - ``ouly/allocators/``
     - Allocators, memory-mapped files, coalescing suballocators (:doc:`tutorials/memory_management`)
   * - ``ouly/containers/``
     - Containers and queues (:doc:`tutorials/containers_tutorial`)
   * - ``ouly/ecs/``
     - Entity component system (:doc:`tutorials/ecs_tutorial`)
   * - ``ouly/reflection/``, ``ouly/serializers/``
     - Reflection, binary and YAML serialization (:doc:`tutorials/serialization_tutorial`)
   * - ``ouly/utility/``, ``ouly/dsl/``
     - Utilities, macro expressions, YAML event parser (:doc:`tutorials/utilities_tutorial`)
