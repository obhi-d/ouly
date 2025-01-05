Entity Component System (ECS)
============================

Introduction
-----------
The ACL Entity Component System (ECS) is a high-performance, template-based implementation designed for efficient game and simulation development. It provides a flexible and type-safe way to manage entities, components, and their relationships.

Key Features
-----------
- Type-safe entity handles with optional revision tracking
- Efficient component storage with multiple storage strategies
- Thread-safe entity creation and management
- Memory-efficient collection system for entity management
- Custom allocator support
- Range-based iteration capabilities

Core Components
-------------

Entity Management
~~~~~~~~~~~~~~~
The system provides two main entity types:

- ``entity<T>`` - Basic entity type without revision tracking
- ``rxentity<T>`` - Entity type with revision tracking for safe handle management

.. code-block:: cpp

	// Create entities
	acl::ecs::entity<> basic_entity;
	acl::ecs::rxentity<> tracked_entity;

Registry
~~~~~~~
The registry manages entity creation, destruction, and lifecycle:

.. code-block:: cpp

	acl::ecs::registry<> registry;
	auto entity = registry.emplace();  // Create new entity
	registry.erase(entity);           // Destroy entity

Component Storage
~~~~~~~~~~~~~~~
Components can be stored with different strategies:

- Direct mapping for fast access
- Sparse storage for memory efficiency 
- Optional self-indexing for reverse lookups

.. code-block:: cpp

	// Create component storage
	acl::ecs::components<Position> positions;
	positions.emplace_at(entity, x, y, z);

Collection System
~~~~~~~~~~~~~~
The collection system provides efficient entity management with:

- Bitset-based storage
- Optional revision tracking
- Pool-based memory allocation
- Custom allocator support

.. code-block:: cpp

	acl::ecs::collection<Entity> entities;
	entities.emplace(entity);    // Add entity
	entities.contains(entity);   // Check if entity exists

Advanced Features
---------------

Custom Allocators
~~~~~~~~~~~~~~~
The system supports custom allocators for fine-grained memory control:

.. code-block:: cpp

	using CustomOptions = acl::options<
		 acl::opt::custom_allocator<MyAllocator>
	>;
	acl::ecs::components<Position, Entity, CustomOptions> positions;

Storage Strategies
~~~~~~~~~~~~~~~
Components can be configured with different storage strategies:

.. code-block:: cpp

	using SparseStorage = acl::options<
		 acl::opt::use_sparse,
		 acl::opt::pool_size<1024>
	>;
	acl::ecs::components<Position, Entity, SparseStorage> positions;

Performance Considerations
-----------------------
- Entity creation/destruction is O(1)
- Component access is O(1) with direct mapping
- Iteration performance depends on storage strategy
- Memory overhead is minimized through configurable storage options
- Revision tracking adds minimal overhead when enabled

Threading Support
---------------
- Entity creation is thread-safe
- Component operations should be externally synchronized
- Collections provide thread-safe read operations


.. autodoxygenindex::
   :project: ecs
