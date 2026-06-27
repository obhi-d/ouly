#include "catch2/catch_all.hpp"
#include "ouly/serializers/lite_yml.hpp"
#include "ouly/utility/any.hpp"
#include "ouly/utility/runtime_type.hpp"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

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

struct ListHolder
{
  std::vector<ouly::runtime_type> items;

  static constexpr auto reflect() noexcept
  {
    return ouly::bind(ouly::bind<"items", &ListHolder::items>());
  }
};

static auto make_registry() -> ouly::yml::runtime_type_registry<>
{
  ouly::yml::runtime_type_registry<> reg;
  reg.bind<Foo>(ouly::type_id{1});
  reg.bind<Bar>(ouly::type_id{2});
  reg.bind<int>(ouly::type_id{3});
  return reg;
}

TEST_CASE("ouly::any: inline and heap storage")
{
  ouly::any a;
  REQUIRE_FALSE(a.has_value());

  a.emplace<int>(42);
  REQUIRE(a.has_value());
  REQUIRE(a.get_if<int>() != nullptr);
  REQUIRE(*a.get_if<int>() == 42);
  REQUIRE(a.get_if<double>() == nullptr);

  // Larger-than-inline payload forces heap storage.
  struct big
  {
    std::array<std::uint64_t, 16> data{};
  };
  a.emplace<big>();
  a.get_if<big>()->data[3] = 7;
  REQUIRE(a.get_if<big>() != nullptr);
  REQUIRE(a.get_if<big>()->data[3] == 7);
  REQUIRE(a.get_if<int>() == nullptr);

  // Copy and move.
  ouly::any b = a;
  REQUIRE(b.get_if<big>()->data[3] == 7);
  ouly::any c = std::move(b);
  REQUIRE(c.get_if<big>()->data[3] == 7);

  a.reset();
  REQUIRE_FALSE(a.has_value());
}

TEST_CASE("yaml_runtime_type: parse object value")
{
  std::string yaml = R"(
item:
  type: 2
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

TEST_CASE("yaml_runtime_type: parse scalar value")
{
  std::string yaml = R"(
item:
  type: 3
  value: 123
)";

  auto   reg = make_registry();
  Holder h;
  ouly::yml::from_string(h, yaml, reg);

  REQUIRE(h.item.id_.id_ == 3);
  auto* value = h.item.value_.get_if<int>();
  REQUIRE(value != nullptr);
  REQUIRE(*value == 123);
}

TEST_CASE("yaml_runtime_type: round trip")
{
  auto   reg = make_registry();
  Holder h;
  h.item.id_ = ouly::type_id{1};
  h.item.value_.emplace<Foo>(Foo{42});

  auto str = ouly::yml::to_string(h, reg);

  Holder out;
  ouly::yml::from_string(out, str, reg);

  REQUIRE(out.item.id_.id_ == 1);
  auto* foo = out.item.value_.get_if<Foo>();
  REQUIRE(foo != nullptr);
  REQUIRE(foo->x == 42);
}

TEST_CASE("yaml_runtime_type: vector of mixed runtime types")
{
  auto       reg = make_registry();
  ListHolder lh;
  {
    ouly::runtime_type a;
    a.id_ = ouly::type_id{1};
    a.value_.emplace<Foo>(Foo{7});
    lh.items.push_back(std::move(a));
  }
  {
    ouly::runtime_type b;
    b.id_ = ouly::type_id{2};
    b.value_.emplace<Bar>(Bar{"world"});
    lh.items.push_back(std::move(b));
  }

  auto str = ouly::yml::to_string(lh, reg);

  ListHolder out;
  ouly::yml::from_string(out, str, reg);

  REQUIRE(out.items.size() == 2);
  REQUIRE(out.items[0].id_.id_ == 1);
  REQUIRE(out.items[0].value_.get_if<Foo>() != nullptr);
  REQUIRE(out.items[0].value_.get_if<Foo>()->x == 7);
  REQUIRE(out.items[1].id_.id_ == 2);
  REQUIRE(out.items[1].value_.get_if<Bar>() != nullptr);
  REQUIRE(out.items[1].value_.get_if<Bar>()->s == "world");
}

TEST_CASE("yaml_runtime_type: customizable type/value field names (write + round trip)")
{
  // The "type"/"value" keys follow the Config's string transform, exactly like
  // std::variant's keys. Using a to_upper transform renames them to TYPE/VALUE.
  using upper_config = ouly::config<ouly::to_upper>;

  ouly::yml::runtime_type_registry<upper_config> reg;
  reg.bind<Foo>(ouly::type_id{1});

  Holder h;
  h.item.id_ = ouly::type_id{1};
  h.item.value_.emplace<Foo>(Foo{42});

  auto str = ouly::yml::to_string(h, reg);

  REQUIRE(str.find("TYPE:") != std::string::npos);
  REQUIRE(str.find("VALUE:") != std::string::npos);
  REQUIRE(str.find("type:") == std::string::npos);
  REQUIRE(str.find("value:") == std::string::npos);

  Holder out;
  ouly::yml::from_string(out, str, reg);
  REQUIRE(out.item.id_.id_ == 1);
  REQUIRE(out.item.value_.get_if<Foo>() != nullptr);
  REQUIRE(out.item.value_.get_if<Foo>()->x == 42);
}

TEST_CASE("yaml_runtime_type: parse with customized field names")
{
  using upper_config = ouly::config<ouly::to_upper>;

  std::string yaml = R"(
ITEM:
  TYPE: 2
  VALUE:
    S: hello
)";

  ouly::yml::runtime_type_registry<upper_config> reg;
  reg.bind<Bar>(ouly::type_id{2});

  Holder h;
  ouly::yml::from_string(h, yaml, reg);

  REQUIRE(h.item.id_.id_ == 2);
  auto* bar = h.item.value_.get_if<Bar>();
  REQUIRE(bar != nullptr);
  REQUIRE(bar->s == "hello");
}

static auto name_for(ouly::type_id id) -> std::string_view
{
  switch (id.id_)
  {
  case 1:
    return "Foo";
  case 2:
    return "Bar";
  case 3:
    return "Int";
  default:
    return "";
  }
}

static auto id_for(std::string_view name) -> ouly::type_id
{
  if (name == "Foo")
  {
    return ouly::type_id{1};
  }
  if (name == "Bar")
  {
    return ouly::type_id{2};
  }
  if (name == "Int")
  {
    return ouly::type_id{3};
  }
  return ouly::type_id{0};
}

TEST_CASE("yaml_runtime_type: parse string type id via registry name resolver")
{
  std::string yaml = R"(
item:
  type: Bar
  value:
    s: hello
)";

  auto reg = make_registry();
  reg.set_name_resolver(id_for, name_for);

  Holder h;
  ouly::yml::from_string(h, yaml, reg);

  REQUIRE(h.item.id_.id_ == 2);
  auto* bar = h.item.value_.get_if<Bar>();
  REQUIRE(bar != nullptr);
  REQUIRE(bar->s == "hello");
}

TEST_CASE("yaml_runtime_type: round trip with registry name resolver emits names")
{
  auto reg = make_registry();
  reg.set_name_resolver(id_for, name_for);

  Holder h;
  h.item.id_ = ouly::type_id{1};
  h.item.value_.emplace<Foo>(Foo{42});

  auto str = ouly::yml::to_string(h, reg);

  // The "type" field is emitted as the resolved name, never the integer id.
  REQUIRE(str.find("Foo") != std::string::npos);
  REQUIRE(str.find("type: 1") == std::string::npos);

  Holder out;
  ouly::yml::from_string(out, str, reg);

  REQUIRE(out.item.id_.id_ == 1);
  auto* foo = out.item.value_.get_if<Foo>();
  REQUIRE(foo != nullptr);
  REQUIRE(foo->x == 42);
}

TEST_CASE("yaml_runtime_type: unknown type id throws")
{
  std::string yaml = R"(
item:
  type: 99
  value:
    s: hello
)";

  auto   reg = make_registry();
  Holder h;
  REQUIRE_THROWS_AS(ouly::yml::from_string(h, yaml, reg), ouly::visitor_error);
}
// NOLINTEND
