Tutorials
=========

One tutorial per module. Each one explains the concepts, shows the API through examples that
compile against the current headers, and points out the behaviour that is easy to get wrong.

.. toctree::
   :maxdepth: 2

   scheduler_tutorial
   memory_management
   containers_tutorial
   ecs_tutorial
   serialization_tutorial
   utilities_tutorial

Overview
--------

1. :doc:`scheduler_tutorial`: workgroups and task contexts, the v1/v2/v3 scheduler generations,
   value tasks, task scopes, parallel loops, coroutines and flow graphs.
2. :doc:`memory_management`: linear, stack and pool allocators, thread-safe frame allocators,
   memory-mapped files, and the coalescing offset allocators with defragmentation.
3. :doc:`containers_tutorial`: ``small_vector``, ``soavector``, sparse storage, intrusive lists,
   queues and the blackboard.
4. :doc:`ecs_tutorial`: registries, component storages, collections and sparse-to-dense maps.
5. :doc:`serialization_tutorial`: reflection, binary streams, YAML and custom formats.
6. :doc:`utilities_tutorial`: program arguments, delegates, macro expressions, the YAML event
   parser, views, pointers and hashing.

The unit tests in ``unit_tests/`` are the most complete source of further examples; see
:doc:`../examples/index`.
