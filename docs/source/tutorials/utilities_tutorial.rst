Utilities and DSL Tutorial
==========================

``ouly/utility/`` collects the small building blocks the rest of the library is made of, and
``ouly/dsl/`` holds two small parsers. Every example on this page compiles against the current
headers.

Command-line arguments
----------------------

``ouly::program_args`` (``ouly/utility/program_args.hpp``) parses ``--name=value``, ``--name value``,
``-n=value`` and bare ``--flag`` options. Values are converted on access, either by declaring an
argument and reading it, or by *sinking* it into a variable that holds the default:

.. code-block:: cpp

   #include <ouly/utility/program_args.hpp>

   ouly::program_args<> args;
   args.parse_args(argc, argv); // argv[0] is the program name

   // Declare-and-read
   auto threads = args.decl<int>("threads", "t").doc("worker thread count").value(); // std::optional<int>

   // Or sink into variables that hold the defaults
   std::string_view config = "default.yml";
   bool             verbose = false;
   args.sink(config, "config", "c", "configuration file");
   args.sink(verbose, "verbose", "v", "print more");

   auto inputs = args.get_positional_args(); // arguments that are not --name / -n options

Lists are written as ``-n=[1,2,3]`` and sink into containers. ``doc(...)`` and ``brief(...)``
attach help text, ``must_print_help()`` reports ``--help``, and ``doc(formatter)`` hands the
collected documentation to a callable for printing.

Delegates
---------

``ouly::delegate<R(Args...)>`` stores a callable inline without allocating. The callable must fit in
the inline buffer (``basic_delegate<Size, R(Args...)>`` chooses its size) and be trivially copyable
and destructible; both are checked at compile time. The scheduler's ``task_delegate`` is one.

.. code-block:: cpp

   #include <ouly/utility/delegate.hpp>

   using callback = ouly::delegate<int(int, int)>;

   auto from_function = callback::bind(&free_add);
   auto from_lambda   = callback::bind([](int a, int b) { return a * b; });
   int  sum           = from_function(2, 3); // 5
   int  product       = from_lambda(2, 3);   // 6

   // Member functions: capture the object (captures must be trivially copyable and fit inline)
   renderer r{.frame = 10};
   auto     member = ouly::delegate<int(int)>::bind([&r](int count) { return r.draw(count); });
   int      drawn  = member(5); // 15

Macro expressions
-----------------

``ouly::microexpr`` (``ouly/dsl/microexpr.hpp``) evaluates preprocessor-style conditions, for
example to filter shader permutations or configuration blocks. A callback reports each macro's
value, or ``std::nullopt`` for an undefined macro:

.. code-block:: cpp

   #include <ouly/dsl/microexpr.hpp>

   std::unordered_map<std::string_view, int> defines = {
    {"PLATFORM_LINUX", 1},
    {"MAX_LIGHTS", 16},
    {"USE_RAYTRACING", 0},
   };

   ouly::microexpr expr(
    [&defines](std::string_view name) -> std::optional<int>
    {
      if (auto it = defines.find(name); it != defines.end())
      {
        return it->second;
      }
      return std::nullopt; // not defined
    });

   bool a = expr.evaluate("$(PLATFORM_LINUX) && !$(PLATFORM_WINDOWS)"); // true: defined / not defined
   bool b = expr.evaluate("$USE_RAYTRACING");                            // true: defined, even though it is 0
   bool c = expr.evaluate("USE_RAYTRACING");                             // false: its value is 0
   bool d = expr.evaluate("MAX_LIGHTS >= 8 && MAX_LIGHTS % 4 == 0");     // true
   bool e = expr.evaluate("(MAX_LIGHTS | 1) == 17 ? 1 : 0");             // true

- ``$NAME`` and ``$(NAME)`` test whether a macro is defined (1 or 0); a bare ``NAME`` yields its
  value, 0 when undefined. Names may contain letters, digits and ``_``.
- Operators: ``!``, ``~``, unary ``-``, ``*``, ``/``, ``%``, ``+``, ``-``, ``<``, ``<=``,
  ``>``, ``>=``, ``==``, ``!=``, ``&``, ``^``, ``|``, ``&&``, ``||``, the conditional ``?:`` and
  parentheses. Decimal and ``0x`` hexadecimal literals are accepted.
- ``evaluate`` returns whether the result is non-zero. Malformed input, such as an unclosed
  parenthesis, evaluates to ``false``.

Event-driven YAML parsing
-------------------------

``ouly::yml::lite_stream`` (``ouly/dsl/lite_yml.hpp``) is the parser underneath the YAML
serializer. It reports the structure of a document to a ``ouly::yml::context`` you implement, which
is useful for reading YAML into your own data structures without the reflection layer:

.. code-block:: cpp

   #include <ouly/dsl/lite_yml.hpp>

   // Print the structure of a document as the parser reports it
   class printer final : public ouly::yml::context
   {
   public:
     void begin_array() override
     {
       std::cout << "[\n";
     }
     void end_array() override
     {
       std::cout << "]\n";
     }
     void begin_object() override
     {
       std::cout << "{\n";
     }
     void end_object() override
     {
       std::cout << "}\n";
     }
     void begin_new_array_item() override
     {
       std::cout << "- ";
     }
     void set_key(std::string_view key) override
     {
       std::cout << key << ": ";
     }
     void set_value(std::string_view value) override // copy the view if you need to keep it
     {
       std::cout << value << '\n';
     }
   };

   void parse(std::string_view text)
   {
     printer            out;
     ouly::yml::lite_stream stream(text, &out);
     stream.parse();
   }

The string views passed to ``set_key`` and ``set_value`` are only valid during the call. Quoted
scalars are unescaped; scalars without escapes are passed through without a copy.

Views
-----

.. code-block:: cpp

   #include <ouly/utility/projected_view.hpp>
   #include <ouly/utility/subrange.hpp>
   #include <ouly/utility/zip_view.hpp>

   std::vector<std::string> names   = {"a", "b", "c"};
   std::vector<int>         weights = {1, 2, 3};
   for (auto&& [name, weight] : ouly::zip(names, weights)) // iterate several ranges in lockstep
   {
     name += std::to_string(weight);
   }

   std::vector<item> items = {{"sword", 3.5F}, {"shield", 6.0F}};
   auto              item_weights = ouly::projected_view<&item::weight>(items.data(), items.size());
   float             total        = 0;
   for (float w : item_weights) // one member across an array of structs
   {
     total += w;
   }

   ouly::subrange<uint32_t> indices{0, 100}; // the integers [0, 100), splittable for parallel work

``ouly::enumerate`` (in ``zip_view.hpp``) pairs elements with their index.

Pointers and handles
--------------------

``intrusive_ptr`` keeps the reference count inside the object; it finds the count through
``intrusive_count_add``, ``intrusive_count_sub`` and ``intrusive_count_get`` overloads:

.. code-block:: cpp

   #include <ouly/utility/intrusive_ptr.hpp>
   #include <ouly/utility/tagged_int.hpp>
   #include <ouly/utility/tagged_ptr.hpp>

   struct texture
   {
     std::string path;
     int         refs = 0;
   };

   // The reference count is found through these free functions (found by ADL)
   inline auto intrusive_count_add(texture* t) noexcept -> int
   {
     return t->refs++;
   }
   inline auto intrusive_count_sub(texture* t) noexcept -> int
   {
     return t->refs--;
   }
   inline auto intrusive_count_get(texture* t) noexcept -> int
   {
     return t->refs;
   }

.. code-block:: cpp

   ouly::intrusive_ptr<texture> albedo(new texture{"albedo.png"});
   auto                         shared = albedo; // refs == 2, no control block
   shared.reset();

   struct handle_tag
   {};
   using handle = ouly::tagged_int<handle_tag, uint32_t, 0xffffffff>; // distinct type, custom null
   handle h{7};
   bool   valid = static_cast<bool>(h); // compares against the null value

   std::string            text = "tagged";
   ouly::tagged_ptr<std::string> versioned;
   versioned.set(&text, versioned.get_next_tag()); // pointer plus a counter, e.g. against ABA

``tagged_ptr`` stores the tag next to the pointer; define ``OULY_PACK_TAGGED_POINTER`` to pack it
into the top byte of the pointer instead.

Hashing
-------

.. code-block:: cpp

   #include <ouly/utility/komihash.hpp>
   #include <ouly/utility/wyhash.hpp>

   std::string_view const key = "player.position";

   ouly::wyhash64 hasher;
   auto           h1 = hasher(key.data(), key.size()); // the hasher is seeded with its previous result,
   auto           h2 = hasher(key.data(), key.size()); // so h2 != h1; hasher() returns the last value

   constexpr auto compile_time = ouly::wyhash32::make("player.position", 15); // constexpr 32-bit hash

   ouly::komihash64_stream stream; // incremental hashing of several pieces
   stream(key.data(), 6);
   stream(key.data() + 6, key.size() - 6);
   auto h3 = stream();

Strings and numbers
-------------------

.. code-block:: cpp

   #include <ouly/utility/from_chars.hpp>
   #include <ouly/utility/string_literal.hpp>
   #include <ouly/utility/string_utils.hpp>

   auto [section, rest] = ouly::split("render:shadows", ':'); // "render", "shadows"
   auto trimmed         = ouly::trim_view("  value \t");    // "value"

   int  parsed = 0;
   ouly::from_chars("1234", parsed);

   constexpr ouly::string_literal name = "texture"; // a structural type: usable as a template argument
   constexpr auto                 id   = name.hash(); // FNV-1a, at compile time

``string_utils.hpp`` also has ``tokenize``, ``split_last``, ``replace``, ``trim_leading``,
``trim_trailing`` and ``is_number``. ``from_chars`` and ``to_chars`` wrap ``<charconv>`` and use
`fast_float <https://github.com/fastfloat/fast_float>`_ for floating point when it is available.

.. code-block:: cpp

   #include <ouly/utility/uint128.hpp>

   ouly::uint128_t big = ouly::uint128_t(1) << 100;
   big += 5;
   bool odd = (big & 1) == 1;

Other utilities
---------------

- ``optional_ref<T>`` (an optional reference), ``optional_val<Null>`` (a value whose null state is
  the sentinel ``Null``, so no extra flag is stored) and ``nullable_optional<T>`` (in-place storage
  with explicit ``emplace``/``reset``).
- ``ouly::any`` and ``ouly::runtime_type``: a small-buffer ``any`` and a type-id tagged value, used
  by the YAML runtime type registry.
- ``word_list`` (words in one string with a delimiter), ``ouly::tuple``, ``type_traits.hpp``
  (function traits) and ``utils.hpp`` (assorted helpers).

Next Steps
----------

* :doc:`serialization_tutorial` for the reflection layer that uses these pieces
* :doc:`../api_reference/index` for the class reference
