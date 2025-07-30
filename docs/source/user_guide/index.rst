User Guide
==========

Complete reference for using OULY's components in real-world applications.

.. toctree::
   :maxdepth: 2

   memory_allocators
   container_library
   ecs_framework
   task_scheduler
   serialization_system
   utility_components
   integration_guide
   best_practices

Overview
--------

This user guide provides comprehensive documentation for each OULY component, including:

* **Detailed API documentation** - Complete interface descriptions
* **Architecture explanations** - How components are designed and why
* **Configuration options** - Available settings and their effects
* **Integration patterns** - How components work together
* **Troubleshooting** - Common issues and solutions

Component Organization
----------------------

OULY is organized into several core modules:

**Memory Management**
   Custom allocators and memory management strategies for high-performance applications.

**Container Library**
   STL-compatible containers optimized for specific use cases and performance characteristics.

**Entity Component System**
   Data-oriented ECS framework for game development and simulation applications.

**Task Scheduler**
   Work-stealing scheduler with coroutine support for parallel execution.

**Serialization**
   Binary and YAML serialization frameworks for data persistence and network protocols.

**Utilities**
   Helper components including reflection, DSL utilities, and meta-programming tools.

Navigation Tips
---------------

* Use the **Search** function to quickly find specific APIs or concepts
* Cross-references link related topics across different modules
* Code examples are provided for all major features
* Performance notes highlight optimization opportunities

Getting Support
---------------

If you need help using OULY:

* Check the :doc:`../api_reference/index` for detailed API documentation
* Review :doc:`../examples/index` for complete working examples
* Visit the `GitHub repository <https://github.com/obhi-d/ouly>`_ for issues and discussions
* Read :doc:`best_practices` for recommended usage patterns
