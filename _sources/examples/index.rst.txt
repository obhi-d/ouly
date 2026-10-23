Examples
========

The unit tests in ``unit_tests/`` are OULY's most complete set of examples: every component is
exercised there, and the tests are built and run by CI on every supported compiler, so they always
match the current API. The tutorials (:doc:`../tutorials/index`) are the place to start; the tests
show edge cases and less common features.

.. toctree::
   :maxdepth: 2

   unit_test_examples

Where to look
-------------

``unit_tests/`` is a flat directory with one or more files per component:

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Area
     - Test files
   * - Scheduler
     - ``scheduler_tests.cpp``, ``scheduler_task_tests.cpp``, ``scheduler_version_v1.cpp``,
       ``scheduler_version_v2.cpp``, ``scheduler_version_v3.cpp``,
       ``test_auto_parallel_for.cpp``, ``flow_graph_tests.cpp``, ``dynamic_flow_graph_tests.cpp``,
       ``spmc_ring.cpp``
   * - Allocators
     - ``linear_allocator.cpp``, ``pool_allocator.cpp``, ``object_pool.cpp``,
       ``thread_safe_allocators.cpp``, ``memory_mapped_allocators.cpp``,
       ``coalescing_allocator.cpp``, ``coalescing_arena_allocator.cpp``, ``defrag_allocator.cpp``
   * - Containers
     - ``small_vector.cpp``, ``dynamic_array.cpp``, ``soavector.cpp``, ``sparse_vector.cpp``,
       ``sparse_vector_iterators.cpp``, ``sparse_table.cpp``, ``table.cpp``, ``block_vector.cpp``,
       ``index_map.cpp``, ``bounded_bitset.cpp``, ``intrusive_list.cpp``, ``basic_queue.cpp``,
       ``concurrent_queue.cpp``, ``blackboard.cpp``
   * - ECS
     - ``ecs_tests.cpp``, ``ecs_comprehensive_tests.cpp``, ``ecs_registry_apis.cpp``,
       ``ecs_components_iterators.cpp``, ``ecs_collection_tests.cpp``, ``ecs_map_tests.cpp``
   * - Serialization
     - ``binary_serializer.cpp``, ``binary_stream.cpp``, ``yaml_input_serializer.cpp``,
       ``yaml_output_serializer.cpp``, ``yaml_object.cpp``, ``yaml_runtime_type.cpp``,
       ``yaml_runtime_type_convert.cpp``, ``yaml_large_data_test.cpp``, ``input_serializer.cpp`` and
       ``output_serializer.cpp`` (adapters for nlohmann::json)
   * - Utilities and DSL
     - ``basic_tests.cpp``, ``string_utils.cpp``, ``program_args.cpp``, ``projected_view.cpp``,
       ``name_deduction.cpp``, ``uint128_tests.cpp``, ``microexpr_tests.cpp``

``examples/ecs_map_example.cpp`` is a standalone walk-through of ``ouly::ecs::map``. It is not part
of the CMake build; compile it against the include directory directly.

Building and running the tests
------------------------------

.. code-block:: bash

   cmake --preset=linux-default            # or macos-default / windows-test
   cmake --build build/linux-default
   ctest --test-dir build/linux-default --output-on-failure

   # One executable per test target, named test_<name> (see unit_tests/CMakeLists.txt)
   ctest --test-dir build/linux-default -R scheduler
   ./build/linux-default/unit_tests/test_small_vector "[capacity]"   # Catch2 tag filter

Adding an example
-----------------

New usage examples belong in the unit tests, where CI keeps them compiling. Add a ``TEST_CASE`` to
the matching file, or a new file registered with ``add_unit_test(NAME ... FILES ...)`` in
``unit_tests/CMakeLists.txt``; see :doc:`../contributing`.
