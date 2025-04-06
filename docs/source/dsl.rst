Domain Specific Languages
========================

The OULY library provides powerful tools for handling domain-specific languages (DSL) and configuration.

Micro Expression Evaluator
-------------------------

The ``microexpr`` class provides a lightweight boolean expression evaluator specifically designed for 
macro expressions. It supports standard boolean operations and macro substitution.

Key features:

* Evaluates boolean macro expressions
* Supports standard logical operators (&&, ||, !)
* Configurable macro context for variable lookup
* Compact and efficient implementation

Basic usage::

	auto ctx = [](std::string_view macro) -> std::optional<int> {
		// Return macro definition or std::nullopt if undefined
	};
	ouly::microexpr evaluator(std::move(ctx));
	bool result = evaluator.evaluate("$(MACRO_A) && !$(MACRO_B)");

YAML Parser
----------

The YAML component provides a streaming parser for YAML documents with a context-based event interface.
The parser is designed for efficiency with minimal memory allocation using small vector optimizations.

Features:

* Streaming event-based parser
* Support for **only** basic YAML structures (mappings, sequences, scalars)
* Block scalar handling (literal | and folded >)
* Compact array support
* Efficient string handling using string views and slices

The parser uses a context interface that can be implemented to handle YAML events::

	class MyContext : public ouly::yaml::context {
		void begin_array() override { /* ... */ }
		void end_array() override { /* ... */ }
		void begin_key(std::string_view key) override { /* ... */ }
		void end_key() override { /* ... */ }
		void set_value(std::string_view value) override { /* ... */ }
	};

.. autodoxygenindex::
   :project: dsl
