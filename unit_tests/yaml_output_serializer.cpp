#include <acl/serializers/yaml_output_serializer.hpp>
#include <catch2/catch_all.hpp>
#include <map>
#include <memory>
#include <optional>
#include <tuple>
#include <variant>
#include <vector>

// NOLINTBEGIN
TEST_CASE("yaml_output: Test write simple struct")
{
  struct OutputTestStruct
  {
    int                   a;
    std::string           b;
    static constexpr auto reflect() noexcept
    {
      return acl::bind(acl::bind<"a", &OutputTestStruct::a>(), acl::bind<"b", &OutputTestStruct::b>());
    }
  };

  OutputTestStruct ts{100, "value"};
  auto             yaml = acl::yaml::to_string(ts);
  REQUIRE(yaml.find("a: 100") != std::string::npos);
  REQUIRE(yaml.find("b: value") != std::string::npos);
}

TEST_CASE("yaml_output: Test write nested struct")
{
  struct NestedInner
  {
    int                   a;
    static constexpr auto reflect() noexcept
    {
      return acl::bind(acl::bind<"a", &NestedInner::a>());
    }
  };

  struct NestedOuter
  {
    int                   b;
    NestedInner           inner;
    static constexpr auto reflect() noexcept
    {
      return acl::bind(acl::bind<"b", &NestedOuter::b>(), acl::bind<"inner", &NestedOuter::inner>());
    }
  };

  NestedOuter no{200, {300}};
  auto        yaml = acl::yaml::to_string(no);
  REQUIRE(yaml.find("b: 200") != std::string::npos);
  REQUIRE(yaml.find("a: 300") != std::string::npos);
}

TEST_CASE("yaml_output: Test write vector")
{
  struct VectorTest
  {
    std::vector<int>      items;
    static constexpr auto reflect() noexcept
    {
      return acl::bind(acl::bind<"items", &VectorTest::items>());
    }
  };
  VectorTest vt{
   {1, 2, 3}
  };
  auto yaml = acl::yaml::to_string(vt);
  REQUIRE(yaml.find("items: \n - 1\n - 2\n - 3") != std::string::npos);
}

TEST_CASE("yaml_output: Test write optional")
{
  struct OptionalTest
  {
    std::optional<int>    value;
    static constexpr auto reflect() noexcept
    {
      return acl::bind(acl::bind<"value", &OptionalTest::value>());
    }
  };
  OptionalTest ot{42};
  auto         yaml = acl::yaml::to_string(ot);
  REQUIRE(yaml.find("value: 42") != std::string::npos);
}

TEST_CASE("yaml_output: Test write map")
{
  struct MapTest
  {
    std::map<std::string, int> data;
    static constexpr auto      reflect() noexcept
    {
      return acl::bind(acl::bind<"data", &MapTest::data>());
    }
  };
  MapTest mt{
   {{"key1", 100}, {"key2", 200}}
  };
  auto yaml = acl::yaml::to_string(mt);
  REQUIRE(yaml.find("key1: 100") != std::string::npos);
  REQUIRE(yaml.find("key2: 200") != std::string::npos);
}

TEST_CASE("yaml_output: Test write variant")
{
  using VarType = std::variant<int, std::string>;
  struct VariantTest
  {
    VarType               var;
    static constexpr auto reflect() noexcept
    {
      return acl::bind(acl::bind<"var", &VariantTest::var>());
    }
  };
  VariantTest vt{std::string("Hello")};
  auto        yaml = acl::yaml::to_string(vt);
  REQUIRE(yaml.find("Hello") != std::string::npos);
}

TEST_CASE("yaml_output: Test write tuple")
{
  struct TupleTest
  {
    std::tuple<int, std::string, double> tup;
    static constexpr auto                reflect() noexcept
    {
      return acl::bind(acl::bind<"tup", &TupleTest::tup>());
    }
  };
  TupleTest tt{
   {10, "test", 3.14}
  };
  auto yaml = acl::yaml::to_string(tt);
  REQUIRE(yaml.find("10") != std::string::npos);
  REQUIRE(yaml.find("test") != std::string::npos);
  REQUIRE(yaml.find("3.14") != std::string::npos);
}

// NOLINTEND