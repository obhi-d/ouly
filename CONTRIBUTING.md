# Contributing to OULY

Thank you for your interest in contributing to OULY! This guide covers how to build and test the
library, the conventions the code follows, and what CI checks before a change is merged.

## Getting Started

### Prerequisites

- A C++20 compiler. CI builds and tests GCC 16, Clang 18, Apple Clang (macos-latest) and MSVC
  (windows-latest); the weekly Coverity job also builds with GCC 14.
- CMake 3.25 or newer and Ninja (the presets use the Ninja generator)
- clang-format 19 and clang-tidy (clang-tidy 18 is what CI runs)
- Git, and optionally [pre-commit](https://pre-commit.com/)

### Fork, clone and build

```bash
# Fork the repository on GitHub, then clone your fork
git clone https://github.com/your-username/ouly.git
cd ouly
git remote add upstream https://github.com/obhi-d/ouly.git
git checkout -b your-topic

# Debug build with the unit tests
cmake --preset=linux-default            # or macos-default / windows-test
cmake --build build/linux-default
ctest --test-dir build/linux-default --output-on-failure
```

## Building

### Presets

`CMakePresets.json` defines configure and build presets; each writes to `build/<preset>`.

| Preset                                 | Configuration                                                         |
| -------------------------------------- | --------------------------------------------------------------------- |
| `linux-default`                        | Debug, unit tests                                                     |
| `linux-release`                        | RelWithDebInfo, unit tests                                            |
| `linux-clang-tidy`                     | Debug with clang++, unit tests, clang-tidy on every library source    |
| `macos-default` / `macos-release`      | Debug / Release with clang++, unit tests                              |
| `windows-test`, `windows-gen`          | Debug, unit tests                                                     |
| `windows-profiling`, `windows-release` | RelWithDebInfo, unit tests                                            |
| `windows-default`                      | Debug, library only                                                   |

Without a preset, configure the way CI does:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DOULY_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build -C Debug --output-on-failure --parallel
```

### CMake options

| Option                                  | Default | Effect                                                                 |
| --------------------------------------- | ------- | ---------------------------------------------------------------------- |
| `OULY_BUILD_TESTS`                      | OFF     | Build the unit tests (fetches Catch2, nlohmann_json, fast_float, GLM)  |
| `OULY_BUILD_BENCHMARKS`                 | OFF     | Build the `bench_*` executables (fetches nanobench and oneTBB)         |
| `OULY_ENABLE_CLANG_TIDY`                | OFF     | Run clang-tidy on the library sources while compiling                  |
| `OULY_SCHEDULER_PMR_INTERNAL_ALLOCATOR` | OFF     | Scheduler-internal nodes come from a synchronized PMR pool             |
| `OULY_EXPORT_CC_TO_SRC_DIR`             | unset   | Symlink `compile_commands.json` into the source directory              |

`ASAN_ENABLED`, `OULY_REC_STATS` and `OULY_TEST_COVERAGE` are still declared (and `ASAN_ENABLED`
is set by some presets), but they currently have no effect. On GCC and Clang every unit test is
always built with AddressSanitizer and with coverage instrumentation.

### Warnings are errors

The library is compiled with `-Wall -Wextra -pedantic -Werror` plus `-Wshadow`, `-Wconversion`,
`-Wsign-conversion`, `-Wold-style-cast`, `-Wnull-dereference` and further warnings on GCC and
Clang, and with `/W4 /WX` on MSVC. The unit tests use `-Wall -Wextra -pedantic -Werror` (`/W4 /WX`
on MSVC). Every CI build (GCC, Clang and MSVC) therefore fails on any new warning; build with the
compiler you have locally before pushing, and prefer fixing the code over suppressing the warning.
Release builds define `NDEBUG`, so variables used only in assertions need `[[maybe_unused]]`.

## Testing

Tests use [Catch2 v3](https://github.com/catchorg/Catch2) and live in `unit_tests/`, one or more
files per component. Each test executable is registered in `unit_tests/CMakeLists.txt`:

```cmake
add_unit_test(NAME small_vector FILES "small_vector.cpp" SANITIZE)
```

This produces the executable and CTest test `test_small_vector`. Run a subset with CTest or with a
Catch2 tag filter:

```bash
ctest --test-dir build/linux-default -R scheduler --output-on-failure
./build/linux-default/unit_tests/test_small_vector "[capacity]"
```

Guidelines:

- Add or update tests with every behaviour change; a bug fix should come with a test that fails
  without it.
- Tag test cases with the component (`[small_vector]`, `[scheduler]`, ...) and a topic.
- Tests may wrap their body in `// NOLINTBEGIN` / `// NOLINTEND`, as the existing ones do.
- Concurrency code should be exercised under ThreadSanitizer as well. `add_unit_test` supports a
  `TSAN` flag; the TSan targets in `unit_tests/CMakeLists.txt` are commented out by default.

### Benchmarks

Configure with `-DOULY_BUILD_BENCHMARKS=ON` (preferably a Release build) to get
`bench_scheduler_comparison`, `bench_coroutine_comparison`, `bench_performance` and
`bench_offset_allocators` in `build/<preset>/unit_tests/`. Include before/after numbers from the same
machine in pull requests that claim a performance change.

## Code Style

### Formatting

`.clang-format` defines the style. Format the lines you change:

```bash
clang-format -i path/to/changed_file.hpp
# or, with pre-commit installed, run the configured hooks
# (clang-format 19.1.5, and gersemi for CMake files)
pre-commit run --all-files
```

### clang-tidy

`.clang-tidy` enables the `bugprone`, `modernize`, `performance`, `portability`, `readability` and
`cppcoreguidelines` checks with `WarningsAsErrors: "*"`, and checks headers under `include/ouly/`.
CI runs it in the Clang-Tidy job (clang++-18, `-DOULY_ENABLE_CLANG_TIDY=ON`), which compiles the
library sources and the headers they include. Run the same locally with:

```bash
cmake --preset=linux-clang-tidy
cmake --build build/linux-clang-tidy
```

Use a targeted `// NOLINTNEXTLINE(check-name)` only where a check is genuinely wrong for the code.

### Naming

The naming rules are enforced by `readability-identifier-naming`:

- Namespaces, classes, structs, functions, variables and constants (including `constexpr` and
  static constants): `lower_case`
- Template parameters and concepts: `CamelCase`
- Macros: `UPPER_CASE` with an `OULY_` prefix
- Data members, public or private: `lower_case_` with a trailing underscore
- Thread-safe variants of a component carry a `ts_` prefix (`ts_shared_linear_allocator`)

### Layout

```
include/ouly/
  allocators/  containers/  dsl/  ecs/  reflection/  scheduler/  serializers/  utility/
  <module>/detail/       implementation details (namespace ouly::detail)
src/ouly/<module>/        compiled sources; add new files to OULY_SOURCES in CMakeLists.txt
unit_tests/               tests and benchmarks
docs/source/              Sphinx documentation
```

Headers start with `// SPDX-License-Identifier: MIT` and `#pragma once`, put public API in
namespace `ouly` (or a module namespace such as `ouly::ecs`), and mark functions defined in `src/`
with `OULY_API` so shared builds export them. Use `OULY_ASSERT` for internal assertions.

## Documentation

- Document public classes and functions with Doxygen comments (`@brief`, `@param`, `@return`,
  `@note`); the API reference is generated from them.
- Behaviour changes need matching updates to the tutorials in `docs/source/tutorials/`, to
  `docs/source/changelog.rst`, and to `README.md` when they affect its examples.
- Code examples in the documentation must compile against the current headers. Compile them as a
  small translation unit before submitting.

Build the documentation locally:

```bash
python3 -m venv .venv && . .venv/bin/activate
pip install -r docs/requirements.txt
doxygen Doxyfile            # writes docs/xml for Breathe
sphinx-build -b html docs/source docs/build/html
```

`scripts/build_docs.sh` does the same and can serve the result (`--serve --open`).

## Continuous Integration

`.github/workflows/ci.yml` runs on pushes and pull requests to `main` and `develop`:

- **Build and test**: Debug builds with the unit tests on Windows (MSVC), Ubuntu (GCC 16),
  Ubuntu (Clang 18) and macOS (Apple Clang), followed by `ctest`. Warnings are errors on all of them.
- **Clang-Tidy**: the `OULY_ENABLE_CLANG_TIDY=ON` build described above; any finding fails the job.
- **Documentation**: on pushes to `main`, Doxygen and Sphinx build the docs and publish them to
  GitHub Pages.

`.github/workflows/coverity.yml` submits a Release build (GCC 14) to Coverity Scan weekly and on
demand.

## Submitting Changes

1. Rebase on the latest `upstream/main`.
2. Make sure the build is warning-free and `ctest` passes, and that the Clang-Tidy build is clean
   for the files you touched.
3. Keep commits focused. The history uses `component: summary` subjects, for example
   `small_vector: range erase accepts last == end()`, with a body that explains why the change is
   needed.
4. Open a pull request against `main` describing the change, how it was tested, any API or
   behaviour changes, and benchmark results for performance work.

## Getting Help

Use [GitHub Issues](https://github.com/obhi-d/ouly/issues) for bug reports, feature requests and
questions.
