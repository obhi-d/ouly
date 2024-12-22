#include <acl/dsl/yaml.hpp>
#include <acl/serializers/yaml_object.hpp>
#include <array>
#include <catch2/catch_all.hpp>
#include <map>
#include <memory>
#include <optional>
#include <tuple>
#include <variant>
#include <vector>

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
  std::string yaml = R"(
a: 100
b: 200.0
c: "value"
)";

  TestStruct ts;
  acl::yaml::read(ts, yaml);

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

TEST_CASE("yaml_object: Test read nested")
{
  std::string yaml = R"(
a: 100
b: 200.0
c: "value"
d:
  a: 300
  b: 400.0
  c: "value2"
)";
  TestStruct2 ts;
  acl::yaml::read(ts, yaml);
  REQUIRE(ts.a == 100);
  REQUIRE(ts.b == 200.0);
  REQUIRE(ts.c == "value");
  REQUIRE(ts.d.a == 300);
  REQUIRE(ts.d.b == 400.0);
  REQUIRE(ts.d.c == "value2");
}

TEST_CASE("yaml_object: Test read vector")
{
  std::string yaml = R"(
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
  acl::yaml::read(ts, yaml);

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
    acl::yaml::read(ts, yaml_present);
    REQUIRE(ts.value.has_value());
    REQUIRE(ts.value.value() == 42);
  }

  // Test with value absent
  {
    TestStructOptional ts;
    acl::yaml::read(ts, yaml_absent);
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
  std::string yaml = R"(
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
  acl::yaml::read(ts, yaml);

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
    acl::yaml::read(ts, yaml_present);
    REQUIRE(ts.value != nullptr);
    REQUIRE(*ts.value == 42);
  }

  // Test with value null
  {
    TestStructPointer ts;
    acl::yaml::read(ts, yaml_null);
    REQUIRE(ts.value == nullptr);
  }
}

TEST_CASE("yaml_object: Test read tuple")
{
  std::string yaml = R"(
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
  acl::yaml::read(ts, yaml);

  REQUIRE(std::get<0>(ts.tuple) == 1);
  REQUIRE(std::get<1>(ts.tuple) == "string");
  REQUIRE(std::get<2>(ts.tuple) == 3.14);
}

TEST_CASE("yaml_object: Test read map")
{
  std::string yaml = R"(
map:
  - key: key1
    value: 1
  - key: key2
    value: 2
  - key: key3
    value: 3
)";

  struct TestStructMap
  {
    std::map<std::string, int> map;
    static constexpr auto      reflect() noexcept
    {
      return acl::bind(acl::bind<"map", &TestStructMap::map>());
    }
  };

  TestStructMap ts;
  acl::yaml::read(ts, yaml);

  REQUIRE(ts.map.size() == 3);
  REQUIRE(ts.map["key1"] == 1);
  REQUIRE(ts.map["key2"] == 2);
  REQUIRE(ts.map["key3"] == 3);
}

TEST_CASE("yaml_object: Test read array")
{
  std::string yaml = R"(
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
  acl::yaml::read(ts, yaml);

  REQUIRE(ts.array[0] == 10);
  REQUIRE(ts.array[1] == 20);
  REQUIRE(ts.array[2] == 30);
}

TEST_CASE("yaml_object: Test read boolean")
{
  std::string yaml = R"(
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
  acl::yaml::read(ts, yaml);

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
    acl::yaml::read(ts, yaml_int);
    REQUIRE(std::holds_alternative<int>(ts.var));
    REQUIRE(std::get<int>(ts.var) == 42);
  }

  {
    TestStructVariant ts;
    acl::yaml::read(ts, yaml_string);
    REQUIRE(std::holds_alternative<std::string>(ts.var));
    REQUIRE(std::get<std::string>(ts.var) == "hello");
  }
}
