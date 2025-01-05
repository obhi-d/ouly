Scheduler Documentation
==========================

The ACL scheduler provides a flexible and efficient task scheduling system for concurrent programming. It enables organizing work into logical groups and executing tasks across multiple worker threads.

Core Components
--------------

Task System
~~~~~~~~~~

The task system provides several key abstractions for concurrent programming:

- ``co_task<R>`` - A coroutine task that executes deferred work and can be manually resumed
- ``co_sequence<R>`` - A sequence task that executes immediately and can be awaited
- Task promises that manage coroutine state and execution

Scheduler 
~~~~~~~~~

The scheduler manages task execution across worker threads and workgroups:

- Creates and manages worker threads organized into workgroups
- Supports task submission via coroutines, lambdas, member functions
- Provides work stealing for load balancing
- Enables priority-based scheduling between workgroups

Key Features
-----------

- **Workgroup Organization**: Logically group related tasks together
- **Multiple Task Types**: Support for coroutines, lambdas, member functions
- **Thread Management**: Control thread affinity and work distribution
- **Priority Scheduling**: Configure execution priorities between workgroups
- **Work Stealing**: Automatic load balancing across worker threads

Basic Usage
----------

.. code-block:: cpp

	// Create scheduler and workgroups
	acl::scheduler scheduler;
	scheduler.create_group(acl::workgroup_id(0), 0, 16);  // 16 workers starting at index 0
	
	// Begin execution
	scheduler.begin_execution();

	// Submit coroutine task
	auto task = some_coroutine();
	scheduler.submit(acl::main_worker_id, acl::default_workgroup_id, task);

	// Submit lambda task
	scheduler.submit(worker_id, group_id, [](acl::worker_context const& ctx) {
		// Task work here
	});

	// Wait for completion  
	scheduler.end_execution();

Common Workgroup Patterns
------------------------

- **Default Group**: General purpose work
- **Game Logic Group**: Game simulation tasks  
- **Render Group**: Graphics/rendering work
- **IO Group**: File/network operations
- **Stream Group**: Media streaming tasks

The scheduler provides a foundation for building concurrent applications with controllable execution and good performance characteristics.

.. autodoxygenindex::
   :project: scheduler
