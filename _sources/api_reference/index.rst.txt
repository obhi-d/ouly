API Reference
=============

Complete API documentation generated from source code comments.

.. toctree::
   :maxdepth: 2

   allocators
   containers
   ecs
   scheduler
   serializers
   utility

Overview
--------

This section provides detailed API documentation for all OULY components, automatically generated from the source code using Doxygen and Breathe.

Allocators
----------

.. doxygennamespace:: ouly::allocators
   :project: OULY

Memory allocators and allocation strategies.

.. doxygenclass:: ouly::linear_allocator
   :project: OULY
   :members:

.. doxygenclass:: ouly::linear_arena_allocator
   :project: OULY
   :members:

.. doxygenclass:: ouly::pool_allocator
   :project: OULY
   :members:

Containers
----------

.. doxygennamespace:: ouly::containers
   :project: OULY

High-performance container classes.

.. doxygenclass:: ouly::small_vector
   :project: OULY
   :members:

.. doxygenclass:: ouly::dynamic_array
   :project: OULY
   :members:

.. doxygenclass:: ouly::sparse_vector
   :project: OULY
   :members:

Entity Component System
-----------------------

.. doxygennamespace:: ouly::ecs
   :project: OULY

Entity Component System framework.

.. doxygenclass:: ouly::ecs::registry
   :project: OULY
   :members:

.. doxygenclass:: ouly::ecs::components
   :project: OULY
   :members:

.. doxygenclass:: ouly::ecs::collection
   :project: OULY
   :members:

Task Scheduler
--------------

.. doxygennamespace:: ouly::scheduler
   :project: OULY

Task scheduling and parallel execution.

.. doxygenclass:: ouly::scheduler
   :project: OULY
   :members:

.. doxygenclass:: ouly::task_context
   :project: OULY
   :members:

Serialization
-------------

.. doxygennamespace:: ouly::serializers
   :project: OULY

Binary and YAML serialization.

.. doxygenclass:: ouly::binary_stream
   :project: OULY
   :members:

.. doxygenfunction:: ouly::write
   :project: OULY

.. doxygenfunction:: ouly::read
   :project: OULY

Utilities
---------

.. doxygennamespace:: ouly::utility
   :project: OULY

Utility components and helpers.

Navigation
----------

* Use the search function to find specific classes or functions
* Click on class names to see detailed member documentation
* Source code links are provided for all documented items
* Examples are included where available
