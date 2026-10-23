Entity Component System Tutorial
================================

OULY's ECS is a small set of independent building blocks rather than a framework: a registry that
hands out entity ids, component storages keyed by entity, collections that track subsets of
entities, and a sparse-to-dense map. You decide how to combine them into systems. Every example on
this page compiles against the current headers.

None of these types are internally synchronized; share them between threads only with external
synchronization, or partition the work (for example with ``parallel_for`` over a storage's values).

Entities and registries
-----------------------

An entity is a typed integer handle. ``ouly::ecs::entity<>`` is just a slot index;
``ouly::ecs::rxentity<>`` also carries revision bits. A registry hands out free slots and recycles
erased ones:

.. code-block:: cpp

   #include <ouly/ecs/registry.hpp>

   ouly::ecs::registry<> registry;

   auto player = registry.emplace();
   auto enemy  = registry.emplace();
   auto index  = player.get(); // the entity's slot index

   registry.erase(enemy);
   auto bullet = registry.emplace(); // reuses enemy's slot

   registry.for_each_index([](uint32_t live_index) { (void)live_index; });

``rxregistry<>`` pairs with ``rxentity<>``: each recycled slot gets a new revision, so a handle
that outlived its entity can be detected:

.. code-block:: cpp

   ouly::ecs::rxregistry<> tracked; // entities carry revision bits
   auto first = tracked.emplace();
   tracked.erase(first);
   auto second = tracked.emplace(); // same slot, next revision

   bool stale = !tracked.is_valid(first); // true: the handle outlived its entity
   bool alive = tracked.is_valid(second); // true

The template parameter of ``entity<Tag>`` and ``registry<Tag>`` is a tag type, so entities of
different registries can be kept apart by the type system. ``basic_entity`` and ``basic_registry``
choose the size type and the number of revision bits.

Components
----------

``ouly::ecs::components<T, Entity, Config>`` stores one ``T`` per entity.

.. code-block:: cpp

   #include <ouly/ecs/components.hpp>

   struct position
   {
     float x = 0, y = 0;
   };

   struct velocity
   {
     float dx = 0, dy = 0;
   };

   struct health
   {
     int current = 100;
   };

.. code-block:: cpp

   ouly::ecs::registry<>             registry;
   ouly::ecs::components<position>   positions;
   ouly::ecs::components<velocity>   velocities;
   ouly::ecs::components<health>     healths;

   auto player = registry.emplace();
   auto rock   = registry.emplace();

   positions.emplace_at(player, 0.0F, 0.0F);
   velocities.emplace_at(player, 1.0F, 0.5F);
   healths.emplace_at(player);
   positions.emplace_at(rock, 5.0F, 5.0F); // no velocity, no health

   positions[player].x += 1.0F;             // the entity must have the component
   if (auto hp = healths.find(rock))        // optional-like result for entities that may not
   {
     hp->current -= 10;
   }
   bool moving = velocities.contains(rock); // false

``emplace_at`` constructs the component (or replaces an existing one) and returns a reference;
``operator[]`` and ``at`` require the entity to have the component, ``find`` returns an
optional-like value, and ``replace`` swaps in a new value. Components are not removed when an entity
is erased from the registry:

.. code-block:: cpp

   positions.erase(rock);
   registry.erase(rock); // removing the components is up to you

Systems
-------

A system is a function over one or more storages. ``for_each`` passes the entity and the value; to
combine storages, iterate the smallest one and look the entities up in the others:

.. code-block:: cpp

   // Iterate one storage and look the others up
   velocities.for_each(
    [&](ouly::ecs::entity<> e, velocity const& v)
    {
      if (auto p = positions.find(e))
      {
        p->x += v.dx;
        p->y += v.dy;
      }
    });

   // Range-for visits the values only
   for (health& h : healths)
   {
     h.current = std::min(h.current + 1, 100);
   }

``for_each(first, last, fn)`` limits the iteration to a range of slots, and
``begin_entities()``/``end_entities()`` iterate entity/value pairs.

Storage strategies
------------------

.. code-block:: cpp

   // Default: values packed densely, plus an entity -> slot index
   ouly::ecs::components<position> dense;

   // Values indexed directly by entity, with a presence bit per slot.
   // Best when most entities have the component.
   ouly::ecs::components<velocity, ouly::ecs::entity<>, ouly::config<ouly::cfg::use_direct_mapping>> direct;
   direct.set_max(registry.max_size()); // optional: grow the storage up front

   // Values kept in a paged sparse_vector instead of a std::vector, so they never move when it grows
   ouly::ecs::components<health, ouly::ecs::entity<>, ouly::config<ouly::cfg::use_sparse>> sparse;

.. list-table::
   :header-rows: 1
   :widths: 35 65

   * - Configuration
     - Layout
   * - default
     - Values packed densely; an index maps entities to slots and slots back to entities.
       Iteration touches only live values; erasing swaps the last value into the hole.
   * - ``cfg::use_direct_mapping``
     - Values stored at the entity's index, with a presence bit per slot. Fastest lookup, memory
       proportional to the largest entity index. ``set_max`` grows it up front.
   * - ``cfg::use_sparse``
     - The values live in a paged ``sparse_vector`` instead of a ``std::vector``, so growing the
       storage never moves them. Combined with ``use_direct_mapping``, pages are allocated only
       where entity indices are in use.

Further options (all in ``ouly::cfg``, combined with ``ouly::config<...>``): ``pool_size<N>``
(page size of the value storage), ``index_pool_size<N>``, ``keys_index_pool_size<N>`` and
``self_index_pool_size<N>`` (page sizes of the internal indexes), ``use_sparse_index``,
``keys_use_sparse_index`` and ``self_use_sparse_index`` (paged rather than flat indexes),
``custom_vector<V>`` (container for the values) and ``allocator_type<A>``.

``cfg::self_index_member<&T::member>`` makes the storage record each element's entity in a member of
the element instead of in a separate table:

.. code-block:: cpp

   struct named
   {
     std::string name;
     uint32_t    self = 0; // the storage records the owning entity here instead of in a side table
   };

   using named_components =
    ouly::ecs::components<named, ouly::ecs::entity<>, ouly::config<ouly::cfg::self_index_member<&named::self>>>;

.. code-block:: cpp

   named_components names;
   auto             e = registry.emplace();
   names.emplace_at(e, "crate");
   names.erase(e);

Collections
-----------

A ``collection`` records which entities belong to a group, in membership bits allocated page by
page, and visits a storage's components for its members only:

.. code-block:: cpp

   #include <ouly/ecs/collection.hpp>

   ouly::ecs::collection<ouly::ecs::entity<>> visible; // membership bits in lazily allocated pages

   for (int i = 0; i < 100; ++i)
   {
     auto e = registry.emplace();
     positions.emplace_at(e, static_cast<float>(i), 0.0F);
     if (i % 2 == 0)
     {
       visible.emplace(e);
     }
   }

   // Visit the components of the members only
   float sum = 0;
   visible.for_each(positions, [&sum](ouly::ecs::entity<>, position const& p) { sum += p.x; });

Collections track entity revisions whenever the entity type has revision bits, so members whose slot
was recycled are skipped consistently in every build type.

Sparse-to-dense maps
--------------------

``ouly::ecs::map`` maps entities to a dense ``0..size()-1`` range and leaves the values to you,
which suits data that lives in external arrays, GPU buffers or a ``soavector``:

.. code-block:: cpp

   #include <ouly/ecs/map.hpp>

   // Sparse entity -> dense index; you keep the values in your own arrays
   ouly::ecs::map<>         slots;
   std::vector<std::string> labels;

   labels.resize(slots.emplace(a) + 1);
   labels[slots.key(a)] = "a";
   labels.resize(slots.emplace(b) + 1);
   labels[slots.key(b)] = "b";

   // Swap-and-pop removal that keeps labels dense and in step with the map
   slots.erase_and_swap_values(a, labels);
   auto owner = slots.entity_at(0); // b now occupies dense slot 0

``erase_and_get_swap_index`` performs the removal and returns the index whose value has to be moved
into the hole, for containers that ``erase_and_swap_values`` cannot handle. ``examples/ecs_map_example.cpp``
in the repository is a longer walk-through.

Next Steps
----------

* :doc:`scheduler_tutorial` to process component storages in parallel
* :doc:`containers_tutorial` for the containers underneath the storages
