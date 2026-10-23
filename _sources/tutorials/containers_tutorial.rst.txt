Containers Tutorial
===================

The containers in ``ouly/containers/`` fill gaps in the standard library: small-buffer and
structure-of-arrays vectors, sparse storage with stable indices, intrusive and concurrent queues,
and a heterogeneous blackboard. Most of them take their memory from a configurable allocator (see
`Configuration`_). Every example on this page compiles against the current headers.

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Container
     - Use it for
   * - ``small_vector<T, N>``
     - A vector that keeps up to ``N`` elements inside the object
   * - ``dynamic_array<T>``, ``fixed_array<T, N>``
     - Heap arrays whose size only changes on request
   * - ``soavector<T>``
     - One contiguous array per member of an aggregate (structure of arrays)
   * - ``sparse_vector<T>``
     - Elements at sparse indices, stored in pages that exist only where indices do
   * - ``sparse_table<T>``, ``table<T>``
     - Insertion returns an index that stays valid until erased; erased slots are reused
   * - ``block_vector<T, B>``
     - A vector whose storage is a multiple of ``B`` elements, for SIMD tails
   * - ``index_map``, ``bounded_bitset``
     - Index remapping and bitsets that skip the unused low range
   * - ``intrusive_list``
     - Linked lists through a hook member, without allocation
   * - ``basic_queue``, ``concurrent_queue``
     - Single-threaded and multi-producer/multi-consumer FIFO queues
   * - ``blackboard``
     - Values of any type stored by name (or type) in pooled pages

Small vector
------------

``small_vector<T, N>`` stores up to ``N`` elements in the object itself and moves them to the heap
when more are needed. Where the elements live is an explicit state, reported by ``is_inlined()``,
and once the vector is on the heap it stays there like a ``std::vector`` would:

.. code-block:: cpp

   #include <ouly/containers/small_vector.hpp>

   ouly::small_vector<std::string, 4> names; // up to 4 strings stored in the object itself
   names.emplace_back("a");
   names.emplace_back("b");
   assert(names.is_inlined());

   names.reserve(16); // moves to the heap now, even though only 2 elements are stored
   assert(!names.is_inlined());

   names.clear(); // like std::vector: the heap buffer is kept for reuse
   assert(!names.is_inlined() && names.capacity() >= 16);

   names.emplace_back("c");
   names.shrink_to_fit(); // the only call that moves the elements back inline
   assert(names.is_inlined());

- ``clear()``, ``pop_back()``, ``erase()`` and a shrinking ``resize()`` keep the heap buffer.
- ``reserve(n)`` allocates as soon as ``n`` exceeds the current capacity, even while the elements
  are inline.
- ``shrink_to_fit()`` is the only call that gives the heap buffer back. It moves the elements inline
  when they fit, otherwise it shrinks the heap buffer to the size.
- Growth (``push_back``, ``emplace_back``, ``insert`` and ``resize``) is geometric.

The rest of the interface follows ``std::vector``:

.. code-block:: cpp

   ouly::small_vector<int, 8> values = {1, 2, 3};
   values.insert(values.begin() + 1, 10);   // 1 10 2 3
   values.erase(values.begin());            // 10 2 3
   values.resize(32);                       // grows geometrically, like push_back
   values.assign({4, 5});                   // keeps the capacity
   auto inline_capacity = ouly::small_vector<int, 8>::get_inlined_capacity(); // at least 8

The inline capacity can be larger than ``N`` when ``N`` elements take less room than the heap
pointer and capacity they share space with; ``get_inlined_capacity()`` reports it.

Heap arrays
-----------

.. code-block:: cpp

   #include <ouly/containers/array_types.hpp>

   ouly::dynamic_array<float> weights(64U, 1.0F); // 64 elements, heap allocated, size fixed until resize()
   weights.resize(128, 0.0F);

   ouly::fixed_array<int, 16> slots(16U, 0); // 16 elements, heap allocated, size fixed at compile time
   slots[0] = 1;

``dynamic_array`` has no ``push_back``: ``resize`` reallocates to exactly the requested size. Note
that ``dynamic_array<int>(10, 0)`` picks the iterator-pair constructor; pass the count as
``uint32_t`` (``10U``).

Structure of arrays
-------------------

``soavector<T>`` takes an aggregate and stores each member in its own contiguous array, so a loop
that touches one member streams through memory without loading the others:

.. code-block:: cpp

   #include <ouly/containers/soavector.hpp>

   struct particle
   {
     float x, y, z;
     float life;
   };

.. code-block:: cpp

   ouly::soavector<particle> particles;
   particles.emplace_back(0.0F, 1.0F, 2.0F, 10.0F);
   particles.push_back({3.0F, 4.0F, 5.0F, 20.0F});

   // Each member is its own contiguous array
   float* xs   = particles.data<0>();
   float* life = particles.data<3>();
   for (uint32_t i = 0; i < particles.size(); ++i)
   {
     xs[i] += 1.0F;
     life[i] -= 1.0F;
   }

   float last_life = particles.back<3>(); // 19
   float second_x  = particles.at<0>(1);  // 4

``data<I>()``, ``at<I>(n)``, ``front<I>()``, ``back<I>()`` and ``ibegin<I>()``/``iend<I>()``
address member ``I``; ``operator[]``, ``at`` and the regular iterators yield all members of an
element together. ``resize`` grows geometrically.

Sparse storage
--------------

.. code-block:: cpp

   #include <ouly/containers/sparse_vector.hpp>

   ouly::sparse_vector<int> by_id; // storage is allocated page by page, only where indices exist
   by_id.emplace_at(3, 30);
   by_id.emplace_at(100000, 7);

   bool has = by_id.contains(3);
   int  v   = by_id[3];
   by_id.erase(3);

``sparse_table`` and ``table`` hand out the index instead of taking it:

.. code-block:: cpp

   #include <ouly/containers/sparse_table.hpp>
   #include <ouly/containers/table.hpp>

   ouly::sparse_table<std::string> strings;
   auto hello = strings.emplace("hello"); // a link (index) that stays valid until erased
   auto world = strings.emplace("world");
   strings.erase(hello);                  // the slot is reused by a later emplace
   std::string const* found = strings.get_if(world);

.. code-block:: cpp

   ouly::table<std::string> names; // dense vector with a free list of erased slots
   auto index = names.emplace("first");
   names[index] += "!";
   names.erase(index);

Both also iterate with ``for_each``, page by page.

Blocks, index maps and bitsets
------------------------------

.. code-block:: cpp

   #include <ouly/containers/block_vector.hpp>

   // Storage is kept at a multiple of 8 elements so SIMD loops can process whole blocks
   ouly::block_vector<float, 8> lanes;
   lanes.push_back(1.0F);
   lanes.push_back(2.0F);
   auto size         = lanes.size();         // 2
   auto storage_size = lanes.storage_size(); // 8

.. code-block:: cpp

   #include <ouly/containers/bounded_bitset.hpp>
   #include <ouly/containers/index_map.hpp>

   ouly::index_map<uint32_t> remap; // key -> value array that starts at the smallest key used
   remap[1000] = 3;
   bool mapped = remap.contains(1000);

   ouly::bounded_bitset<uint32_t> visible; // bitset that does not store the words below its lowest bit
   visible.resize(4096); // make room for bits up to 4096 first
   visible.set(4096);
   bool on = visible.test(4096);

Intrusive list
--------------

The list links objects through a ``list_hook`` member, so pushing and removing never allocate and
the objects stay where they are:

.. code-block:: cpp

   #include <ouly/containers/intrusive_list.hpp>

   struct job
   {
     int             priority = 0;
     ouly::list_hook hook{}; // the list links through this member
   };

   using job_list = ouly::intrusive_list<&job::hook>; // also caches size and tail by default

.. code-block:: cpp

   job a{.priority = 1}, b{.priority = 2}, c{.priority = 3};
   job_list jobs;
   jobs.push_back(a);
   jobs.push_back(b);
   jobs.push_front(c);
   for (job& j : jobs)
   {
     j.priority *= 2;
   }
   jobs.clear(); // unlinks; the objects are owned elsewhere

The template arguments after the hook pointer are ``CacheSize`` (maintain an element count) and
``CacheTail`` (keep a tail pointer, which ``push_back`` needs); both default to ``true``. A
``list_hook`` member makes the list doubly linked (``insert``, ``erase``, ``pop_back``); an
``slist_hook`` makes it singly linked.

Queues
------

.. code-block:: cpp

   #include <ouly/containers/basic_queue.hpp>
   #include <ouly/containers/concurrent_queue.hpp>

   ouly::basic_queue<int> fifo; // single-threaded FIFO over linked blocks
   fifo.emplace_back(1);
   fifo.push_back(2);
   int front = fifo.pop_front(); // 1

   ouly::concurrent_queue<int> mpmc; // multi-producer, multi-consumer
   std::thread producer([&mpmc] {
     for (int i = 0; i < 100; ++i)
     {
       mpmc.emplace(i);
     }
   });
   producer.join();

   int value = 0;
   int count = 0;
   while (mpmc.try_dequeue(value))
   {
     ++count;
   }

``concurrent_queue`` is built from a chain of fixed-size buckets. Producers claim slots with an
atomic increment and publish them through a per-slot commit flag; consumers claim committed slots
with a compare-and-swap. Element operations run under a shared lock that costs two atomic
operations when uncontended, and only bucket transitions (growing the tail, retiring the head) take
it exclusively. With a single producer the dequeue order is the enqueue order.

Configured with ``ouly::cfg::single_threaded_consumer_for_each``, the queue drops
``try_dequeue`` and instead offers ``for_each`` and ``clear`` for single-threaded traversal after
the producers are done, which suits collect-then-process patterns.

Blackboard
----------

.. code-block:: cpp

   #include <ouly/containers/blackboard.hpp>

   ouly::blackboard board; // heterogeneous values by name, stored in pooled pages
   board.emplace<float>("gravity", 9.81F);
   board.emplace<std::string>("level", "intro");

   float gravity = board.get<float>("gravity");
   if (auto* level = board.get_if<std::string>("level"))
   {
     *level = "boss";
   }
   board.erase("level");

By default the keys are ``std::string``\ s. ``ouly::cfg::map<std::unordered_map>`` (or another
map template) makes the blackboard keyed by type: ``emplace<T>(...)``, ``get<T>()`` and
``get_if<T>()`` without a name. ``ouly::cfg::name_val_map<Map>`` supplies the complete lookup map.

Configuration
-------------

Containers take an ``ouly::config<...>`` of options from the ``ouly::cfg`` namespace:

.. code-block:: cpp

   // Containers get their memory from cfg::allocator_type (default_allocator<> otherwise)
   using frame_vector_config = ouly::config<ouly::cfg::allocator_type<ouly::default_allocator<>>>;
   ouly::small_vector<int, 4, frame_vector_config> ids;

   // Pages of 256 elements instead of the default
   ouly::sparse_vector<int, ouly::config<ouly::cfg::pool_size<256>>> sparse;

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - Option
     - Effect
   * - ``cfg::allocator_type<A>``
     - Allocator the container uses (``default_allocator<>`` otherwise)
   * - ``cfg::pool_size<N>``
     - Elements per page of paged containers (``sparse_vector``, ``sparse_table``, ``basic_queue``,
       ``concurrent_queue``, ``blackboard``)
   * - ``cfg::basic_size_type<T>``
     - Size type of the container
   * - ``cfg::self_index_member<&T::member>``
     - ``sparse_table`` and ECS components keep each element's own index in this member
   * - ``cfg::assume_pod``, ``cfg::no_fill``, ``cfg::zero_out_memory``
     - ``small_vector``, ``sparse_vector``, ``basic_queue``: treat elements as trivial, skip value
       initialization on resize, or zero new memory
   * - ``cfg::trivially_destroyed_on_move``
     - ``small_vector``: moved-from elements need no destructor call
   * - ``cfg::single_threaded_consumer_for_each``
     - ``concurrent_queue`` traversal mode (see `Queues`_)
   * - ``cfg::map<M>``, ``cfg::name_map<M>``, ``cfg::name_val_map<M>``
     - ``blackboard`` key types and lookup maps

ECS-specific storage options (``use_sparse``, ``use_direct_mapping`` and the index pool sizes) are
covered in :doc:`ecs_tutorial`.

Next Steps
----------

* :doc:`ecs_tutorial` builds an entity component system from these pieces
* :doc:`memory_management` for allocators to plug into ``cfg::allocator_type``
