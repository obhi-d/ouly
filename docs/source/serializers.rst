Serialization
========================

The ACL library provides a flexible serialization system that supports multiple formats including YAML and binary. The serialization system is based on concepts that allow implementing custom serializers.

Core Components
========================

Input Serializer
~~~~~~~~~~~~~~~~~~~~~~~~~~~~
The ``input_serializer`` class template provides deserialization capabilities for converting data from a format into C++ objects. It uses concepts to define requirements for input serializer implementations.

Key features:

- Supports reading primitive types (integers, floats, booleans)
- Handles containers like arrays and maps  
- Supports optional values and variants
- Extensible for custom types via concepts

Output Serializer 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
The ``output_serializer`` class template handles serializing C++ objects into a format. Like the input serializer, it defines concepts for implementing custom serializers.

Key features:

- Serializes primitive types
- Handles STL containers
- Supports optional values and variants
- Extensible via concepts

Format Implementations
--------------------------------------

YAML Serialization
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
The YAML serializers provide human-readable serialization:

- ``yaml::to_string()`` - Serializes objects to YAML format
- ``yaml::from_string()`` - Deserializes YAML into objects

Example:

.. code-block:: cpp

	struct Person {
		std::string name;
		int age;
	};

	Person person{"John", 30};
	
	// Serialize to YAML
	std::string yaml = acl::yaml::to_string(person);

	// Deserialize from YAML
	Person loaded;
	acl::yaml::from_string(loaded, yaml);

Binary Serialization
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
The binary serializers provide compact binary serialization:

- ``binary_output_serializer`` - Serializes to binary
- ``binary_input_serializer`` - Deserializes from binary

Features:

- Compact binary format
- Configurable endianness
- Fast serialization of POD types

Creating Custom Serializers
------------------------------------------------
New serializers can be created by implementing the required concepts:

- ``InputSerializer`` concept for deserialization
- ``OutputSerializer`` concept for serialization
- ``BinaryInputStream`` concept for binary input
- ``BinaryOutputStream`` concept for binary output

This allows extending the serialization system to support new formats while reusing the core serialization logic.


.. autodoxygenindex::
   :project: serializers
