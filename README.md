# Alternate Container Library

[![Build](https://github.com/obhi-d/acl/actions/workflows/test_and_coverage.yml/badge.svg)](https://github.com/obhi-d/acl/actions/workflows/test_and_coverage.yml)
[![Coverage Status](https://coveralls.io/repos/github/obhi-d/acl/badge.svg)](https://coveralls.io/github/obhi-d/acl)

This is a C++ library that provides various containers and algorithms. It requires C++20 and is designed for optimal performance.

## Features

- Multiple allocator implementations:
	- Arena allocator
	- Coalescing allocator
	- Linear allocator
	- Pool allocator
	
- Containers:
	- Dynamic array
	- Small vector
	- SOA vector
	- Sparse vector
	- Table
	- Intrusive list

- Serialization:
	- Binary serializer
	- YAML serializer
	- Input/Output serializers

- Other utilities:
	- Scheduler
	- Microexpr (expression parser)
	- String utilities
	- Program arguments parser

## Building

```bash
cmake -B build
cmake --build build
```

### Testing

Tests can be enabled with:

```bash
cmake -B build -DACL_BUILD_TESTS=ON
cmake --build build
cd build/unit_tests
ctest
```

### Options

- `ACL_BUILD_TESTS` - Build unit tests (OFF by default)
- `ACL_TEST_COVERAGE` - Enable test coverage reporting

## Installation

The library can be installed using CMake:

```bash
cmake --install build
```

# Documentation



## License

This project is licensed under the MIT License.