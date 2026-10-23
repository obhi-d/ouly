User Guide
==========

This guide helps pick the right component for a job. The tutorials cover how to use each one.

Choosing a scheduler generation
-------------------------------

All three generations share the submission API, so switching is a matter of setting
``OULY_SCHEDULER_VERSION`` (see :doc:`../tutorials/scheduler_tutorial`).

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Situation
     - Choice
   * - General purpose, the default
     - ``v2``: work-stealing deques, mailboxes for cross-group work, workers assigned to the
       workgroups that advertise work.
   * - The process shares the machine with render, audio or IO threads, or runs on battery
     - ``v3``: workers belong to fixed workgroups and sleep on a condition variable as soon as
       nothing is queued for them.
   * - Comparing against the original design
     - ``v1``: per-worker rings in each workgroup with randomized stealing.

Within a generation:

* Use ``submit`` for small fire-and-forget callables (they must fit the 64-byte task delegate).
* Use ``submit_task`` when you need the result, a continuation or exception propagation.
* Use ``task_scope`` for fork/join inside a function, and ``parallel_for`` or
  ``auto_parallel_for`` for loops (``auto_parallel_for`` when the cost per element varies).
* Use ``flow_graph`` for a dependency graph that runs to completion, possibly repeatedly, and
  ``dynamic_flow_graph`` for a persistent loop that changes while it runs.

Choosing an allocator
---------------------

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - Allocation pattern
     - Allocator
   * - Everything is freed at once (per frame, per request)
     - ``linear_arena_allocator`` and ``rewind()``; ``linear_allocator`` when a fixed buffer is enough
   * - Nested temporary scopes
     - ``linear_stack_allocator`` with rewind points
   * - The same, from many threads
     - ``ts_shared_linear_allocator`` (shared pages) or ``ts_thread_local_allocator`` (per-thread pages)
   * - Many objects of one type
     - ``object_pool<T>``
   * - Many blocks of a few fixed sizes
     - ``pool_allocator``
   * - Ranges inside memory you do not address directly (one GPU buffer)
     - ``coalescing_allocator``
   * - Ranges across many heaps, with defragmentation
     - ``coalescing_arena_allocator``
   * - Whole pages, or file contents
     - ``virtual_allocator``, ``mmap_source`` / ``mmap_sink``

See :doc:`../tutorials/memory_management`.

Choosing a container
--------------------

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - Need
     - Container
   * - Usually a handful of elements, no heap allocation for them
     - ``small_vector<T, N>``
   * - Loops that touch one member of many objects
     - ``soavector<T>``
   * - Values at sparse integer keys
     - ``sparse_vector<T>``
   * - Insert and get a stable index back
     - ``sparse_table<T>`` (paged) or ``table<T>`` (one vector)
   * - Objects that are already allocated, linked without allocation
     - ``intrusive_list``
   * - Producers and consumers on different threads
     - ``concurrent_queue<T>``
   * - Named values of different types
     - ``blackboard``

See :doc:`../tutorials/containers_tutorial`.

Choosing an ECS layout
----------------------

* Dense component storage (the default) when iteration speed matters and not every entity has the
  component.
* ``cfg::use_direct_mapping`` when almost every entity has the component and lookups dominate.
* ``ecs::collection`` for tags and subsets, ``ecs::map`` when the values live in arrays you manage.

See :doc:`../tutorials/ecs_tutorial`.

Getting support
---------------

* The :doc:`../api_reference/index` documents the classes from the header comments.
* :doc:`../examples/index` points to the unit tests, which exercise every component.
* Report problems on `GitHub Issues <https://github.com/obhi-d/ouly/issues>`_.
