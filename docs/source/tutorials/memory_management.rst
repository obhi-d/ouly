Memory Management Tutorial
==========================

OULY's allocators cover the allocation patterns that general-purpose ``new``/``delete`` handles
poorly: memory that is thrown away all at once, many objects of one size, allocations from several
threads within a frame, and suballocation of memory that you do not address with pointers (GPU heaps
and buffers). Every example on this page compiles against the current headers.

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Allocator
     - Use it for
   * - ``linear_allocator``
     - One fixed buffer, bump allocation, LIFO deallocation of the last block
   * - ``linear_arena_allocator``
     - Bump allocation that grows by arenas; ``rewind()`` reuses everything
   * - ``linear_stack_allocator``
     - Arena bump allocation with rewind points (scoped temporaries)
   * - ``pool_allocator``
     - Blocks made of fixed-size atoms
   * - ``object_pool<T>``
     - Objects of one type, recycled through an intrusive free list
   * - ``ts_shared_linear_allocator``
     - Lock-free bump allocation shared by many threads, reset per frame
   * - ``ts_thread_local_allocator``
     - Per-thread bump allocation with no synchronization on the fast path
   * - ``virtual_allocator``
     - Whole pages from the operating system
   * - ``mmap_source`` / ``mmap_sink``
     - Read-only / read-write memory-mapped files
   * - ``coalescing_allocator``
     - Offsets inside one range, such as a GPU buffer
   * - ``coalescing_arena_allocator``
     - Offsets inside many arenas (GPU heaps), with defragmentation

The allocator interface
-----------------------

Allocators take sizes in bytes and an optional alignment, spelled either as the compile-time tag
``ouly::alignment<N>`` or, when the value is only known at runtime, as a ``std::align_val_t``. The
same size and alignment have to be handed back to ``deallocate``, as with the aligned
``operator new``/``operator delete`` pair. ``ouly::allocate<T>`` and ``ouly::deallocate`` in
``ouly/allocators/allocator.hpp`` cast the result and default the alignment to ``alignof(T)``; they
do not construct objects.

.. code-block:: cpp

   ouly::linear_allocator<> allocator(4096);

   void* raw = allocator.allocate(256);                           // bytes
   void* simd = allocator.allocate(256, ouly::alignment<32>());   // alignment known at compile time
   auto* floats = ouly::allocate<float>(allocator, 64 * sizeof(float)); // typed, aligned to alignof(float)

   std::size_t const wanted = query_device_alignment();           // alignment known only at runtime
   void*             block  = allocator.allocate(1024, std::align_val_t{wanted});

   // Hand the same size (and alignment) back. A linear allocator only reclaims the most recent block.
   allocator.deallocate(block, 1024, std::align_val_t{wanted});
   ouly::deallocate(allocator, floats, 64 * sizeof(float));
   allocator.deallocate(simd, 256, ouly::alignment<32>());
   allocator.deallocate(raw, 256);

Linear allocators
-----------------

.. code-block:: cpp

   #include <ouly/allocators/linear_allocator.hpp>

   // One fixed buffer; allocate() returns null() once it is full
   ouly::linear_allocator<> scratch(16 * 1024);
   auto*                    header = ouly::allocate<std::uint8_t>(scratch, 64);
   auto*                    body   = ouly::allocate<std::uint8_t>(scratch, 512);
   scratch.deallocate(body, 512); // LIFO: the most recent block is reclaimed

``linear_arena_allocator`` adds arenas as it fills up. ``rewind()`` makes all of them available
again; ``smart_rewind()`` additionally frees the arenas after the one currently in use.

.. code-block:: cpp

   #include <ouly/allocators/linear_arena_allocator.hpp>

   // Arenas of 64 KiB are added as needed (a larger request gets an arena of its own)
   ouly::linear_arena_allocator<> frame(64 * 1024);
   for (int i = 0; i < 1000; ++i)
   {
     [[maybe_unused]] auto* particle = ouly::allocate<float>(frame, 16 * sizeof(float));
   }
   frame.rewind();       // keep every arena, start again from the first one
   frame.smart_rewind(); // also free the arenas after the one in use

``linear_stack_allocator`` remembers positions to rewind to, which suits nested scratch memory:

.. code-block:: cpp

   #include <ouly/allocators/linear_stack_allocator.hpp>

   ouly::linear_stack_allocator<> stack(64 * 1024);
   auto*                          persistent = ouly::allocate<int>(stack, 128);
   {
     auto marker = stack.get_auto_rewind_point(); // rewinds to this point when it goes out of scope
     auto* temporary = ouly::allocate<int>(stack, 4096);
     (void)temporary;
   }
   auto point = stack.get_rewind_point(); // or rewind explicitly
   [[maybe_unused]] auto* more = ouly::allocate<int>(stack, 256);
   stack.rewind(point);

Every linear allocator, including the thread-safe ones below, supports ``realloc``. A block that
is still the most recent allocation of its arena is grown or shrunk by moving the arena head, so a
growing buffer costs nothing; any other block is reallocated and copied.

.. code-block:: cpp

   ouly::linear_arena_allocator<> allocator(1024);
   auto* buffer = static_cast<std::uint8_t*>(allocator.allocate(64));
   buffer       = static_cast<std::uint8_t*>(allocator.realloc(buffer, 64, 256)); // grows in place

The non-threaded linear allocators store a small header in front of aligned allocations so that a
LIFO ``deallocate`` restores both the payload and its alignment padding. The header reuses the
padding the allocation needs anyway and only crosses another alignment boundary when that gap is too
small. Configure ``ouly::cfg::disable_rollback`` for workloads that only rewind in bulk: it removes
the header and makes ``deallocate`` a no-op (see `Configuration`_).

Pools
-----

.. code-block:: cpp

   #include <ouly/allocators/object_pool.hpp>
   #include <ouly/allocators/pool_allocator.hpp>

   struct particle // object_pool needs sizeof(T) and alignof(T) of at least those of void*
   {
     double x = 0, y = 0, z = 0, life = 1;
   };

.. code-block:: cpp

   // Atoms of 64 bytes, 1000 atoms per arena. A request uses as many contiguous atoms as it needs.
   ouly::pool_allocator<> pool(64, 1000);
   void*                  one   = pool.allocate(64);
   void*                  three = pool.allocate(192);
   pool.deallocate(three, 192);
   pool.deallocate(one, 64);

   // Fixed type: an intrusive free list over pages of particle-sized slots
   ouly::object_pool<particle> particles;
   particle*                   p = new (particles.allocate()) particle{}; // allocate() does not construct
   p->~particle();
   particles.deallocate(p);

Thread-safe allocators
----------------------

.. code-block:: cpp

   #include <ouly/allocators/ts_shared_linear_allocator.hpp>
   #include <ouly/allocators/ts_thread_local_allocator.hpp>

   // Lock-free shared arenas: many threads bump-allocate from the same pages
   ouly::ts_shared_linear_allocator shared;
   std::vector<std::thread>         producers;
   for (int t = 0; t < 4; ++t)
   {
     producers.emplace_back(
      [&shared]
      {
        for (int i = 0; i < 100; ++i)
        {
          [[maybe_unused]] void* p = shared.allocate(128);
        }
      });
   }
   for (auto& thread : producers)
   {
     thread.join();
   }
   shared.reset(); // frame boundary: single thread, no allocation in flight

   // Per-thread pages: no synchronization on the allocation fast path
   ouly::ts_thread_local_allocator per_thread;
   void*                           q = per_thread.allocate(64);
   per_thread.reset(); // invalidates every thread's pages, single thread

Both allocate from pages of ``default_page_size`` (1 MiB) unless constructed with another page
size and also take a ``std::align_val_t``. Without one, ``ts_shared_linear_allocator`` aligns to
at least 16 bytes and ``ts_thread_local_allocator`` to ``alignof(std::max_align_t)``.
``deallocate`` only reclaims the most recent allocation and returns whether it did; under contention
it may fail. ``reset()`` recycles the pages and ``release()`` returns them to the system; both must
be called from a single thread while no other thread allocates. Each ``ts_thread_local_allocator``
instance keeps its own per-thread page, so several instances can be used from one thread.

Standard library adaptors
-------------------------

.. code-block:: cpp

   #include <ouly/allocators/default_allocator.hpp>
   #include <ouly/allocators/std_allocator_wrapper.hpp>

   // allocator_ref keeps a pointer to an allocator instance
   ouly::pool_allocator<> pool(8, 1000);
   using pool_ref = ouly::allocator_ref<std::uint64_t, ouly::pool_allocator<>>;
   std::vector<std::uint64_t, pool_ref> values{pool_ref(pool)};
   values.resize(256);

   // allocator_wrapper derives from a default-constructible allocator such as default_allocator
   ouly::vector<int> numbers = {1, 2, 3}; // std::vector<int, allocator_wrapper<int, default_allocator<>>>

Virtual memory and memory-mapped files
--------------------------------------

.. code-block:: cpp

   #include <ouly/allocators/virtual_allocator.hpp>

   // Reserve and commit whole pages from the OS
   ouly::virtual_allocator<> pages;
   void*                     region = pages.allocate(1024 * 1024);
   pages.deallocate(region, 1024 * 1024);

``virtual_allocator`` also has ``protect``, ``advise``, ``page_size()`` and
``allocation_granularity()``.

.. code-block:: cpp

   #include <ouly/allocators/mmap_file.hpp>

   ouly::mmap_sink sink; // read-write mapping
   sink.map("level.bin");
   std::ranges::fill(sink, static_cast<unsigned char>(0xAB));
   sink.sync();

   ouly::mmap_source source; // read-only mapping of bytes [4096, 4096 + 1024)
   source.map("level.bin", 4096, 1024);
   auto first = source[0]; // byte 4096 of the file
   auto count = source.size(); // 1024

``map(path, offset, length)`` maps the file from its start through ``offset + length`` and exposes
the requested ``length`` bytes starting at ``offset`` (fewer if the file is shorter);
``map(path)`` maps the whole file. Mapping errors throw ``std::system_error``.
``make_mmap_source(path[, offset, length])`` returns a mapped source directly.

Coalescing allocators
---------------------

The coalescing allocators hand out *offsets* rather than pointers. They never touch the memory, so
they can manage GPU heaps, buffers or file ranges, and the backing memory is yours.

``coalescing_allocator`` manages a single range ``[0, capacity)`` (by default the whole size type
range, so the backing buffer can grow as higher offsets are handed out):

.. code-block:: cpp

   #include <ouly/allocators/coalescing_allocator.hpp>

   ouly::coalescing_allocator allocator(64 * 1024);  // manages offsets in [0, 64 KiB)
   auto                       first  = allocator.allocate(256);     // stable allocation_id
   auto                       second = allocator.allocate(256, 64); // power-of-two alignment
   if (second == ouly::allocation_id())
   {
     // a null id means no gap can hold the request
   }
   auto offset = allocator.get_offset(second);
   allocator.deallocate(first); // merges with the neighbouring free space

   // Slide live allocations towards offset zero; ids stay valid, offsets change
   allocator.compact([&](auto from, auto to, auto size) { std::memmove(buf + to, buf + from, size); });
   offset = allocator.get_offset(second); // 0

``allocate`` and ``deallocate`` are O(1) regardless of how many allocations are live: only occupied
blocks are stored, each one owning the free gap behind it, and the gaps are filed in TLSF size bins.
Freeing a block adds its bytes and gap to its predecessor's gap, so coalescing costs nothing.
``compact`` takes an optional byte budget and reports in ``completed_`` whether everything moved; its
callback may be handed overlapping ranges and must behave like ``memmove``.

Coalescing arena allocator
~~~~~~~~~~~~~~~~~~~~~~~~~~

``coalescing_arena_allocator`` spreads suballocations over many arenas of a configurable size and
addresses them by ``(arena, offset)``. It uses the same gap-owning block list, with one set of TLSF
bins per *memory class* shared by all arenas of that class, so allocation stays O(1). Alignment is
exact and ``allocation_id`` handles are stable.

The backing memory is owned by a *manager* that satisfies ``ouly::CoalescingMemoryManager``:
``add(arena, size[, memory_class])`` creates an arena's memory and ``remove(arena)`` destroys it.

.. code-block:: cpp

   #include <ouly/allocators/coalescing_arena_allocator.hpp>

   // Owns the backing memory of each arena (here plain host memory; typically a GPU heap)
   struct heap_manager
   {
     std::vector<std::vector<std::byte>> heaps;

     void add(ouly::arena_id arena, ouly::allocation_size_type size, uint32_t /*memory_class*/)
     {
       if (heaps.size() <= arena.get())
       {
         heaps.resize(arena.get() + 1U);
       }
       heaps[arena.get()].resize(size);
     }

     void remove(ouly::arena_id arena)
     {
       heaps[arena.get()] = {};
     }
   };

.. code-block:: cpp

   heap_manager                     manager;
   ouly::coalescing_arena_allocator heaps{16U * 1024U * 1024U}; // 16 MiB per arena

   auto mesh = heaps.allocate(4096, manager, ouly::alignment<256>{});
   auto image = heaps.allocate(64U * 1024U, manager,
                               ouly::ca_allocation_options{
                                .alignment_      = 4096,
                                .memory_class_   = 1, // e.g. device-local vs host-visible
                                .resource_class_ = 2, // passed back in relocations
                               });
   auto pinned = heaps.allocate(1024, manager, ouly::ca_allocation_options{.movable_ = false});

   // Where it lives: (arena, offset)
   auto arena  = mesh.get_arena_id();
   auto offset = mesh.get_offset();

   heaps.deallocate(mesh.get_allocation_id(), manager);
   heaps.deallocate(image.get_allocation_id(), manager); // the emptied class-1 arena is kept for reuse
   heaps.deallocate(pinned.get_allocation_id(), manager);
   heaps.trim(manager); // hand the retained empty arenas back to the manager

A request of at least the arena size, or one with ``.dedicated_ = true``, gets a dedicated arena of
exactly its size; dedicated allocations are never moved.

**Empty arenas.** When a deallocation empties an arena, each memory class keeps up to
``get_retained_empty_arenas()`` empty arenas (1 by default) registered and reuses them for its next
allocations, so a per-frame allocate/free does not create and destroy backing memory every frame.
Any further empty arena of that class is returned to the manager right away, and so are dedicated
arenas and arenas emptied by an evacuation. ``trim(manager)`` returns the retained arenas and
reports how many it released. ``set_retained_empty_arenas(n)`` changes the limit (``0`` restores
immediate release); lowering it does not release arenas that are already retained until the next
``trim``.

**Defragmentation** evacuates whole arenas. A relocation executor satisfying
``ouly::CoalescingRelocationExecutor`` performs the copies: ``schedule_copy`` returns the timeline
value at which the destination holds the data, ``switch_references`` runs once that value is
reached, rebinds whatever refers to the stable ``allocation_id`` and returns the timeline value after
which the source is no longer read, and ``is_complete`` reports progress. Returning ``std::nullopt``
from either call cancels the evacuation. An executor that copies inside ``schedule_copy`` and reports
every value as complete makes defragmentation synchronous:

.. code-block:: cpp

   // Copies immediately and reports every timeline value as reached: synchronous defragmentation
   struct immediate_executor
   {
     heap_manager*           memory   = nullptr;
     ouly::ca_timeline_value timeline = 0;

     auto schedule_copy(ouly::ca_relocation const& move) -> std::optional<ouly::ca_timeline_value>
     {
       auto& from = memory->heaps[move.source_arena_.get()];
       auto& to   = memory->heaps[move.destination_arena_.get()];
       std::memcpy(to.data() + move.destination_offset_, from.data() + move.source_offset_, move.size_);
       return ++timeline;
     }

     auto switch_references(ouly::ca_relocation const& /*move*/) -> std::optional<ouly::ca_timeline_value>
     {
       // rebind whatever refers to move.allocation_ (descriptors, buffer views) to the new placement
       return ++timeline;
     }

     [[nodiscard]] auto is_complete(ouly::ca_timeline_value) const -> bool
     {
       return true;
     }
   };

.. code-block:: cpp

   immediate_executor executor{.memory = &manager};
   auto               result = heaps.defragment(manager, executor,
                                                ouly::ca_defrag_budget{
                                                 .max_copy_bytes_ = 8U * 1024U * 1024U,
                                                 .max_move_count_ = 32,
                                                });
   if (result.evacuation_active_)
   {
     // more work is pending; call defragment() again next frame
   }

Before an arena is frozen, the allocator reserves an alignment-aware destination for every
allocation it holds (largest and most aligned first) and rolls the reservations back if any of them
does not fit, so an evacuation never ends half done. ``ca_defrag_budget`` limits the copied bytes and
the number of moves per call; ``process_completions`` advances pending copies without scheduling new
ones. Non-movable allocations (``.movable_ = false`` or ``set_movable``) pin their arena. The
allocator is not internally synchronized.

Planning an evacuation needs temporary buffers. ``defragment`` takes them from a call-local
``linear_stack_allocator`` by default, or from any allocator satisfying ``ouly::ScratchAllocator``
(any of the linear allocators) passed as the last argument:

.. code-block:: cpp

   ouly::linear_stack_allocator<> frame_scratch(64 * 1024); // rewound once per frame

   heaps.defragment(manager, executor, ouly::ca_defrag_budget{}, frame_scratch);
   frame_scratch.rewind();

Configuration
-------------

Allocators take an ``ouly::config<...>`` of options from the ``ouly::cfg`` namespace
(``ouly/allocators/config.hpp``):

.. code-block:: cpp

   using tracked = ouly::config<ouly::cfg::min_alignment<32>, ouly::cfg::compute_stats>;
   ouly::linear_arena_allocator<tracked> allocator(64 * 1024);

   // Bulk-rewind only: no rollback header in front of aligned allocations, deallocate() is a no-op
   using bulk = ouly::config<ouly::cfg::disable_rollback>;
   ouly::linear_allocator<bulk> frame(64 * 1024);

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - Option
     - Effect
   * - ``cfg::min_alignment<N>``
     - Minimum alignment of the default and virtual allocators
   * - ``cfg::underlying_allocator<T>``
     - Where the linear, stack, arena and pool allocators get their arenas from
   * - ``cfg::allocator_type<T>``
     - Allocator used by containers (see :doc:`containers_tutorial`)
   * - ``cfg::disable_rollback``
     - Linear allocators: drop the rollback header; ``deallocate`` becomes a no-op
   * - ``cfg::atom_size<N>``, ``cfg::atom_count<N>``, ``cfg::atom_size_npt<N>``
     - Pool allocator atom size (power of two, or not) and atoms per arena
   * - ``cfg::compute_stats``, ``cfg::compute_atomic_stats``
     - Collect allocation statistics (plain or atomic counters)
   * - ``cfg::base_stats<T>``
     - Extra statistics type mixed into the allocator
   * - ``cfg::track_memory``, ``cfg::debug_tracer<T>``
     - Track live allocations of the default allocator, optionally reporting to a tracer

Next Steps
----------

* :doc:`containers_tutorial` for containers that take these allocators
* :doc:`scheduler_tutorial` for per-frame task memory through ``scheduler_allocator``
