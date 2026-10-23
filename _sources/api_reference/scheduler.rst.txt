Task Scheduler
==============

Headers in ``ouly/scheduler/``. ``ouly::scheduler`` and ``ouly::task_context`` alias the generation selected by ``OULY_SCHEDULER_VERSION`` (``v2`` by default). ``submit_task``, ``when_all`` and ``async`` are overloaded free functions in ``ouly/scheduler/task.hpp`` and ``ouly/scheduler/scheduler.hpp``; the tutorial lists their forms.

See :doc:`../tutorials/scheduler_tutorial` for a guided introduction.

Schedulers
----------

``#include <ouly/scheduler/v1/scheduler.hpp>``

.. doxygenclass:: ouly::v1::scheduler
   :project: OULY
   :members:

``#include <ouly/scheduler/v2/scheduler.hpp>``

.. doxygenclass:: ouly::v2::scheduler
   :project: OULY
   :members:

``#include <ouly/scheduler/v3/scheduler.hpp>``

.. doxygenclass:: ouly::v3::scheduler
   :project: OULY
   :members:

``#include <ouly/scheduler/v2/task_context.hpp>``

.. doxygenclass:: ouly::v2::task_context
   :project: OULY
   :members:

``#include <ouly/scheduler/worker_structs.hpp>``

.. doxygenclass:: ouly::workgroup_id
   :project: OULY
   :members:

``#include <ouly/scheduler/worker_structs.hpp>``

.. doxygenclass:: ouly::worker_id
   :project: OULY
   :members:

Tasks
-----

``#include <ouly/scheduler/task.hpp>``

.. doxygenclass:: ouly::basic_task
   :project: OULY
   :members:

``#include <ouly/scheduler/task.hpp>``

.. doxygenclass:: ouly::basic_task_scope
   :project: OULY
   :members:

``#include <ouly/scheduler/detail/allocation.hpp>``

.. doxygenclass:: ouly::scheduler_allocator
   :project: OULY
   :members:

``#include <ouly/scheduler/co_task.hpp>``

.. doxygenstruct:: ouly::co_task
   :project: OULY
   :members:

``#include <ouly/scheduler/co_task.hpp>``

.. doxygenstruct:: ouly::co_sequence
   :project: OULY
   :members:

Parallel loops
--------------

``#include <ouly/scheduler/parallel_for.hpp>``

.. doxygenfunction:: ouly::parallel_for
   :project: OULY

``#include <ouly/scheduler/auto_parallel_for.hpp>``

.. doxygenfunction:: ouly::auto_parallel_for
   :project: OULY

``#include <ouly/scheduler/task_traits.hpp>``

.. doxygenstruct:: ouly::default_partitioner_traits
   :project: OULY
   :members:

``#include <ouly/scheduler/task_traits.hpp>``

.. doxygenstruct:: ouly::auto_partitioner_traits
   :project: OULY
   :members:

Flow graphs
-----------

``#include <ouly/scheduler/flow_graph.hpp>``

.. doxygenclass:: ouly::flow_graph
   :project: OULY
   :members:

``#include <ouly/scheduler/dynamic_flow_graph.hpp>``

.. doxygenclass:: ouly::dynamic_flow_graph
   :project: OULY
   :members:

``#include <ouly/scheduler/flow_graph_config.hpp>``

.. doxygenstruct:: ouly::cfg::flow_graph_node_id
   :project: OULY
   :members:
