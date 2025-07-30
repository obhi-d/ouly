Changelog
=========

All notable changes to OULY will be documented in this file.

The format is based on `Keep a Changelog <https://keepachangelog.com/en/1.0.0/>`_,
and this project adheres to `Semantic Versioning <https://semver.org/spec/v2.0.0.html>`_.

[Unreleased]
------------

Added
~~~~~
- Comprehensive documentation with Sphinx and Breathe integration
- Coalescing arena allocator for reduced fragmentation

Changed
~~~~~~~
- Improved error handling in binary serialization
- Enhanced type safety in ECS component storage
- Optimized memory layout for better cache performance

Fixed
~~~~~
- Thread safety issues in shared linear allocator
- Memory alignment bugs in pool allocator
- Compilation errors with certain compiler configurations

[1.0.0] - 2025-01-01
--------------------

Added
~~~~~

**Core Features**
- Linear allocator with LIFO deallocation support
- Arena allocator with automatic block management
- Pool allocator for fixed-size object allocation
- Thread-safe allocator variants with lock-free design

**Container Library**
- ``small_vector<T, N>`` with stack-based storage optimization
- ``dynamic_array<T>`` with custom allocator support
- ``sparse_vector<T>`` for memory-efficient sparse storage
- ``intrusive_list<T>`` for zero-allocation linked lists
- ``soavector<Types...>`` for Structure of Arrays layout

**Entity Component System**
- ``registry<>`` for entity lifecycle management
- ``components<T>`` with configurable storage strategies
- ``collection<Entity>`` for efficient entity grouping
- ``rxregistry<>`` with revision-based thread safety

**Task Scheduler**
- Work-stealing scheduler with multiple worker threads
- Workgroup organization for task dependencies
- C++20 coroutine support with ``co_task<T>``
- Manual thread affinity configuration support

**Serialization Framework**
- Binary serialization with endianness control
- YAML serialization for human-readable configuration
- Stream-based I/O for memory efficiency
- Custom type serialization support

**Utility Components**
- Memory alignment utilities and helpers
- Template-based configuration system
- STL allocator wrapper for compatibility
- Debugging and statistics collection

**Build System**
- CMake integration with modern targets
- Multiple build presets for different platforms
- Comprehensive unit test suite with Catch2

**Documentation**
- Getting started guide and tutorials
- Complete API reference with Doxygen
- Performance optimization guide
- Real-world usage examples

Platform Support
~~~~~~~~~~~~~~~~~
- Linux (GCC 10+, Clang 12+)
- macOS (Apple Clang 13+, Intel/ARM)
- Windows (MSVC 2019+)
- C++20 standard compliance

Performance Characteristics
~~~~~~~~~~~~~~~~~~~~~~~~~~~
- Zero-cost abstractions through template specialization
- Lock-free algorithms for multi-threaded performance
- Cache-friendly memory layouts and access patterns
- SIMD optimization opportunities with SoA containers

Changed
~~~~~~~
- Initial stable release
- API design finalized for v1.x compatibility

Security
~~~~~~~~
- Memory safety through RAII and automatic lifetime management
- Bounds checking in debug builds
- Thread safety guarantees documented for all components

[0.9.0] - 2024-12-01
--------------------

Added
~~~~~
- Beta release with core allocator functionality
- Basic container implementations
- Initial ECS framework
- Preliminary task scheduler

[0.5.0] - 2024-09-01  
--------------------

Added
~~~~~
- Alpha release with experimental allocators
- Proof-of-concept implementations

[0.1.0] - 2024-06-01
--------------------

Added
~~~~~
- Project initialization
- Basic project structure
- Initial CMake configuration
- Foundational header-only library design

Migration Guide
---------------

Upgrading from 0.x to 1.0
~~~~~~~~~~~~~~~~~~~~~~~~~~

**Breaking Changes**
- Allocator interface standardization
- ECS component storage configuration changes
- Scheduler API refinements

**Code Migration**

Old (0.x):

.. code-block:: cpp

   // Old allocator interface
   auto allocator = ouly::make_linear_allocator(1024);
   void* ptr = allocator->allocate(256);

New (1.0):

.. code-block:: cpp

   // New standardized interface
   ouly::linear_allocator<> allocator(1024);
   void* ptr = allocator.allocate(256);

**ECS Configuration**

Old (0.x):

.. code-block:: cpp

   // Old configuration
   ouly::ecs::components<Position, ouly::ecs::sparse_storage> positions;

New (1.0):

.. code-block:: cpp

   // New configuration system
   using SparseConfig = ouly::cfg::use_sparse<>;
   ouly::ecs::components<Position, ouly::ecs::entity<>, SparseConfig> positions;

Future Roadmap
--------------

**Planned for 1.1.0**
- Additional allocator strategies (buddy, slab)
- Enhanced SIMD optimization support
- Improved debugging and profiling tools
- Additional serialization formats (JSON, MessagePack)

**Planned for 1.2.0**
- GPU memory allocator support
- Distributed computing extensions
- Advanced ECS queries and filters
- Real-time garbage collection options

**Planned for 2.0.0**
- Modern C++ features adoption (modules, concepts)
- Breaking API improvements based on user feedback
- Platform-specific optimizations
- Enterprise features and support

Contributing
------------

We welcome contributions! Please see our `Contributing Guide <contributing.html>`_ for details on:

- Code style and standards
- Pull request process
- Testing requirements
- Documentation guidelines

Support
-------

For support and questions:

- GitHub Issues: Bug reports and feature requests
- GitHub Discussions: General questions and community support
- Documentation: Comprehensive guides and API reference

License
-------

OULY is released under the MIT License. See LICENSE file for details.
