
#include "acl/reflection/detail/base_concepts.hpp"
#include "acl/reflection/visitor.hpp"
#include <acl/serializers/lite_yml.hpp>
#include <array>
#include <catch2/catch_all.hpp>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <variant>
#include <vector>

// NOLINTBEGIN
struct TestStruct
{
  int         a;
  double      b;
  std::string c;

  static constexpr auto reflect() noexcept
  {
    return acl::bind(acl::bind<"a", &TestStruct::a>(), acl::bind<"b", &TestStruct::b>(),
                     acl::bind<"c", &TestStruct::c>());
  }
};

TEST_CASE("yaml_object: Test read")
{
  std::string yml = R"(
a: 100
b: 200.0
c: "value"
)";

  TestStruct ts;
  acl::yml::from_string(ts, yml);

  REQUIRE(ts.a == 100);
  REQUIRE(ts.b == 200.0);
  REQUIRE(ts.c == "value");
}

struct TestStruct2
{
  int                   a;
  double                b;
  std::string           c;
  TestStruct            d;
  static constexpr auto reflect() noexcept
  {
    return acl::bind(acl::bind<"a", &TestStruct2::a>(), acl::bind<"b", &TestStruct2::b>(),
                     acl::bind<"c", &TestStruct2::c>(), acl::bind<"d", &TestStruct2::d>());
  }
};

TEST_CASE("yaml_object: Test simple nested")
{
  struct aggregate
  {
    int value = -1;
  };
  struct instance
  {
    int       parent = -1;
    aggregate child;
  };
  std::string yml = R"(
parent: 100
child:
  value: 300
)";
  instance    ts;
  acl::yml::from_string(ts, yml);
  REQUIRE(ts.parent == 100);
  REQUIRE(ts.child.value == 300);
}

TEST_CASE("yaml_object: Test read nested")
{
  std::string yml = R"(
a: 100
b: 200.0
c: "value"
d:
  a: 300
  b: 400.0
  c: "value2"
)";
  TestStruct2 ts;
  acl::yml::from_string(ts, yml);
  REQUIRE(ts.a == 100);
  REQUIRE(ts.b == 200.0);
  REQUIRE(ts.c == "value");
  REQUIRE(ts.d.a == 300);
  REQUIRE(ts.d.b == 400.0);
  REQUIRE(ts.d.c == "value2");
}

TEST_CASE("yaml_object: Test read vector")
{
  std::string yml = R"(
numbers:
  - 1
  - 2
  - 3
)";
  struct TestStructVector
  {
    std::vector<int>      numbers;
    static constexpr auto reflect() noexcept
    {
      return acl::bind(acl::bind<"numbers", &TestStructVector::numbers>());
    }
  };

  TestStructVector ts;
  acl::yml::from_string(ts, yml);

  REQUIRE(ts.numbers.size() == 3);
  REQUIRE(ts.numbers[0] == 1);
  REQUIRE(ts.numbers[1] == 2);
  REQUIRE(ts.numbers[2] == 3);
}

TEST_CASE("yaml_object: Test read optional")
{
  std::string yaml_present = R"(
value: 42
)";

  std::string yaml_absent = R"(
)";

  struct TestStructOptional
  {
    std::optional<int>    value;
    static constexpr auto reflect() noexcept
    {
      return acl::bind(acl::bind<"value", &TestStructOptional::value>());
    }
  };

  // Test with value present
  {
    TestStructOptional ts;
    acl::yml::from_string(ts, yaml_present);
    REQUIRE(ts.value.has_value());
    REQUIRE(ts.value.value() == 42);
  }

  // Test with value absent
  {
    TestStructOptional ts;
    acl::yml::from_string(ts, yaml_absent);
    REQUIRE(!ts.value.has_value());
  }
}

enum class Color
{
  Red   = 0,
  Green = 1,
  Blue  = 2
};

TEST_CASE("yaml_object: Test read enum")
{
  std::string yml = R"(
color: 1
)";

  struct TestStructEnum
  {
    Color                 color;
    static constexpr auto reflect() noexcept
    {
      return acl::bind(acl::bind<"color", &TestStructEnum::color>());
    }
  };

  TestStructEnum ts;
  acl::yml::from_string(ts, yml);

  REQUIRE(ts.color == Color::Green);
}

TEST_CASE("yaml_object: Test read pointer")
{
  std::string yaml_present = R"(
value: 42
)";

  std::string yaml_null = R"(
value: null
)";

  struct TestStructPointer
  {
    std::unique_ptr<int>  value;
    static constexpr auto reflect() noexcept
    {
      return acl::bind(acl::bind<"value", &TestStructPointer::value>());
    }
  };

  // Test with value present
  {
    TestStructPointer ts;
    acl::yml::from_string(ts, yaml_present);
    REQUIRE(ts.value != nullptr);
    REQUIRE(*ts.value == 42);
  }

  // Test with value null
  {
    TestStructPointer ts;
    acl::yml::from_string(ts, yaml_null);
    REQUIRE(ts.value == nullptr);
  }
}

TEST_CASE("yaml_object: Test read tuple")
{
  std::string yml = R"(
tuple:
  - 1
  - "string"
  - 3.14
)";

  struct TestStructTuple
  {
    std::tuple<int, std::string, double> tuple;
    static constexpr auto                reflect() noexcept
    {
      return acl::bind(acl::bind<"tuple", &TestStructTuple::tuple>());
    }
  };

  TestStructTuple ts;
  acl::yml::from_string(ts, yml);

  REQUIRE(std::get<0>(ts.tuple) == 1);
  REQUIRE(std::get<1>(ts.tuple) == "string");
  REQUIRE(std::get<2>(ts.tuple) == 3.14);
}

TEST_CASE("yaml_object: Test read array")
{
  std::string yml = R"(
array:
  - 10
  - 20
  - 30
)";

  struct TestStructArray
  {
    std::array<int, 3>    array;
    static constexpr auto reflect() noexcept
    {
      return acl::bind(acl::bind<"array", &TestStructArray::array>());
    }
  };

  TestStructArray ts;
  acl::yml::from_string(ts, yml);

  REQUIRE(ts.array[0] == 10);
  REQUIRE(ts.array[1] == 20);
  REQUIRE(ts.array[2] == 30);
}

TEST_CASE("yaml_object: Test read compact array ")
{
  std::string yml = R"(
array: [10, 20, 30]
)";

  struct TestStructArray
  {
    std::array<int, 3>    array;
    static constexpr auto reflect() noexcept
    {
      return acl::bind(acl::bind<"array", &TestStructArray::array>());
    }
  };

  TestStructArray ts;
  acl::yml::from_string(ts, yml);

  REQUIRE(ts.array[0] == 10);
  REQUIRE(ts.array[1] == 20);
  REQUIRE(ts.array[2] == 30);
}

TEST_CASE("yaml_object: Test read boolean")
{
  std::string yml = R"(
flag1: true
flag2: false
)";

  struct TestStructBool
  {
    bool                  flag1;
    bool                  flag2;
    static constexpr auto reflect() noexcept
    {
      return acl::bind(acl::bind<"flag1", &TestStructBool::flag1>(), acl::bind<"flag2", &TestStructBool::flag2>());
    }
  };

  TestStructBool ts;
  acl::yml::from_string(ts, yml);

  REQUIRE(ts.flag1 == true);
  REQUIRE(ts.flag2 == false);
}

TEST_CASE("yaml_object: Test read variant")
{
  std::string yaml_int = R"(
var:
  type: 0
  value: 42
)";

  std::string yaml_string = R"(
var:
  type: 1
  value: "hello"
)";

  using VarType = std::variant<int, std::string>;

  struct TestStructVariant
  {
    VarType               var;
    static constexpr auto reflect() noexcept
    {
      return acl::bind(acl::bind<"var", &TestStructVariant::var>());
    }
  };

  {
    TestStructVariant ts;
    acl::yml::from_string(ts, yaml_int);
    REQUIRE(std::holds_alternative<int>(ts.var));
    REQUIRE(std::get<int>(ts.var) == 42);
  }

  {
    TestStructVariant ts;
    acl::yml::from_string(ts, yaml_string);
    REQUIRE(std::holds_alternative<std::string>(ts.var));
    REQUIRE(std::get<std::string>(ts.var) == "hello");
  }
}

TEST_CASE("yaml_object: Test read invalid YAML")
{
  std::string yml = R"(
a: 100
b: [1, 2, 3
c: "value
)";

  struct TestStructInvalid
  {
    int              a;
    std::vector<int> b;
    std::string      c;

    static constexpr auto reflect() noexcept
    {
      return acl::bind(acl::bind<"a", &TestStructInvalid::a>(), acl::bind<"b", &TestStructInvalid::b>(),
                       acl::bind<"c", &TestStructInvalid::c>());
    }
  };

  TestStructInvalid ts;
  REQUIRE_THROWS_AS(acl::yml::from_string(ts, yml), std::runtime_error);
}

TEST_CASE("yaml_object: Test read block scalar literals")
{
  std::string yml = R"(
literal_block: |
  This is a block of text
  that spans multiple lines.

folded_block: >
  This is another block
  that folds newlines
  into spaces.
)";

  struct TestStructBlockScalar
  {
    std::string literal_block;
    std::string folded_block;

    static constexpr auto reflect() noexcept
    {
      return acl::bind(acl::bind<"literal_block", &TestStructBlockScalar::literal_block>(),
                       acl::bind<"folded_block", &TestStructBlockScalar::folded_block>());
    }
  };

  TestStructBlockScalar ts;
  acl::yml::from_string(ts, yml);

  REQUIRE(ts.literal_block == "This is a block of text\nthat spans multiple lines.");
  REQUIRE(ts.folded_block == "This is another block that folds newlines into spaces.");
}

TEST_CASE("yaml_object: Test read with unexpected token")
{
  std::string yml = R"(
list:
  - 1
  - 2
  - x3
)";

  struct TestStructUnexpectedToken
  {
    std::vector<int> list;

    static constexpr auto reflect() noexcept
    {
      return acl::bind(acl::bind<"list", &TestStructUnexpectedToken::list>());
    }
  };

  TestStructUnexpectedToken ts;
  REQUIRE_THROWS_AS(acl::yml::from_string(ts, yml), acl::visitor_error);
}

TEST_CASE("yaml_object: Test read with missing key")
{
  std::string yml = R"(
a: 100
c: "value"
)";

  struct TestStructMissingKey
  {
    int         a;
    int         b = -100; // Missing in YAML
    std::string c;

    static constexpr auto reflect() noexcept
    {
      return acl::bind(acl::bind<"a", &TestStructMissingKey::a>(), acl::bind<"b", &TestStructMissingKey::b>(),
                       acl::bind<"c", &TestStructMissingKey::c>());
    }
  };

  TestStructMissingKey ts;
  acl::yml::from_string(ts, yml);

  REQUIRE(ts.a == 100);
  REQUIRE(ts.b == -100); // value untouched
  REQUIRE(ts.c == "value");
}

TEST_CASE("yaml_object: Test read with extra fields")
{
  std::string yml = R"(
a: 100
b: 200
c: "value"
extra_field: "cause a crash"
)";

  struct TestStructExtraField
  {
    int         a;
    int         b;
    std::string c;

    static constexpr auto reflect() noexcept
    {
      return acl::bind(acl::bind<"a", &TestStructExtraField::a>(), acl::bind<"b", &TestStructExtraField::b>(),
                       acl::bind<"c", &TestStructExtraField::c>());
    }
  };

  TestStructExtraField ts;
  REQUIRE_THROWS_AS(acl::yml::from_string(ts, yml), acl::visitor_error);
}

TEST_CASE("yaml_object: Test read of unexpected type")
{
  std::string yml = R"(
number: "not_a_number"
)";

  struct TestStructUnexpectedType
  {
    int number;

    static constexpr auto reflect() noexcept
    {
      return acl::bind(acl::bind<"number", &TestStructUnexpectedType::number>());
    }
  };

  TestStructUnexpectedType ts;
  REQUIRE_THROWS_AS(acl::yml::from_string(ts, yml), acl::visitor_error);
}

TEST_CASE("yaml_object: Test read recursive structures")
{
  std::string yml = R"(
node:
  value: 1
  next:
    value: 2
    next:
      value: 3
)";

  struct Node
  {
    int                   value;
    std::unique_ptr<Node> next;

    static constexpr auto reflect() noexcept
    {
      return acl::bind(acl::bind<"value", &Node::value>(), acl::bind<"next", &Node::next>());
    }
  };

  struct TestStructRecursive
  {
    Node node;

    static constexpr auto reflect() noexcept
    {
      return acl::bind(acl::bind<"node", &TestStructRecursive::node>());
    }
  };

  TestStructRecursive ts;
  acl::yml::from_string(ts, yml);

  REQUIRE(ts.node.value == 1);
  REQUIRE(ts.node.next->value == 2);
  REQUIRE(ts.node.next->next->value == 3);
  REQUIRE(ts.node.next->next->next == nullptr);
}

TEST_CASE("yaml_object: Test read with incorrect type casting")
{
  std::string yml = R"(
value: "string_instead_of_int"
)";

  struct TestStructTypeCast
  {
    int value;

    static constexpr auto reflect() noexcept
    {
      return acl::bind(acl::bind<"value", &TestStructTypeCast::value>());
    }
  };

  TestStructTypeCast ts;
  REQUIRE_THROWS_AS(acl::yml::from_string(ts, yml), acl::visitor_error);
}

TEST_CASE("yaml_object: Test read with empty YAML")
{
  std::string yml = "";

  struct TestStructEmpty
  {
    int a = -1;

    static constexpr auto reflect() noexcept
    {
      return acl::bind(acl::bind<"a", &TestStructEmpty::a>());
    }
  };

  TestStructEmpty ts;
  acl::yml::from_string(ts, yml);

  // Since YAML is empty, the value should remain default-initialized
  REQUIRE(ts.a == -1);
}

TEST_CASE("yaml_object: Test read with null value")
{
  std::string yml = R"(
value: null
)";

  struct TestStructNull
  {
    std::optional<int> value = 10;

    static constexpr auto reflect() noexcept
    {
      return acl::bind(acl::bind<"value", &TestStructNull::value>());
    }
  };

  TestStructNull ts;
  acl::yml::from_string(ts, yml);

  // The optional should not be set
  REQUIRE(!ts.value.has_value());
}

TEST_CASE("yaml_object: Test read large numbers")
{
  std::string yml = R"(
int_max: 9223372036854775807
int_min: -9223372036854775808
uint_max: 18446744073709551615
)";

  struct TestStructLargeNumbers
  {
    int64_t  int_max;
    int64_t  int_min;
    uint64_t uint_max;

    static constexpr auto reflect() noexcept
    {
      return acl::bind(acl::bind<"int_max", &TestStructLargeNumbers::int_max>(),
                       acl::bind<"int_min", &TestStructLargeNumbers::int_min>(),
                       acl::bind<"uint_max", &TestStructLargeNumbers::uint_max>());
    }
  };

  TestStructLargeNumbers ts;
  acl::yml::from_string(ts, yml);

  REQUIRE(ts.int_max == INT64_MAX);
  REQUIRE(ts.int_min == INT64_MIN);
  REQUIRE(ts.uint_max == UINT64_MAX);
}

TEST_CASE("yaml_object: Test read floating-point edge cases")
{
  std::string yml = R"(
positive_infinity: .inf
negative_infinity: -.inf
not_a_number: .nan
)";

  struct TestStructFloatEdgeCases
  {
    double positive_infinity;
    double negative_infinity;
    double not_a_number;

    static constexpr auto reflect() noexcept
    {
      return acl::bind(acl::bind<"positive_infinity", &TestStructFloatEdgeCases::positive_infinity>(),
                       acl::bind<"negative_infinity", &TestStructFloatEdgeCases::negative_infinity>(),
                       acl::bind<"not_a_number", &TestStructFloatEdgeCases::not_a_number>());
    }
  };

  TestStructFloatEdgeCases ts;
  acl::yml::from_string(ts, yml);

  REQUIRE(std::isinf(ts.positive_infinity));
  REQUIRE(ts.positive_infinity > 0);

  REQUIRE(std::isinf(ts.negative_infinity));
  REQUIRE(ts.negative_infinity < 0);

  REQUIRE(std::isnan(ts.not_a_number));
}

TEST_CASE("yaml_object: Test read deeply nested structures")
{
  std::string yml = R"(
level1:
  level2:
    level3:
      level4:
        value: 42
)";

  struct Level4
  {
    int value;

    static constexpr auto reflect() noexcept
    {
      return acl::bind(acl::bind<"value", &Level4::value>());
    }
  };

  struct Level3
  {
    Level4 level4;

    static constexpr auto reflect() noexcept
    {
      return acl::bind(acl::bind<"level4", &Level3::level4>());
    }
  };

  struct Level2
  {
    Level3 level3;

    static constexpr auto reflect() noexcept
    {
      return acl::bind(acl::bind<"level3", &Level2::level3>());
    }
  };

  struct Level1
  {
    Level2 level2;

    static constexpr auto reflect() noexcept
    {
      return acl::bind(acl::bind<"level2", &Level1::level2>());
    }
  };

  struct Level0
  {
    Level1 level1;
  };

  Level0 ts;
  acl::yml::from_string(ts, yml);

  REQUIRE(ts.level1.level2.level3.level4.value == 42);
}

TEST_CASE("yaml_object: Test read sequence of maps with single key")
{
  std::string yml = R"(
- name: "Item1"
- name: "Item2"
- name: "Item3"
)";

  struct Item
  {
    std::string name;
  };

  using ItemsList = std::vector<Item>;
  ItemsList items;

  acl::yml::from_string(items, yml);

  REQUIRE(items.size() == 3);
  REQUIRE(items[0].name == "Item1");
  REQUIRE(items[1].name == "Item2");
  REQUIRE(items[2].name == "Item3");
}

TEST_CASE("yaml_object: Test read sequence of maps")
{
  std::string yml = R"(
- name: "Item1"
  value: 10
- name: "Item2"
  value: 20
- name: "Item3"
  value: 30
)";

  struct Item
  {
    std::string name;
    int         value;

    static constexpr auto reflect() noexcept
    {
      return acl::bind(acl::bind<"name", &Item::name>(), acl::bind<"value", &Item::value>());
    }
  };

  using ItemsList = std::vector<Item>;
  ItemsList items;

  acl::yml::from_string(items, yml);

  REQUIRE(items.size() == 3);
  REQUIRE(items[0].name == "Item1");
  REQUIRE(items[0].value == 10);
  REQUIRE(items[1].name == "Item2");
  REQUIRE(items[1].value == 20);
  REQUIRE(items[2].name == "Item3");
  REQUIRE(items[2].value == 30);
}

TEST_CASE("yaml_object: Test read with duplicate keys")
{
  std::string yml = R"(
a: 1
a: 2
)";

  struct TestStructDuplicateKeys
  {
    int a;

    static constexpr auto reflect() noexcept
    {
      return acl::bind(acl::bind<"a", &TestStructDuplicateKeys::a>());
    }
  };

  TestStructDuplicateKeys ts;
  acl::yml::from_string(ts, yml);

  // Expect the last value to override
  REQUIRE(ts.a == 2);
}

TEST_CASE("yaml_object: Test read map types")
{
  std::string yml = R"(
map:
  - [1, "one"]
  - [2, "two"]
  - [3, "three"]
)";

  struct TestStructMap
  {
    std::map<int, std::string> map;
  };

  TestStructMap ts;
  acl::yml::from_string(ts, yml);

  REQUIRE(ts.map.size() == 3);
  REQUIRE(ts.map[1] == "one");
  REQUIRE(ts.map[2] == "two");
  REQUIRE(ts.map[3] == "three");
}

// NOLINTEND