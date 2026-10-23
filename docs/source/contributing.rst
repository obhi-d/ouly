Contributing
============

The full guide is `CONTRIBUTING.md <https://github.com/obhi-d/ouly/blob/main/CONTRIBUTING.md>`_ in
the repository. In short:

Build and test
--------------

.. code-block:: bash

   cmake --preset=linux-default            # Debug, unit tests (macos-default, windows-test)
   cmake --build build/linux-default
   ctest --test-dir build/linux-default --output-on-failure

Without presets, pass ``-DOULY_BUILD_TESTS=ON``. Tests use Catch2 v3 and are registered with
``add_unit_test`` in ``unit_tests/CMakeLists.txt``; on GCC and Clang they always build with
AddressSanitizer.

What CI checks
--------------

* Debug builds and ``ctest`` on MSVC, GCC 16, Clang 18 and Apple Clang. The library builds with
  ``-Wall -Wextra -pedantic -Werror`` and further warnings on GCC and Clang and ``/W4 /WX`` on
  MSVC, so any new warning fails CI.
* A clang-tidy build (``OULY_ENABLE_CLANG_TIDY=ON``, clang-tidy 18, ``WarningsAsErrors: "*"``).
  Reproduce it with the ``linux-clang-tidy`` preset.
* On pushes to ``main``, this documentation is built and published.

Style
-----

* Format with ``.clang-format`` (clang-format 19; ``pre-commit run`` runs it and gersemi for CMake).
* ``lower_case`` names for types, functions, variables and constants; ``CamelCase`` template
  parameters and concepts; ``OULY_`` prefixed ``UPPER_CASE`` macros; data members end in ``_``.
* Public API in ``include/ouly/<module>/``, details in ``detail/`` subdirectories, compiled sources
  in ``src/ouly/`` (listed in ``OULY_SOURCES``).

Documentation
-------------

Document public API with Doxygen comments, and keep the tutorials, the changelog and the README in
step with behaviour changes. Examples must compile against the current headers. To build these
pages:

.. code-block:: bash

   pip install -r docs/requirements.txt
   doxygen Doxyfile
   sphinx-build -b html docs/source docs/build/html
