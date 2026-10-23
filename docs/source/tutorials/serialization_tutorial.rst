Serialization Tutorial
======================

Serialization in OULY is driven by compile-time reflection (``ouly/reflection/``). A type
describes its fields once, and every format uses that description: the binary streams, YAML, and
any structured format you adapt yourself. Every example on this page compiles against the current
headers and round-trips its data.

Describing types
----------------

**Aggregates** with up to 64 members are reflected automatically; nothing has to be written:

.. code-block:: cpp

   struct material
   {
     std::string                name;
     float                      roughness = 0.5F;
     std::vector<std::string>   textures{};
     std::optional<int>         priority{};
     std::map<std::string, int> flags{};
   };

**Other classes** provide a static ``reflect()`` built from ``ouly::bind<"name", &Class::member>()``
entries:

.. code-block:: cpp

   #include <ouly/reflection/reflection.hpp>

   class camera
   {
   public:
     camera() = default;
     camera(float fov, std::string name) : name_(std::move(name)), fov_(fov) {}

     [[nodiscard]] auto get_fov() const -> float
     {
       return fov_;
     }

     // Field names and member pointers; reflect() may name private members
     static auto reflect() noexcept
     {
       return ouly::bind(ouly::bind<"name", &camera::name_>(), ouly::bind<"fov", &camera::fov_>());
     }

   private:
     std::string name_;
     float       fov_ = 60.0F;
   };

**Properties behind accessors** bind a getter and a setter instead of a member pointer. Member
functions and free functions both work, ``noexcept`` or not; the getter may return by value or by
const reference:

.. code-block:: cpp

   class window;
   auto get_height(window const& w) noexcept -> int;
   void set_height(window& w, int height) noexcept;

   class window
   {
   public:
     [[nodiscard]] auto get_width() const -> int
     {
       return width_;
     }
     void set_width(int width)
     {
       width_ = width;
     }

     static auto reflect() noexcept
     {
       return ouly::bind(ouly::bind<"width", &window::get_width, &window::set_width>(),
                         ouly::bind<"height", &get_height, &set_height>());
     }

   private:
     friend auto get_height(window const& w) noexcept -> int;
     friend void set_height(window& w, int height) noexcept;

     int width_  = 0;
     int height_ = 0;
   };

   auto get_height(window const& w) noexcept -> int
   {
     return w.height_;
   }

   void set_height(window& w, int height) noexcept
   {
     w.height_ = height;
   }

**Types you cannot modify** get a specialization of ``ouly::reflect<T>()`` instead:

.. code-block:: cpp

   // A type you cannot change: specialize ouly::reflect instead
   struct vec3
   {
     float x, y, z;
     vec3() = default;
     vec3(float a, float b, float c) : x(a), y(b), z(c) {}
   };

   template <>
   auto ouly::reflect<vec3>() noexcept
   {
     return ouly::bind(ouly::bind<"x", &vec3::x>(), ouly::bind<"y", &vec3::y>(), ouly::bind<"z", &vec3::z>());
   }

**Value-like types** that should appear as a single value specialize ``ouly::convert<T>`` with a
``to_type`` that returns a string or number and a matching ``from_type``:

.. code-block:: cpp

   // Stored as a string in every format
   struct asset_id
   {
     uint32_t value = 0;
   };

   template <>
   struct ouly::convert<asset_id>
   {
     static auto to_type(asset_id const& id) -> std::string
     {
       return "asset:" + std::to_string(id.value);
     }

     static void from_type(asset_id& id, std::string_view text)
     {
       ouly::from_chars(text.substr(6), id.value);
     }
   };

Standard types work out of the box: integers, floating point, ``bool``, enums (as their underlying
value), strings and string views, sequence containers, maps, ``std::array``, ``std::pair`` and
``std::tuple``, ``std::optional``, ``std::variant``, ``std::unique_ptr`` and ``std::shared_ptr``
(created on read when null), OULY containers such as ``dynamic_array``, ``small_vector`` and ``soavector``, and
nested combinations of all of these.

Binary streams
--------------

``ouly::write(stream, value)`` and ``ouly::read(stream, value)`` (``ouly/serializers/serializers.hpp``)
take any stream with ``write(std::byte const*, size_t)`` or ``read(std::byte*, size_t)`` and
``skip(size_t)``. The byte order is a template argument and defaults to little endian:

.. code-block:: cpp

   #include <ouly/serializers/binary_stream.hpp>
   #include <ouly/serializers/serializers.hpp>

   material in{.name = "rock", .roughness = 0.9F, .textures = {"albedo.png", "normal.png"}};

   ouly::binary_output_stream out; // grows an in-memory buffer
   ouly::write(out, in);           // little endian
   ouly::write<std::endian::big>(out, in);

   ouly::binary_input_stream bin(out.get_string()); // reads from a view of the bytes
   material                  little, big;
   ouly::read(bin, little);
   ouly::read<std::endian::big>(bin, big);

``binary_output_stream`` owns a growing ``std::vector<std::byte>`` (``release()`` hands it over,
``get_string()`` views it) and ``binary_input_stream`` reads from a ``std::span<std::byte const>``.
For files and other ``std::iostream``\ s, use ``binary_ostream`` and ``binary_istream``:

.. code-block:: cpp

   {
     std::ofstream   file("materials.bin", std::ios::binary);
     ouly::binary_ostream stream(file); // wraps a std::ostream
     ouly::write(stream, in);
   }
   std::ifstream       file("materials.bin", std::ios::binary);
   ouly::binary_istream stream(file); // wraps a std::istream
   material            loaded;
   ouly::read(stream, loaded);

Reading past the end of the data throws.

Taking over the encoding
~~~~~~~~~~~~~~~~~~~~~~~~

A type can write its own bytes by providing ``operator<<`` and ``operator>>`` for the stream type.
The serializer uses them in place of reflection, including for elements of containers:

.. code-block:: cpp

   // Full control over the bytes: operators on the stream type itself
   struct packed_color
   {
     uint8_t r = 0, g = 0, b = 0, a = 255;

     friend auto operator<<(ouly::binary_output_stream& out, packed_color const& c) -> ouly::binary_output_stream&
     {
       std::byte const bytes[] = {std::byte{c.r}, std::byte{c.g}, std::byte{c.b}, std::byte{c.a}};
       out.write(bytes, sizeof(bytes));
       return out;
     }

     friend auto operator>>(ouly::binary_input_stream& in, packed_color& c) -> ouly::binary_input_stream&
     {
       std::byte bytes[4];
       in.read(bytes, sizeof(bytes));
       c = {std::to_integer<uint8_t>(bytes[0]), std::to_integer<uint8_t>(bytes[1]), std::to_integer<uint8_t>(bytes[2]),
            std::to_integer<uint8_t>(bytes[3])};
       return in;
     }
   };

.. code-block:: cpp

   std::vector<packed_color>  palette = {{255, 0, 0, 255}, {0, 255, 0, 255}};
   ouly::binary_output_stream colors;
   ouly::write(colors, palette);

   ouly::binary_input_stream  colors_in(colors.get_string());
   std::vector<packed_color> read_back;
   ouly::read(colors_in, read_back);

YAML
----

``ouly::yml::to_string`` and ``ouly::yml::from_string`` (``ouly/serializers/lite_yml.hpp``) write
and parse a YAML subset that covers what the serializer produces: block mappings and sequences,
compact ``[a, b]`` sequences, quoted and plain scalars, and ``|``/``>`` block scalars.

.. code-block:: cpp

   #include <ouly/serializers/lite_yml.hpp>

   scene s;
   s.materials.push_back({.name = "rock", .textures = {"albedo.png"}, .priority = 2});
   s.tag         = std::string("outdoor");
   s.main_camera = std::make_unique<camera>(75.0F, "main");
   s.sky         = asset_id{42};

   std::string yaml = ouly::yml::to_string(s);

   scene loaded;
   ouly::yml::from_string(loaded, yaml);

Fields are written under their reflected names, an empty ``std::optional`` is written as
``null``, and a ``std::variant`` is written as a mapping with its ``type`` index and ``value``. When reading, missing fields keep their current value, so default member
initializers act as defaults.

For polymorphic data, ``ouly::yml::runtime_type_registry`` (``ouly::stream_type_registry``) maps type
ids to registered types for fields of type ``ouly::runtime_type``; see
``unit_tests/yaml_runtime_type.cpp``.

Other structured formats
------------------------

The same ``ouly::read``/``ouly::write`` calls accept any value that models
``ouly::StructuredInputStream`` or ``ouly::StructuredOutputStream`` (``ouly/serializers/serializers.hpp``).
An input stream answers ``is_object()``, ``is_array()``, ``is_null()``, ``size()``, ``at(key)``,
``at(index)``, ``for_each_entry`` and ``as_bool``/``as_int64``/``as_uint64``/``as_double``/``as_string``;
an output stream receives ``begin_object``/``end_object``, ``begin_array``/``end_array``, ``key``,
the ``as_*`` value calls, ``as_null`` and ``next_map_entry``/``next_array_entry``.
``unit_tests/input_serializer.cpp`` and ``unit_tests/output_serializer.cpp`` adapt
`nlohmann::json <https://github.com/nlohmann/json>`_ this way.

Next Steps
----------

* :doc:`utilities_tutorial` for the event-driven YAML parser in ``ouly/dsl/lite_yml.hpp``
* :doc:`containers_tutorial` for containers that serialize directly
