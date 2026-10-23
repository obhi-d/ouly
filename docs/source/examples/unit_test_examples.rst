Unit Test Examples
==================

The excerpts below are included straight from the test sources, so they always show the code
that CI compiles and runs.

Scheduler: continuations
------------------------

``then`` attaches continuations that the scheduler queues once the producer finishes.

.. literalinclude:: ../../../unit_tests/scheduler_task_tests.cpp
   :language: cpp
   :start-at: TEST_CASE("scheduler tasks chain queued continuations"
   :end-before: TEST_CASE("when_all completes

Scheduler: scoped joins
-----------------------

``task_scope::join(ctx)`` runs only the scope's own children while it waits.

.. literalinclude:: ../../../unit_tests/scheduler_task_tests.cpp
   :language: cpp
   :start-at: TEST_CASE("task_scope cooperative join executes only its own children"
   :end-before: TEST_CASE("task_scope queued nodes outlive its allocator"

auto_parallel_for: range splitting
----------------------------------

The range pool behind ``auto_parallel_for`` splits a range into disjoint pieces that cover it
exactly once.

.. literalinclude:: ../../../unit_tests/test_auto_parallel_for.cpp
   :language: cpp
   :start-at: TEST_CASE("range_pool: split_to_fill
   :end-before: TEMPLATE_TEST_CASE("Auto Parallel For visits every element exactly once"

small_vector: storage mode
--------------------------

Once on the heap, a ``small_vector`` keeps its buffer until ``shrink_to_fit``.

.. literalinclude:: ../../../unit_tests/small_vector.cpp
   :language: cpp
   :start-at: TEST_CASE("small_vector: heap buffer is kept until shrink_to_fit"
   :end-before: TEST_CASE("small_vector: storage mode flag does not grow the object"

coalescing_arena_allocator: retained arenas
-------------------------------------------

One emptied arena per memory class is kept for reuse until ``trim``.

.. literalinclude:: ../../../unit_tests/coalescing_arena_allocator.cpp
   :language: cpp
   :start-at: TEST_CASE("coalescing_arena_allocator: emptied arenas are retained for reuse until trimmed"
   :end-before: // NOLINTEND

Memory-mapped file ranges
-------------------------

``map(path, offset, length)`` exposes ``length`` bytes starting at ``offset``.

.. literalinclude:: ../../../unit_tests/memory_mapped_allocators.cpp
   :language: cpp
   :start-at: TEST_CASE("Validate mmap_source with offset"
   :end-before: // NOLINTEND

ECS: collections
----------------

.. literalinclude:: ../../../unit_tests/ecs_collection_tests.cpp
   :language: cpp
   :start-at: TEMPLATE_TEST_CASE("collection: for_each visits exactly the inserted set
   :end-before: TEST_CASE("components: direct mapping for_each skips holes

microexpr: operators and identifiers
------------------------------------

.. literalinclude:: ../../../unit_tests/microexpr_tests.cpp
   :language: cpp
   :start-at: TEST_CASE("microexpr: logical operators, negation and identifiers"
   :end-before: // NOLINTEND
