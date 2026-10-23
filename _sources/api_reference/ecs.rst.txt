Entity Component System
=======================

Headers in ``ouly/ecs/``. ``registry<>``/``rxregistry<>`` and ``entity<>``/``rxentity<>`` are aliases of ``basic_registry`` and ``basic_entity`` without and with revision bits.

See :doc:`../tutorials/ecs_tutorial` for a guided introduction.

Types
-----

``#include <ouly/ecs/entity.hpp>``

.. doxygenclass:: ouly::ecs::basic_entity
   :project: OULY
   :members: basic_entity, get, value, revision, set_revision, bump_revision, revised, is_null, null, size_type, revision_type, nb_revision_bits

``#include <ouly/ecs/registry.hpp>``

.. doxygenclass:: ouly::ecs::basic_registry
   :project: OULY
   :members:

``#include <ouly/ecs/components.hpp>``

.. doxygenclass:: ouly::ecs::components
   :project: OULY
   :members:

``#include <ouly/ecs/collection.hpp>``

.. doxygenclass:: ouly::ecs::collection
   :project: OULY
   :members:

``#include <ouly/ecs/map.hpp>``

.. doxygenclass:: ouly::ecs::map
   :project: OULY
   :members:
