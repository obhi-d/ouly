OULY Documentation
==================

.. image:: https://img.shields.io/badge/C%2B%2B-20-blue.svg
   :target: https://isocpp.org/std/the-standard
   :alt: C++20

.. image:: https://img.shields.io/badge/License-MIT-yellow.svg
   :target: https://opensource.org/licenses/MIT
   :alt: License: MIT

.. image:: https://github.com/obhi-d/ouly/actions/workflows/ci.yml/badge.svg
   :target: https://github.com/obhi-d/ouly/actions/workflows/ci.yml
   :alt: CI

OULY (Optimized Utility Library) is a C++20 library for programs where allocation patterns, cache
behaviour and thread wake-ups matter: game engines, simulations, tools and services. It provides
task schedulers, memory allocators, cache-friendly containers, an entity component system, and
reflection-driven serialization, behind small STL-like interfaces that can be adopted one header
at a time.

.. toctree::
   :maxdepth: 2
   :caption: Contents:

   getting_started
   tutorials/index
   user_guide/index
   api_reference/index
   examples/index
   performance/index
   contributing
   changelog

Where to start
--------------

* :doc:`getting_started` builds the library, links it from CMake and runs a first program.
* The :doc:`tutorials/index` walk through each module with examples that compile against the
  current headers.
* :doc:`user_guide/index` helps choose between the allocators, containers and scheduler
  generations.
* The :doc:`api_reference/index` is generated from the header comments.

Modules
-------

**Task scheduling** (``ouly/scheduler/``)
   Three work-stealing scheduler generations behind one API, value tasks with continuations,
   structured task scopes, ``parallel_for`` and ``auto_parallel_for``, lazily started coroutines,
   and static and dynamic flow graphs.

**Memory allocators** (``ouly/allocators/``)
   Linear, arena, stack and pool allocators, thread-safe frame allocators, virtual memory,
   memory-mapped files, and coalescing offset suballocators for GPU-style heaps with
   defragmentation.

**Containers** (``ouly/containers/``)
   Small and structure-of-arrays vectors, sparse vectors and tables, block vectors, an intrusive
   list, single-threaded and concurrent queues, and a heterogeneous blackboard.

**Entity component system** (``ouly/ecs/``)
   Registries with optional revision tracking, component storages with selectable layouts,
   collections and sparse-to-dense maps.

**Serialization and reflection** (``ouly/serializers/``, ``ouly/reflection/``)
   Compile-time reflection that drives binary, YAML and user-defined structured formats.

**Utilities and DSL** (``ouly/utility/``, ``ouly/dsl/``)
   Delegates, program arguments, views, intrusive pointers, hashing, string helpers, a macro
   expression evaluator and an event-driven YAML parser.

Indices and tables
==================

* :ref:`genindex`
* :ref:`search`
