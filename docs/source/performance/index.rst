Performance Guide
=================

OULY ships benchmarks that compare its components with Intel oneTBB and with one another. This page
describes how to build and run them, and collects the performance-relevant behaviour documented in
the tutorials.

Benchmarks
----------

The benchmarks are ``unit_tests/bench_*.cpp``, built with ``-DOULY_BUILD_BENCHMARKS=ON``. CMake
fetches `nanobench <https://github.com/martinus/nanobench>`_ and
`oneTBB <https://github.com/oneapi-src/oneTBB>`_ for them and copies the TBB runtime next to the
executables.

.. code-block:: bash

   cmake -S . -B build-bench -G Ninja -DCMAKE_BUILD_TYPE=Release \
         -DOULY_BUILD_TESTS=ON -DOULY_BUILD_BENCHMARKS=ON
   cmake --build build-bench
   cd build-bench/unit_tests
   ./bench_scheduler_comparison

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Executable
     - Measures
   * - ``bench_scheduler_comparison``
     - Task submission, ``parallel_for`` over vectors, matrix operations, a mixed workload and task
       throughput for ``v1::scheduler`` and ``v2::scheduler`` against oneTBB.
   * - ``bench_coroutine_comparison``
     - ``co_task``/``co_sequence`` chains and fan-out/fan-in against TBB equivalents.
   * - ``bench_performance``
     - ``ts_shared_linear_allocator``, ``ts_thread_local_allocator`` and
       ``coalescing_arena_allocator`` allocation, reset and fragmentation.
   * - ``bench_offset_allocators``
     - Runs the same deterministic allocate/free trace through the offset allocators and reports
       nanoseconds per free+allocate pair, fragmentation and the cost of a full defragmentation.
       ``bench_offset_allocators validate`` checks placements for overlap and alignment instead.

The nanobench-based benchmarks print tables and write JSON results named
``<compiler>-<commit>-<build>-<test>.json``; the commit and build number come from the
``GITHUB_SHA`` and ``GITHUB_RUN_NUMBER`` environment variables (``local`` and ``0`` otherwise).
``bench_performance`` also accepts an output file name as its first argument.

Run benchmarks on an otherwise idle machine, in a Release build, and compare results from the same
machine only.

Performance notes
-----------------

**Scheduling**

* ``submit`` stores the callable in a 64-byte inline delegate and never allocates; ``submit_task``
  and ``task_scope`` allocate a node per task (from a ``scheduler_allocator`` if you pass one).
* ``get(ctx)``, ``wait(ctx)`` and ``cooperative_wait`` run queued work while they wait;
  ``task_scope::join(ctx)`` only runs the scope's own children.
* ``auto_parallel_for`` splits stolen ranges further, which pays off for uneven work; for uniform
  work ``parallel_for`` with suitable ``batches_per_worker`` is slightly cheaper.
* ``v3::scheduler`` lets idle workers sleep as soon as nothing is queued for them, at the cost of a
  wake-up when work arrives; ``v2`` workers keep spinning while any task runs.

**Memory**

* Linear allocators turn per-frame allocation into a pointer bump and freeing into a ``rewind``.
* ``coalescing_allocator`` and ``coalescing_arena_allocator`` allocate and free in O(1) regardless
  of the number of live allocations, and the arena allocator keeps one empty arena per memory class
  so allocate/free cycles do not create and destroy backing memory.
* Defragmentation runs incrementally under a byte and move budget per call.

**Containers**

* ``small_vector`` keeps its heap buffer once it has one; call ``shrink_to_fit`` to give it back.
* ``soavector`` stores each member contiguously, so loops over one member load only that member.
* ``block_vector`` keeps storage a multiple of the block size so SIMD loops need no scalar tail.

Profiling
---------

The library has no profiler integration of its own; use the usual tools (``perf``, VTune,
Instruments, Tracy, ...). Allocators can collect counters with ``ouly::cfg::compute_stats`` (see
:doc:`../tutorials/memory_management`).
