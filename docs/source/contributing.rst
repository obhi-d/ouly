Contributing to OULY
===================

Guidelines for contributing to the OULY project.

Development Environment
-----------------------

Setting up your development environment:

.. code-block:: bash

   # Clone the repository
   git clone https://github.com/obhi-d/ouly.git
   cd ouly

   # Install dependencies (Ubuntu/Debian)
   sudo apt-get update
   sudo apt-get install build-essential cmake ninja-build doxygen

   # Install dependencies (macOS)
   brew install cmake ninja doxygen

   # Configure development build
   cmake --preset=linux-default  # or macos-default
   
   # Build and test
   cmake --build build/linux-default
   cd build/linux-default && ctest

Code Style and Standards
------------------------

OULY follows specific coding standards:

**Naming Conventions**
   * Types: ``snake_case`` (e.g., ``linear_allocator``)
   * Functions: ``snake_case`` (e.g., ``allocate``, ``get_size``)
   * Variables: ``snake_case`` (e.g., ``memory_pool``, ``block_size``)
   * Constants: ``UPPER_CASE`` (e.g., ``DEFAULT_ALIGNMENT``)
   * Template parameters: ``CamelCase`` (e.g., ``AllocatorType``)

**File Organization**
   * Headers: ``include/ouly/module/file.hpp``
   * Implementation: ``src/module/file.cpp`` (if needed)
   * Tests: ``unit_tests/module/test_file.cpp``

**Code Formatting**
   Use the provided ``.clang-format`` configuration:

   .. code-block:: bash

      # Format all code
      find include src unit_tests -name "*.hpp" -o -name "*.cpp" | xargs clang-format -i

**Documentation Standards**
   All public APIs must be documented with Doxygen comments:

   .. code-block:: cpp

      /**
       * @brief Allocates memory with specified alignment.
       * 
       * @param size Number of bytes to allocate
       * @param alignment Memory alignment requirement (must be power of 2)
       * @return Pointer to allocated memory, or nullptr on failure
       * 
       * @pre size > 0
       * @pre alignment is a power of 2
       * @post Returned pointer is aligned to the specified boundary
       */
      void* allocate(size_t size, alignment<> alignment = alignment<>{});

Contribution Types
------------------

We welcome several types of contributions:

**Bug Fixes**
   * Identify and fix bugs in existing code
   * Add regression tests for fixed issues
   * Update documentation if behavior changes

**New Features**
   * Implement new allocators, containers, or utilities
   * Add platform-specific optimizations
   * Enhance existing APIs with new functionality

**Performance Improvements**
   * Optimize existing algorithms and data structures
   * Add SIMD optimizations for supported platforms
   * Improve memory layout and cache efficiency

**Documentation**
   * Improve API documentation and examples
   * Add tutorials for new features
   * Fix typos and clarify explanations

**Testing**
   * Add unit tests for untested code paths
   * Create integration tests for complex scenarios

Pull Request Process
--------------------

1. **Create an Issue**
   
   Before starting work, create an issue describing the problem or enhancement.

2. **Fork and Branch**
   
   .. code-block:: bash

      # Fork the repository on GitHub, then:
      git clone https://github.com/YOUR_USERNAME/ouly.git
      cd ouly
      git checkout -b feature/your-feature-name

3. **Implement Changes**
   
   * Follow the coding standards
   * Add appropriate tests
   * Update documentation
   * Ensure all tests pass

4. **Test Your Changes**
   
   .. code-block:: bash

      # Run all tests
      cmake --build build && cd build && ctest
      
      # Run specific test categories
      ctest -R allocator  # Run allocator tests only

5. **Update Documentation**
   
   If you add new features or change APIs:

   .. code-block:: bash

      # Build documentation
      cd docs
      make html
      
      # Check for warnings
      make html 2>&1 | grep -i warning

6. **Commit Changes**
   
   Use clear, descriptive commit messages:

   .. code-block:: bash

      git add .
      git commit -m "feat(allocators): add coalescing arena allocator
      
      - Implements block coalescing for reduced fragmentation
      - Adds configurable coalescing strategies  
      - Includes comprehensive unit tests
      
      Fixes #123"

7. **Submit Pull Request**
   
   * Push your branch and create a pull request
   * Fill out the pull request template completely
   * Link to related issues
   * Include before/after performance data if applicable

Code Review Guidelines
----------------------

**For Contributors**
   * Be responsive to feedback
   * Keep pull requests focused and reasonably sized
   * Update your branch if conflicts arise
   * Add tests for edge cases identified during review

**For Reviewers**
   * Be constructive and specific in feedback
   * Test the changes locally when possible
   * Check for performance implications
   * Verify documentation is complete and accurate

Testing Requirements
--------------------

All contributions must include appropriate tests:

**Unit Tests**
   
   .. code-block:: cpp

      #include <catch2/catch_test_macros.hpp>
      #include <ouly/allocators/linear_allocator.hpp>

      TEST_CASE("linear_allocator basic allocation", "[allocators]") {
          ouly::linear_allocator<> allocator(1024);
          
          SECTION("successful allocation") {
              void* ptr = allocator.allocate(256);
              REQUIRE(ptr != nullptr);
              REQUIRE(allocator.used() == 256);
          }
          
          SECTION("alignment requirements") {
              void* ptr = allocator.allocate(100, ouly::alignment<16>{});
              REQUIRE(reinterpret_cast<uintptr_t>(ptr) % 16 == 0);
          }
      }

Documentation Standards
-----------------------

**API Documentation**
   Use Doxygen with complete parameter and return documentation.

**Code Examples**
   Include practical, compilable examples in documentation.

**Architecture Documentation**
   Explain design decisions and trade-offs for complex components.

**Performance Notes**
   Document performance characteristics and optimization tips.

Release Process
---------------

OULY follows semantic versioning (MAJOR.MINOR.PATCH):

* **MAJOR**: Incompatible API changes
* **MINOR**: New functionality, backward compatible
* **PATCH**: Bug fixes, backward compatible

**Release Checklist**
1. Update version numbers
2. Update CHANGELOG.md
3. Run full test suite on all supported platforms
4. Update documentation
5. Create release tag
6. Publish release notes

Community Guidelines
--------------------

**Be Respectful**
   Treat all contributors with respect and professionalism.

**Be Collaborative**
   Work together to find the best solutions.

**Be Patient**
   Code review and discussion take time.

**Ask Questions**
   If you're unsure about anything, ask for clarification.

**Learn and Teach**
   Share knowledge and learn from others.

Getting Help
------------

If you need help contributing:

* Check existing issues and discussions
* Ask questions in GitHub Discussions
* Join the community chat (if available)
* Reach out to maintainers for guidance

Recognition
-----------

Contributors are recognized in:

* CONTRIBUTORS.md file
* Release notes for significant contributions  
* Annual contributor highlights

Thank you for contributing to OULY! Your efforts help make it a better library for everyone.
