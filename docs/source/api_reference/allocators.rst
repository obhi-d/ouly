Memory Allocators
=================

Headers in ``ouly/allocators/``. ``ouly::allocate``, ``ouly::zallocate`` and ``ouly::deallocate`` in ``ouly/allocators/allocator.hpp`` are typed helpers over any allocator.

See :doc:`../tutorials/memory_management` for a guided introduction.

Linear and pool allocators
--------------------------

``#include <ouly/allocators/linear_allocator.hpp>``

.. doxygenclass:: ouly::linear_allocator
   :project: OULY
   :members:

``#include <ouly/allocators/linear_arena_allocator.hpp>``

.. doxygenclass:: ouly::linear_arena_allocator
   :project: OULY
   :members:

``#include <ouly/allocators/linear_stack_allocator.hpp>``

.. doxygenclass:: ouly::linear_stack_allocator
   :project: OULY
   :members:

``#include <ouly/allocators/pool_allocator.hpp>``

.. doxygenclass:: ouly::pool_allocator
   :project: OULY
   :members:

``#include <ouly/allocators/object_pool.hpp>``

.. doxygenclass:: ouly::object_pool
   :project: OULY
   :members:

``#include <ouly/allocators/default_allocator.hpp>``

.. doxygenstruct:: ouly::default_allocator
   :project: OULY
   :members:

Thread-safe allocators
----------------------

``#include <ouly/allocators/ts_shared_linear_allocator.hpp>``

.. doxygenclass:: ouly::ts_shared_linear_allocator
   :project: OULY
   :members:

``#include <ouly/allocators/ts_thread_local_allocator.hpp>``

.. doxygenclass:: ouly::ts_thread_local_allocator
   :project: OULY
   :members:

Coalescing allocators
---------------------

``#include <ouly/allocators/coalescing_allocator.hpp>``

.. doxygenclass:: ouly::coalescing_allocator
   :project: OULY
   :members:

``#include <ouly/allocators/coalescing_arena_allocator.hpp>``

.. doxygenclass:: ouly::coalescing_arena_allocator
   :project: OULY
   :members:

``#include <ouly/allocators/coalescing_arena_allocator.hpp>``

.. doxygenstruct:: ouly::ca_allocation_options
   :project: OULY
   :members:

``#include <ouly/allocators/coalescing_arena_allocator.hpp>``

.. doxygenstruct:: ouly::ca_allocation
   :project: OULY
   :members:

``#include <ouly/allocators/coalescing_arena_allocator.hpp>``

.. doxygenstruct:: ouly::ca_relocation
   :project: OULY
   :members:

``#include <ouly/allocators/coalescing_arena_allocator.hpp>``

.. doxygenstruct:: ouly::ca_defrag_budget
   :project: OULY
   :members:

``#include <ouly/allocators/coalescing_arena_allocator.hpp>``

.. doxygenstruct:: ouly::ca_defrag_result
   :project: OULY
   :members:

``#include <ouly/allocators/coalescing_arena_allocator.hpp>``

.. doxygenstruct:: ouly::ca_defrag_statistics
   :project: OULY
   :members:

``#include <ouly/allocators/allocation_id.hpp>``

.. doxygenstruct:: ouly::allocation_id
   :project: OULY
   :members:

``#include <ouly/allocators/allocation_id.hpp>``

.. doxygenstruct:: ouly::arena_id
   :project: OULY
   :members:

Virtual memory and files
------------------------

``#include <ouly/allocators/virtual_allocator.hpp>``

.. doxygenclass:: ouly::virtual_allocator
   :project: OULY
   :members:

``#include <ouly/allocators/mmap_file.hpp>``

.. doxygenclass:: ouly::basic_mmap_file
   :project: OULY
   :members:

Adaptors
--------

``#include <ouly/allocators/std_allocator_wrapper.hpp>``

.. doxygenstruct:: ouly::allocator_ref
   :project: OULY
   :members:

``#include <ouly/allocators/std_allocator_wrapper.hpp>``

.. doxygenstruct:: ouly::allocator_wrapper
   :project: OULY
   :members:

``#include <ouly/allocators/alignment.hpp>``

.. doxygenstruct:: ouly::alignment
   :project: OULY
   :members:
