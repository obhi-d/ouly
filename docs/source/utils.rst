Utility Classes
==============

This section describes the utility classes provided by OULY (Abstract Core Library).

Integer Range
------------

``integer_range<I>`` provides a simple range abstraction for integers:

.. code-block:: cpp

	template <typename I>
	class integer_range;

Key features:

- Represents a range between two integer values
- Provides standard container interface (begin/end)
- Supports size() and empty() operations

Program Arguments
----------------

``program_args`` provides command-line argument parsing functionality:

Key features:

- Parse C-style main() arguments
- Support for flags and named arguments
- Type-safe argument retrieval
- Built-in help generation
- Documentation support

Projected View
-------------

``projected_view<M, C>`` provides a view that projects a member from a sequence of objects:

.. code-block:: cpp

	template <auto M, typename C = typename ouly::opt::member<M>::class_type>
	class projected_view;

Key features:

- Projects a specific member from a sequence of objects
- Standard random access iterator support
- Const and non-const views

Reflection
----------

The reflection utilities provide compile-time introspection capabilities:

Key features:

- Class member reflection
- Serialization support
- Type traits and concepts for reflection
- Custom binding support
- Field iteration capabilities

String Utilities
--------------

Comprehensive string manipulation utilities including:

- Case conversion (to_lower, to_upper)
- String trimming (trim, trim_leading, trim_trailing)
- String splitting and tokenization
- Word wrapping
- Unicode support
- Time string formatting

Tagged Types
-----------

Tagged Integer
~~~~~~~~~~~~~

``tagged_int<Tag, Int, Null>`` provides type-safe integer wrappers:

.. code-block:: cpp

	template <typename Tag, typename Int = int, Int Null = Int()>
	class tagged_int;

Tagged Pointer
~~~~~~~~~~~~~

``tagged_ptr<T>`` combines a pointer with a small tag value:

- Memory efficient pointer+tag combination
- Smart pointer interface
- Comparison operations
- Tag manipulation methods

Type Name
--------

Compile-time type name utilities:

- Platform-independent type name retrieval
- Type hash generation
- Static assertion helpers

Word List
--------

``word_list<TChar, Delimiter>`` provides string tokenization:

Key features:

- Iterate over delimited strings
- Custom delimiter support
- String view based implementation
- Efficient memory usage

Zip View
-------

``zip_view`` allows iterating over multiple containers simultaneously:

.. code-block:: cpp

	template <typename... T>
	class zip_view;

Key features:

- Iterate multiple containers in lockstep
- Works with any container supporting begin()/end()
- Tuple-based value access


.. autodoxygenindex::
   :project: utils
