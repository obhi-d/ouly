#include "catch2/catch_all.hpp"
#include "ouly/serializers/lite_yml.hpp"
#include <array>
#include <map>
#include <memory>
#include <optional>
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
    return ouly::bind(ouly::bind<"a", &TestStruct::a>(), ouly::bind<"b", &TestStruct::b>(),
                      ouly::bind<"c", &TestStruct::c>());
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
  ouly::yml::from_string(ts, yml);

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
    return ouly::bind(ouly::bind<"a", &TestStruct2::a>(), ouly::bind<"b", &TestStruct2::b>(),
                      ouly::bind<"c", &TestStruct2::c>(), ouly::bind<"d", &TestStruct2::d>());
  }
};

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
  ouly::yml::from_string(ts, yml);
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
      return ouly::bind(ouly::bind<"numbers", &TestStructVector::numbers>());
    }
  };

  TestStructVector ts;
  ouly::yml::from_string(ts, yml);

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
      return ouly::bind(ouly::bind<"value", &TestStructOptional::value>());
    }
  };

  // Test with value present
  {
    TestStructOptional ts;
    ouly::yml::from_string(ts, yaml_present);
    REQUIRE(ts.value.has_value());
    REQUIRE(ts.value.value() == 42);
  }

  // Test with value absent
  {
    TestStructOptional ts;
    ouly::yml::from_string(ts, yaml_absent);
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
      return ouly::bind(ouly::bind<"color", &TestStructEnum::color>());
    }
  };

  TestStructEnum ts;
  ouly::yml::from_string(ts, yml);

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
      return ouly::bind(ouly::bind<"value", &TestStructPointer::value>());
    }
  };

  // Test with value present
  {
    TestStructPointer ts;
    ouly::yml::from_string(ts, yaml_present);
    REQUIRE(ts.value != nullptr);
    REQUIRE(*ts.value == 42);
  }

  // Test with value null
  {
    TestStructPointer ts;
    ouly::yml::from_string(ts, yaml_null);
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
      return ouly::bind(ouly::bind<"tuple", &TestStructTuple::tuple>());
    }
  };

  TestStructTuple ts;
  ouly::yml::from_string(ts, yml);

  REQUIRE(std::get<0>(ts.tuple) == 1);
  REQUIRE(std::get<1>(ts.tuple) == "string");
  REQUIRE(std::get<2>(ts.tuple) == 3.14);
}

TEST_CASE("yaml_object: Test read map")
{
  std::string yml = R"(
map:
  - [key1, 1]
  - [key2, 2]
  - [key3, 3]
)";

  struct TestStructMap
  {
    std::map<std::string, int> map;
    static constexpr auto      reflect() noexcept
    {
      return ouly::bind(ouly::bind<"map", &TestStructMap::map>());
    }
  };

  TestStructMap ts;
  ouly::yml::from_string(ts, yml);

  REQUIRE(ts.map.size() == 3);
  REQUIRE(ts.map["key1"] == 1);
  REQUIRE(ts.map["key2"] == 2);
  REQUIRE(ts.map["key3"] == 3);
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
      return ouly::bind(ouly::bind<"array", &TestStructArray::array>());
    }
  };

  TestStructArray ts;
  ouly::yml::from_string(ts, yml);

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
      return ouly::bind(ouly::bind<"array", &TestStructArray::array>());
    }
  };

  TestStructArray ts;
  ouly::yml::from_string(ts, yml);

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
      return ouly::bind(ouly::bind<"flag1", &TestStructBool::flag1>(), ouly::bind<"flag2", &TestStructBool::flag2>());
    }
  };

  TestStructBool ts;
  ouly::yml::from_string(ts, yml);

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
      return ouly::bind(ouly::bind<"var", &TestStructVariant::var>());
    }
  };

  {
    TestStructVariant ts;
    ouly::yml::from_string(ts, yaml_int);
    REQUIRE(std::holds_alternative<int>(ts.var));
    REQUIRE(std::get<int>(ts.var) == 42);
  }

  {
    TestStructVariant ts;
    ouly::yml::from_string(ts, yaml_string);
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
      return ouly::bind(ouly::bind<"a", &TestStructInvalid::a>(), ouly::bind<"b", &TestStructInvalid::b>(),
                        ouly::bind<"c", &TestStructInvalid::c>());
    }
  };

  TestStructInvalid ts;
  REQUIRE_THROWS_AS(ouly::yml::from_string(ts, yml), std::runtime_error);
}

TEST_CASE("yaml_object: Test read complex nested structure")
{
  std::string yml = R"(
root:
  child1:
    grandchild:
      - item1
      - item2
  child2:
    - [key1, value1]
    - 
      - key2
      - value2
)";

  struct TestStructComplex
  {
    struct Child1
    {
      std::vector<std::string> grandchild;

      static constexpr auto reflect() noexcept
      {
        return ouly::bind(ouly::bind<"grandchild", &Child1::grandchild>());
      }
    };

    std::map<std::string, std::string> child2;
    Child1                             child1;

    static constexpr auto reflect() noexcept
    {
      return ouly::bind(ouly::bind<"child1", &TestStructComplex::child1>(),
                        ouly::bind<"child2", &TestStructComplex::child2>());
    }
  };

  struct Root
  {
    TestStructComplex root;

    static constexpr auto reflect() noexcept
    {
      return ouly::bind(ouly::bind<"root", &Root::root>());
    }
  };

  Root ts;
  ouly::yml::from_string(ts, yml);

  REQUIRE(ts.root.child1.grandchild.size() == 2);
  REQUIRE(ts.root.child1.grandchild[0] == "item1");
  REQUIRE(ts.root.child1.grandchild[1] == "item2");
  REQUIRE(ts.root.child2["key1"] == "value1");
  REQUIRE(ts.root.child2["key2"] == "value2");
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
      return ouly::bind(ouly::bind<"literal_block", &TestStructBlockScalar::literal_block>(),
                        ouly::bind<"folded_block", &TestStructBlockScalar::folded_block>());
    }
  };

  TestStructBlockScalar ts;
  ouly::yml::from_string(ts, yml);

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
      return ouly::bind(ouly::bind<"list", &TestStructUnexpectedToken::list>());
    }
  };

  TestStructUnexpectedToken ts;
  REQUIRE_THROWS_AS(ouly::yml::from_string(ts, yml), std::runtime_error);
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
      return ouly::bind(ouly::bind<"a", &TestStructMissingKey::a>(), ouly::bind<"b", &TestStructMissingKey::b>(),
                        ouly::bind<"c", &TestStructMissingKey::c>());
    }
  };

  TestStructMissingKey ts;
  ouly::yml::from_string(ts, yml);

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
extra_field: "should be ignored"
)";

  struct TestStructExtraField
  {
    int         a;
    int         b;
    std::string c;

    static constexpr auto reflect() noexcept
    {
      return ouly::bind(ouly::bind<"a", &TestStructExtraField::a>(), ouly::bind<"b", &TestStructExtraField::b>(),
                        ouly::bind<"c", &TestStructExtraField::c>());
    }
  };

  TestStructExtraField ts;
  REQUIRE_THROWS_AS(ouly::yml::from_string(ts, yml), ouly::visitor_error);
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
      return ouly::bind(ouly::bind<"number", &TestStructUnexpectedType::number>());
    }
  };

  TestStructUnexpectedType ts;
  REQUIRE_THROWS_AS(ouly::yml::from_string(ts, yml), std::runtime_error);
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
      return ouly::bind(ouly::bind<"value", &Node::value>(), ouly::bind<"next", &Node::next>());
    }
  };

  struct TestStructRecursive
  {
    Node node;

    static constexpr auto reflect() noexcept
    {
      return ouly::bind(ouly::bind<"node", &TestStructRecursive::node>());
    }
  };

  TestStructRecursive ts;
  ouly::yml::from_string(ts, yml);

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
      return ouly::bind(ouly::bind<"value", &TestStructTypeCast::value>());
    }
  };

  TestStructTypeCast ts;
  REQUIRE_THROWS_AS(ouly::yml::from_string(ts, yml), std::runtime_error);
}

TEST_CASE("yaml_object: Test read with empty YAML")
{
  std::string yml = "";

  struct TestStructEmpty
  {
    int a = -1;

    static constexpr auto reflect() noexcept
    {
      return ouly::bind(ouly::bind<"a", &TestStructEmpty::a>());
    }
  };

  TestStructEmpty ts;
  ouly::yml::from_string(ts, yml);

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
      return ouly::bind(ouly::bind<"value", &TestStructNull::value>());
    }
  };

  TestStructNull ts;
  ouly::yml::from_string(ts, yml);

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
      return ouly::bind(ouly::bind<"int_max", &TestStructLargeNumbers::int_max>(),
                        ouly::bind<"int_min", &TestStructLargeNumbers::int_min>(),
                        ouly::bind<"uint_max", &TestStructLargeNumbers::uint_max>());
    }
  };

  TestStructLargeNumbers ts;
  ouly::yml::from_string(ts, yml);

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
      return ouly::bind(ouly::bind<"positive_infinity", &TestStructFloatEdgeCases::positive_infinity>(),
                        ouly::bind<"negative_infinity", &TestStructFloatEdgeCases::negative_infinity>(),
                        ouly::bind<"not_a_number", &TestStructFloatEdgeCases::not_a_number>());
    }
  };

  TestStructFloatEdgeCases ts;
  ouly::yml::from_string(ts, yml);

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
  };

  struct Level3
  {
    Level4 level4;
  };

  struct Level2
  {
    Level3 level3;
  };

  struct Level1
  {
    Level2 level2;
  };

  struct Level0
  {
    Level1 level1;
  };

  Level0 ts;
  ouly::yml::from_string(ts, yml);

  REQUIRE(ts.level1.level2.level3.level4.value == 42);
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
      return ouly::bind(ouly::bind<"name", &Item::name>(), ouly::bind<"value", &Item::value>());
    }
  };

  using ItemsList = std::vector<Item>;
  ItemsList items;

  ouly::yml::from_string(items, yml);

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
      return ouly::bind(ouly::bind<"a", &TestStructDuplicateKeys::a>());
    }
  };

  TestStructDuplicateKeys ts;
  ouly::yml::from_string(ts, yml);

  // Expect the last value to override
  REQUIRE(ts.a == 2);
}

struct float3
{
  float x, y, z;

  inline auto operator<=>(float3 const&) const = default;
};

template <>
struct ouly::convert<float3>
{
  // NOLINTNEXTLINE
  static auto to_type(float3 const& ref) -> std::array<float, 3>
  {
    return std::array{ref.x, ref.y, ref.z}; // NOLINT
  }

  // NOLINTNEXTLINE
  static void from_type(float3& ref, std::array<float, 3> const& value)
  {
    ref.x = value[0];
    ref.y = value[1];
    ref.z = value[2];
  }
};

TEST_CASE("yaml_object: Test read custom type")
{

  struct TestStructCustomType
  {
    float3 vec;

    inline auto operator<=>(TestStructCustomType const&) const = default;
  };

  TestStructCustomType ts;
  ts.vec.x        = 42.0f;
  ts.vec.y        = 43.0f;
  ts.vec.z        = 44.0f;
  std::string yml = ouly::yml::to_string(ts);

  TestStructCustomType ts2;
  ouly::yml::from_string(ts2, yml);

  REQUIRE(ts2.vec.x == 42.0f);
  REQUIRE(ts2.vec.y == 43.0f);
  REQUIRE(ts2.vec.z == 44.0f);
}

TEST_CASE("yaml_object: Test read complex custom type1")
{

  struct Vertex
  {
    float3      p;
    inline auto operator<=>(Vertex const&) const = default;
  };

  struct TestStructCustomType
  {
    std::vector<Vertex>   vertices;
    std::vector<uint16_t> indices;

    inline auto operator<=>(TestStructCustomType const&) const = default;
  };

  TestStructCustomType ts;
  ts.vertices.resize(2);
  ts.vertices[0].p.x = 48.0f;
  ts.vertices[0].p.y = 49.0f;
  ts.vertices[0].p.z = 50.0f;
  ts.vertices[1].p.x = 54.0f;
  ts.vertices[1].p.y = 55.0f;
  ts.vertices[1].p.z = 56.0f;
  ts.indices.resize(3);
  ts.indices[0] = 60;
  ts.indices[1] = 61;
  ts.indices[2] = 62;

  std::string yml = ouly::yml::to_string(ts);

  TestStructCustomType ts2;
  ouly::yml::from_string(ts2, yml);

  REQUIRE(ts2 == ts);
}

TEST_CASE("yaml_object: Test read complex custom type2")
{

  struct Vertex
  {
    float3 p;
    float3 uv;

    inline auto operator<=>(Vertex const&) const = default;
  };

  struct TestStructCustomType
  {
    float3                center;
    float3                bounds;
    std::vector<Vertex>   vertices;
    std::vector<uint16_t> indices;

    inline auto operator<=>(TestStructCustomType const&) const = default;
  };

  TestStructCustomType ts;
  ts.center.x = 42.0f;
  ts.center.y = 43.0f;
  ts.center.z = 44.0f;
  ts.bounds.x = 45.0f;
  ts.bounds.y = 46.0f;
  ts.bounds.z = 47.0f;
  ts.vertices.resize(2);
  ts.vertices[0].p.x  = 48.0f;
  ts.vertices[0].p.y  = 49.0f;
  ts.vertices[0].p.z  = 50.0f;
  ts.vertices[0].uv.x = 51.0f;
  ts.vertices[0].uv.y = 52.0f;
  ts.vertices[0].uv.z = 53.0f;
  ts.vertices[1].p.x  = 54.0f;
  ts.vertices[1].p.y  = 55.0f;
  ts.vertices[1].p.z  = 56.0f;
  ts.vertices[1].uv.x = 57.0f;
  ts.vertices[1].uv.y = 58.0f;
  ts.vertices[1].uv.z = 59.0f;
  ts.indices.resize(3);
  ts.indices[0] = 60;
  ts.indices[1] = 61;
  ts.indices[2] = 62;

  std::string yml = ouly::yml::to_string(ts);

  TestStructCustomType ts2;
  ouly::yml::from_string(ts2, yml);

  REQUIRE(ts2 == ts);
}

struct IntArray
{
  std::vector<uint16_t> values;
  inline auto           operator<=>(IntArray const&) const = default;
};

template <typename T>
struct ArrayOfArrays
{
  std::vector<T> items;
  inline auto    operator<=>(ArrayOfArrays<T> const&) const = default;
};

using ArrayOfIntArrays = ArrayOfArrays<IntArray>;

TEST_CASE("yaml_object: Test read array of complex struct with an empty array in the middle")
{
  ArrayOfIntArrays ts;

  ts.items.resize(3);
  ts.items[0].values = {};
  ts.items[1].values = {1, 2, 3};
  ts.items[2].values = {4, 5, 6};

  std::string yml = ouly::yml::to_string(ts);

  ArrayOfIntArrays ts2;
  ouly::yml::from_string(ts2, yml);

  REQUIRE(ts2 == ts);
}

using ArrayOfArrayOfArrayOfInts = ArrayOfArrays<ArrayOfArrays<ArrayOfArrays<IntArray>>>;

TEST_CASE("yaml_object: Test read nested array of arrays with empty array in the middle")
{
  ArrayOfArrayOfArrayOfInts ts;

  ts.items.resize(3);
  for (auto& item : ts.items)
  {
    item.items.resize(3);
    for (auto& item2 : item.items)
    {
      item2.items.resize(3);
      item2.items[0].values = {};
      item2.items[1].values = {1, 2, 3};
      item2.items[2].values = {4, 5, 6};
    }
  }

  std::string yml = ouly::yml::to_string(ts);

  ArrayOfArrayOfArrayOfInts ts2;
  ouly::yml::from_string(ts2, yml);

  REQUIRE(ts2 == ts);
}

TEST_CASE("yaml_object: Test read nested array of arrays with empty array in the middle of nested level 1")
{
  ArrayOfArrayOfArrayOfInts ts;

  ts.items.resize(3);
  ts.items[1].items.resize(3);
  ts.items[1].items[0].items.resize(3);
  ts.items[1].items[2].items.resize(3);

  std::string yml = ouly::yml::to_string(ts);

  ArrayOfArrayOfArrayOfInts ts2;
  ouly::yml::from_string(ts2, yml);

  REQUIRE(ts2 == ts);
}
// NOLINTEND