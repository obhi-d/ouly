#include "catch2/catch_all.hpp"
#include "ouly/utility/convert.hpp"
#include "ouly/utility/runtime_type.hpp"
#include <string>
#include <string_view>

// A convert<type_id> specialization makes the runtime_type "type" field a string
// (a human-readable type name) instead of the default integer. The presence of this
// specialization is what switches the (de)serializer onto the string path.
template <>
struct ouly::convert<ouly::type_id>
{
  static auto to_type(ouly::type_id const& id) -> std::string
  {
    switch (id.id_)
    {
    case 1:
      return "Foo";
    case 2:
      return "Bar";
    default:
      return "";
    }
  }

  static void from_type(ouly::type_id& id, std::string_view name)
  {
    if (name == "Foo")
    {
      id.id_ = 1;
    }
    else if (name == "Bar")
    {
      id.id_ = 2;
    }
    else
    {
      id.id_ = 0;
    }
  }
};

#include "ouly/serializers/lite_yml.hpp"

// NOLINTBEGIN
struct Foo
{
  int x = 0;

  static constexpr auto reflect() noexcept
  {
    return ouly::bind(ouly::bind<"x", &Foo::x>());
  }
};

struct Bar
{
  std::string s;

  static constexpr auto reflect() noexcept
  {
    return ouly::bind(ouly::bind<"s", &Bar::s>());
  }
};

struct Holder
{
  ouly::runtime_type item;

  static constexpr auto reflect() noexcept
  {
    return ouly::bind(ouly::bind<"item", &Holder::item>());
  }
};

static auto make_registry() -> ouly::yml::runtime_type_registry<>
{
  ouly::yml::runtime_type_registry<> reg;
  reg.bind<Foo>(ouly::type_id{1});
  reg.bind<Bar>(ouly::type_id{2});
  return reg;
}

TEST_CASE("yaml_runtime_type_convert: parse string type id")
{
  std::string yaml = R"(
item:
  type: Bar
  value:
    s: hello
)";

  auto   reg = make_registry();
  Holder h;
  ouly::yml::from_string(h, yaml, reg);

  REQUIRE(h.item.id_.id_ == 2);
  auto* bar = h.item.value_.get_if<Bar>();
  REQUIRE(bar != nullptr);
  REQUIRE(bar->s == "hello");
}

TEST_CASE("yaml_runtime_type_convert: round trip emits string type id")
{
  auto   reg = make_registry();
  Holder h;
  h.item.id_ = ouly::type_id{1};
  h.item.value_.emplace<Foo>(Foo{42});

  auto str = ouly::yml::to_string(h, reg);

  // The type field is emitted as the converted name, not the integer.
  REQUIRE(str.find("Foo") != std::string::npos);

  Holder out;
  ouly::yml::from_string(out, str, reg);

  REQUIRE(out.item.id_.id_ == 1);
  auto* foo = out.item.value_.get_if<Foo>();
  REQUIRE(foo != nullptr);
  REQUIRE(foo->x == 42);
}
// NOLINTEND
