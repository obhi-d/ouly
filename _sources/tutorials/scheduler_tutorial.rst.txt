Task Scheduler Tutorial
=======================

OULY's scheduler runs tasks on a fixed pool of worker threads that are organized into
*workgroups*. On top of plain task submission it provides reference-counted value tasks with
continuations, structured task scopes, ``parallel_for``, lazily started coroutines and two kinds
of flow graph. All examples on this page compile against the current headers and run with each of
the three scheduler generations.

Concepts
--------

**Workers.** A scheduler owns ``N`` workers, numbered ``0..N-1``. Worker ``0`` is the thread that
calls ``begin_execution()``; the scheduler starts a thread for each of the others.

**Workgroups.** ``create_group(group, start, count, priority)`` declares that workgroup ``group``
runs on workers ``[start, start + count)``. Groups are created before ``begin_execution()`` and are
fixed afterwards. The worker count is the highest ``start + count`` of any group.

**Task context.** Every task receives a ``task_context const&`` that identifies the worker
(``get_worker()``), the workgroup it is running for (``get_workgroup()``), the scheduler
(``get_scheduler()``) and the user context pointer passed to ``begin_execution()``
(``get_user_context<T>()``). Every submission takes the submitter's context. On worker 0 outside a
task, get it with ``task_context::this_context::get()``.

**Lifecycle.** ``create_group`` ... ``begin_execution()`` ... submit work ...
``end_execution()``. ``end_execution()`` waits for the submitted work and joins the worker threads.
When several schedulers exist, call ``take_ownership()`` from the main thread before using one.

Choosing a scheduler generation
-------------------------------

``ouly/scheduler/scheduler.hpp`` includes three implementations with the same submission API:

.. list-table::
   :header-rows: 1
   :widths: 15 85

   * - Generation
     - Design
   * - ``ouly::v1``
     - The original design: a bounded lock-free ring per worker inside each workgroup plus a
       locked overflow queue, with randomized stealing between the workers of a group. A worker
       that finds nothing sleeps on its own semaphore until a submitter wakes it.
   * - ``ouly::v2``
     - TBB-style: a Chase-Lev work-stealing deque per worker in each workgroup, mailboxes for
       cross-group submission, and workgroups that advertise pending work so idle workers can be
       assigned to them. Idle workers park on a semaphore, but they keep spinning while any task is
       still executing anywhere.
   * - ``ouly::v3``
     - Built for game engines that share the machine with render, audio and IO threads. A worker
       is a member of every workgroup whose thread range covers it, fixed at
       ``begin_execution()``, and only runs tasks of those groups (in priority order). Queue
       accounting is exact, so idle workers block on a condition variable even while long tasks run
       elsewhere, and a worker that dequeues work while more is queued wakes one more sleeper.

``ouly::scheduler``, ``ouly::task_context``, ``ouly::workgroup`` and ``ouly::task_delegate`` are
aliases for the generation named by the ``OULY_SCHEDULER_VERSION`` macro, which defaults to ``v2``.
Define it to ``v1`` or ``v3`` consistently for every translation unit, for example as a compile
definition or in ``ouly_user_defines.hpp`` (see :doc:`../getting_started`). ``ouly::task<T>`` and
``ouly::task_scope`` follow the same alias. You can also use a generation explicitly, see
`Using a generation explicitly`_.

Starting the scheduler and submitting work
------------------------------------------

.. code-block:: cpp

   #include <ouly/scheduler/scheduler.hpp>

   struct engine_state
   {
     float frame_time = 0.016F;
   };

   constexpr auto game_group      = ouly::workgroup_id(0);
   constexpr auto streaming_group = ouly::workgroup_id(1);

   void run(engine_state& state)
   {
     ouly::scheduler scheduler;
     scheduler.create_group(game_group, 0, 6);      // workers 0..5
     scheduler.create_group(streaming_group, 6, 2); // workers 6..7

     scheduler.begin_execution(
      [](ouly::worker_id const& worker)
      {
        // runs once on every worker thread (including worker 0) before work starts
        (void)worker;
      },
      &state); // user context, available to every task

     auto const& ctx = ouly::task_context::this_context::get(); // worker 0, member of game_group

     // Run in the submitter's current workgroup
     scheduler.submit(ctx,
                      [](ouly::task_context const& task)
                      {
                        auto* engine = task.get_user_context<engine_state>();
                        update_physics(engine->frame_time);
                      });

     // Run on the streaming workers; ouly::async(ctx, group, ...) is shorthand for the same call
     ouly::async(ctx, streaming_group,
                 [](ouly::task_context const& task)
                 {
                   decode(task.get_worker().get_index());
                 });

     scheduler.end_execution(); // waits for submitted work, then joins the worker threads
   }

``submit`` accepts a callable taking ``task_context const&``, a coroutine task (see
`Coroutines`_), or a function pointer followed by its arguments. Without a workgroup argument the
task goes to the submitter's current workgroup. ``submit`` stores the callable inline in an
``ouly::task_delegate``: it must fit in 64 bytes and be trivially copyable and destructible, which a
compile-time check enforces. Capture pointers or references, or use ``submit_task`` or a
``task_scope``, which keep the callable in a separately allocated node and accept any capture.

Value tasks
-----------

``ouly::submit_task`` returns a reference-counted ``task<T>``. The callable can take no arguments
or the ``task_context``. ``then`` attaches a continuation that is queued through the scheduler once
the predecessor has finished, and ``when_all`` accepts several tasks of any type or a sized range of
tasks of one type:

.. code-block:: cpp

   auto mesh     = ouly::submit_task(ctx, []() -> uint32_t { return 128; });
   auto textures = ouly::submit_task(ctx, streaming_group, []() -> uint32_t { return 16; });

   // Continuations are queued through the scheduler once the predecessor finishes
   auto vertices = mesh.then(ctx, [](uint32_t triangles) { return triangles * 3; });

   // Wait for a heterogeneous set of tasks...
   ouly::when_all(ctx, vertices, textures).get(ctx);

   // ...or for a sized range of tasks of one type
   std::vector<ouly::task<uint32_t>> chunks;
   for (uint32_t i = 0; i < 4; ++i)
   {
     chunks.push_back(ouly::submit_task(ctx, [i] { return i; }));
   }
   ouly::when_all(ctx, chunks).get(ctx);

   uint32_t count = vertices.get(ctx); // 384

   // Exceptions thrown by a task are rethrown by get(), then() and when_all()
   auto failing = ouly::submit_task(ctx, []() -> int { throw std::runtime_error("load failed"); });
   try
   {
     failing.get(ctx);
   }
   catch (std::runtime_error const&)
   {
     // handle the error
   }

``get(ctx)`` and ``wait(ctx)`` run other queued work on the calling worker while they wait;
``get()`` and ``wait()`` without a context simply block. ``submit_task`` and ``then`` also have
overloads that take a target workgroup and a ``scheduler_allocator`` (see `Where task memory comes
from`_).

Structured task scopes
----------------------

``task_scope`` is for fork/join work with lexical ownership. ``join(ctx)`` claims the scope's
children that have not started yet and runs them on the calling worker, then waits for the ones
that are already running. It never runs unrelated scheduler work while waiting, so a join cannot
pick up a long task from elsewhere. ``join()`` without a context only blocks. A scope must be joined
before it is destroyed, and no new work may be added from outside the scope after ``join`` starts.

.. code-block:: cpp

   std::vector<float> results(64);

   ouly::task_scope scope;
   for (uint32_t i = 0; i < results.size(); ++i)
   {
     scope.run(ctx, [&results, i] { results[i] = static_cast<float>(i) * 0.5F; });
   }
   scope.join(ctx); // every child has finished (or join rethrows the first exception)

``run`` returns a ``task<T>`` for the child, so results can also be read individually.

Where task memory comes from
----------------------------

``scheduler_allocator`` is a non-owning, type-erased reference to an allocator. Task states and
coroutine frames created with it are taken from that allocator, which must outlive them. The
allocator needs ``allocate(size)``; ``allocate(size, std::align_val_t)`` and
``deallocate(pointer, size)`` are used when present, so a linear allocator that is reset once per
frame works:

.. code-block:: cpp

   ouly::linear_allocator<> frame_memory(64 * 1024);
   ouly::scheduler_allocator frame_storage(frame_memory);

   auto value = ouly::submit_task(ctx, frame_storage, [] { return 42; });
   ouly::task_scope scope(frame_storage);
   scope.run(ctx, [] {});
   scope.join(ctx);
   int answer = value.get(ctx);

The scheduler's own bookkeeping nodes (the producers behind ``submit_task``, continuations,
``when_all`` and scope nodes) are always owned by the scheduler, because their lifetime is governed
by its queues. By default they use aligned ``operator new``/``delete``, so an overridden global
allocator such as mimalloc provides the thread caching. Configure with
``-DOULY_SCHEDULER_PMR_INTERNAL_ALLOCATOR=ON`` (or define ``OULY_SCHEDULER_PMR_INTERNAL_ALLOCATOR``
for every translation unit) to use a synchronized PMR pool owned by the scheduler instead. The pool
grows lazily and reuses returned blocks.

Parallel loops
--------------

``ouly::parallel_for`` (``ouly/scheduler/parallel_for.hpp``) splits a range across the workers of
the submitter's workgroup and returns when every element has been processed. The callable takes
either one element or a ``[begin, end)`` batch, plus the task context:

.. code-block:: cpp

   #include <ouly/scheduler/parallel_for.hpp>
   #include <ouly/utility/subrange.hpp>

   std::vector<uint32_t> data(10'000);
   std::iota(data.begin(), data.end(), 0U);

   // Element form
   ouly::parallel_for([](uint32_t& value, ouly::task_context const&) { value *= 2; }, data, ctx);

   // Range form: each call gets a [begin, end) batch
   ouly::parallel_for(
    [](auto begin, auto end, ouly::task_context const&)
    {
      for (auto it = begin; it != end; ++it)
      {
        *it += 1;
      }
    },
    data, ctx);

   // Index ranges: ouly::subrange<uint32_t> iterates the integers in [first, last)
   std::atomic<uint64_t> total{0};
   ouly::parallel_for([&total](uint32_t index, ouly::task_context const&) { total += index; },
                      ouly::subrange<uint32_t>{0, 1000}, ctx);

By default the range is cut into ``batches_per_worker`` (4) batches per worker, and ranges with
fewer than ``parallel_execution_threshold`` (16) elements run inline. Pass a traits type to change
this; ``fixed_batch_size`` overrides the batch count with a fixed batch size.

For work whose cost varies per element, ``ouly::auto_parallel_for``
(``ouly/scheduler/auto_parallel_for.hpp``) partitions adaptively in the style of TBB's auto
partitioner: it starts with roughly one chunk per worker, and a chunk that has been stolen is split
further (up to ``max_depth`` levels, never below ``grain_size`` elements) so idle workers find more
to steal. Ranges of up to ``sequential_threshold`` (128) elements run inline. ``parallel_for``
dispatches to it when given traits derived from ``auto_partitioner_traits``.

.. code-block:: cpp

   struct coarse_batches : ouly::default_partitioner_traits
   {
     static constexpr uint32_t batches_per_worker           = 2;
     static constexpr uint32_t parallel_execution_threshold = 1024;
   };

   struct fine_grained : ouly::auto_partitioner_traits
   {
     static constexpr uint32_t sequential_threshold = 32;
   };

.. code-block:: cpp

   ouly::parallel_for([](uint32_t& value, ouly::task_context const&) { value -= 1; }, data, ctx, coarse_batches{});

   // Uneven per-element cost: let stolen ranges split further
   ouly::auto_parallel_for([](uint32_t& value, ouly::task_context const&) { value = expensive_for(value); }, data, ctx,
                           fine_grained{});

Both loops are built on task scopes, so waiting for them never executes unrelated scheduler work.
Exceptions thrown by the callable are rethrown to the caller.

Coroutines
----------

``ouly::co_task<T>`` (``ouly/scheduler/co_task.hpp``) is a lazy coroutine: it does nothing until it
is submitted or awaited. Inside a coroutine, ``co_await`` another ``co_task`` or a ``task<T>``;
the awaiting coroutine resumes as queued scheduler work, and uncaught exceptions propagate to
whoever awaits or waits on it.

.. code-block:: cpp

   #include <ouly/scheduler/co_task.hpp>

   auto load_chunk(uint32_t index) -> ouly::co_task<uint32_t>
   {
     co_return index * 10;
   }

   auto load_level() -> ouly::co_task<uint32_t>
   {
     auto first  = load_chunk(1);
     auto second = load_chunk(2);
     // A co_task is lazy: awaiting it runs it, and the awaiting coroutine resumes as queued work
     co_return co_await first + co_await second;
   }

Submit a coroutine by reference (it must then outlive the work) or move it into the scheduler, and
wait from outside a coroutine with ``cooperative_wait(ctx)`` (which helps run work) or ``wait()``:

.. code-block:: cpp

   auto level = load_level();
   scheduler.submit(ctx, ouly::default_workgroup_id, level); // submitted by reference
   uint32_t total = level.cooperative_wait(ctx);             // 30, helps run work while waiting

Pass a ``scheduler_allocator`` as a coroutine parameter to allocate its frame from that allocator:

.. code-block:: cpp

   auto build_mesh([[maybe_unused]] ouly::scheduler_allocator allocator, uint32_t lod) -> ouly::co_task<uint32_t>
   {
     co_return lod * 100;
   }

.. code-block:: cpp

   ouly::linear_allocator<> frame_memory(16 * 1024);
   ouly::scheduler_allocator frame_storage(frame_memory);

   auto mesh = build_mesh(frame_storage, 2); // the coroutine frame comes from frame_memory
   scheduler.submit(ctx, mesh);
   uint32_t vertices = mesh.cooperative_wait(ctx);

``ouly::co_sequence<T>`` starts running immediately instead. It is meant to sequence other
coroutines from non-coroutine code and must not be submitted to a scheduler.

Flow graphs
-----------

``ouly::flow_graph<Scheduler>`` (``ouly/scheduler/flow_graph.hpp``) executes a static dependency
graph. Nodes hold any number of tasks; a node runs once all of its predecessors have finished. The
graph can be started again after it completes.

.. code-block:: cpp

   #include <ouly/scheduler/flow_graph.hpp>

   ouly::flow_graph<ouly::scheduler> graph;

   auto load    = graph.create_node();
   auto animate = graph.create_node();
   auto physics = graph.create_node();
   auto render  = graph.create_node();

   // load -> (animate, physics) -> render
   graph.connect(load, animate);
   graph.connect(load, physics);
   graph.connect(animate, render);
   graph.connect(physics, render);

   std::atomic<int> steps{0};
   graph.add(load, [&](ouly::task_context const&) { ++steps; });
   graph.add(animate, [&](ouly::task_context const&) { ++steps; });
   graph.add(physics, [&](ouly::task_context const&) { ++steps; });
   graph.add(physics, [&](ouly::task_context const&) { ++steps; }); // a node can hold several tasks
   graph.add(render, [&](ouly::task_context const&) { ++steps; });

   graph.start(ctx);
   graph.cooperative_wait(ctx); // the waiting thread executes work too

   graph.start(ctx); // the same graph can run again
   graph.cooperative_wait(ctx);

``create_node(group)`` places a node's tasks on another workgroup, and ``create_main_thread_node()``
creates a node that runs on the thread that calls ``cooperative_wait``. Use ``cooperative_wait`` and
not ``wait`` when the waiting thread is a member of the graph's workgroup, otherwise the graph can
deadlock. With the ``ouly::cfg::flow_graph_node_id`` option tasks may also take the id of the node
that launched them:

.. code-block:: cpp

   using graph_t = ouly::flow_graph<ouly::scheduler, 4, 4, ouly::config<ouly::cfg::flow_graph_node_id>>;
   graph_t graph;

   auto node = graph.create_node();
   graph.add(node, [](ouly::task_context const&, graph_t::node_id id) { (void)id.value(); });
   graph.start(ctx);
   graph.cooperative_wait(ctx);

``ouly::dynamic_flow_graph<Scheduler>`` (``ouly/scheduler/dynamic_flow_graph.hpp``) is for long
running, persistent graphs such as a game loop:

- ``create_node``, ``add``, ``remove`` and ``connect`` are thread-safe and may be called while the
  graph runs, including from its own tasks; changes apply to the next firing of the affected node.
- Cycles are allowed. A node fires every time it has received ``max(in_degree, 1)`` triggers, and a
  node that finishes sends one trigger to each successor, so a node inside a loop fires once per
  iteration.
- ``signal(node, ctx)`` injects a trigger (that is how roots and loops are started), and
  ``request_stop()`` stops trigger propagation so the running work drains. ``wait`` and
  ``cooperative_wait`` return once the graph is idle. The graph is not restarted.

.. code-block:: cpp

   #include <ouly/scheduler/dynamic_flow_graph.hpp>

   ouly::dynamic_flow_graph<ouly::scheduler> graph;

   auto update = graph.create_node();
   auto render = graph.create_node();

   // update -> render -> update: one loop iteration per frame
   graph.connect(update, render);
   graph.connect(render, update);

   int frame = 0;
   graph.add(update, [](ouly::task_context const&) { /* simulate */ });
   graph.add(render,
             [&](ouly::task_context const&)
             {
               if (++frame == 3)
               {
                 graph.request_stop(); // let in-flight work drain, then the wait returns
               }
             });

   graph.signal(update, ctx);   // seed the loop
   graph.cooperative_wait(ctx); // returns once the graph is idle

Waiting and helping
-------------------

- ``scheduler.busy_work(ctx)`` tries to run one queued task on the calling worker and returns
  whether it ran something. Use it to build your own cooperative wait loops and yield or back off
  only when it returns ``false``, as the built-in waits do. All three generations report the result
  (``v3`` used to return ``false`` unconditionally).
- ``scheduler.wait_for_tasks()``, called from worker 0, helps with work and returns once everything
  submitted so far has finished.
- ``ctx.cooperative_wait(semaphore)`` keeps the worker busy until a ``std::binary_semaphore`` is
  released.

Using a generation explicitly
-----------------------------

The free functions and class templates take any task context, so a specific generation can be used
next to the default alias. ``task<T>`` and ``task_scope`` are aliases for the default context; with
another generation use ``basic_task`` and ``basic_task_scope`` (or ``auto``):

.. code-block:: cpp

   ouly::v3::scheduler scheduler;
   scheduler.create_group(ouly::default_workgroup_id, 0, 4);
   scheduler.begin_execution();
   auto const& ctx = ouly::v3::task_context::this_context::get();

   // submit_task/when_all/parallel_for take any task context; the task type follows it
   ouly::basic_task<int, ouly::v3::task_context> answer = ouly::submit_task(ctx, [] { return 42; });

   ouly::basic_task_scope<ouly::v3::task_context> scope;
   scope.run(ctx, [] {});
   scope.join(ctx);

   std::vector<uint32_t> data(1000, 1);
   ouly::parallel_for([](uint32_t& v, ouly::v3::task_context const&) { v += 1; }, data, ctx);

   int value = answer.get(ctx);
   scheduler.end_execution();

``co_task`` works with the context named by ``OULY_SCHEDULER_VERSION``; to use coroutines with
``v1`` or ``v3``, select that generation through the macro.

Next Steps
----------

* :doc:`memory_management` for the allocators that pair well with per-frame task memory
* :doc:`ecs_tutorial` for data to process with ``parallel_for``
* :doc:`../performance/index` for the benchmarks that compare ``v1`` and ``v2`` with oneTBB
