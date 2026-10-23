Serialization and Reflection
============================

Headers in ``ouly/serializers/`` and ``ouly/reflection/``. ``ouly::read`` and ``ouly::write`` (``ouly/serializers/serializers.hpp``) are overloaded for binary and structured streams; ``ouly::bind`` (``ouly/reflection/bind.hpp``) builds reflection declarations.

See :doc:`../tutorials/serialization_tutorial` for a guided introduction.

Streams
-------

``#include <ouly/serializers/binary_stream.hpp>``

.. doxygenclass:: ouly::binary_output_stream
   :project: OULY
   :members:

``#include <ouly/serializers/binary_stream.hpp>``

.. doxygenclass:: ouly::binary_input_stream
   :project: OULY
   :members:

``#include <ouly/serializers/binary_stream.hpp>``

.. doxygenclass:: ouly::binary_ostream
   :project: OULY
   :members:

``#include <ouly/serializers/binary_stream.hpp>``

.. doxygenclass:: ouly::binary_istream
   :project: OULY
   :members:

YAML
----

``#include <ouly/serializers/lite_yml.hpp>``

.. doxygenfunction:: ouly::yml::to_string(Class const &obj)
   :project: OULY

``#include <ouly/serializers/lite_yml.hpp>``

.. doxygenfunction:: ouly::yml::from_string(Class &obj, std::string_view data)
   :project: OULY

``#include <ouly/serializers/lite_yml.hpp>``

.. doxygenclass:: ouly::stream_type_registry
   :project: OULY
   :members:

Customization
-------------

``#include <ouly/utility/convert.hpp>``

.. doxygenstruct:: ouly::convert
   :project: OULY
   :members:
