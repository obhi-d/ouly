#include "catch2/catch_all.hpp"
#include "ouly/reflection/detail/base_concepts.hpp"
#include "ouly/serializers/lite_yml.hpp"
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
      return ouly::bind(ouly::bind<"a", &OutputTestStruct::a>(), ouly::bind<"b", &OutputTestStruct::b>());
    }
  };

  OutputTestStruct ts{100, "value"};
  auto             yml = ouly::yml::to_string(ts);
  REQUIRE(yml.find("a: 100") != std::string::npos);
  REQUIRE(yml.find("b: value") != std::string::npos);
}

TEST_CASE("yaml_output: Test write nested struct")
{
  struct NestedInner
  {
    int                   a;
    static constexpr auto reflect() noexcept
    {
      return ouly::bind(ouly::bind<"a", &NestedInner::a>());
    }
  };

  struct NestedOuter
  {
    int                   b;
    NestedInner           inner;
    static constexpr auto reflect() noexcept
    {
      return ouly::bind(ouly::bind<"b", &NestedOuter::b>(), ouly::bind<"inner", &NestedOuter::inner>());
    }
  };

  NestedOuter no{200, {300}};
  auto        yml = ouly::yml::to_string(no);
  REQUIRE(yml.find("b: 200") != std::string::npos);
  REQUIRE(yml.find("a: 300") != std::string::npos);
}

TEST_CASE("yaml_output: Test write vector")
{
  struct VectorTest
  {
    std::vector<int>      items;
    static constexpr auto reflect() noexcept
    {
      return ouly::bind(ouly::bind<"items", &VectorTest::items>());
    }
  };
  VectorTest vt{
   {1, 2, 3}
  };
  auto yml = ouly::yml::to_string(vt);
  REQUIRE(yml.find("items: \n - 1\n - 2\n - 3") != std::string::npos);
}

TEST_CASE("yaml_output: Test write optional")
{
  struct OptionalTest
  {
    std::optional<int>    value;
    static constexpr auto reflect() noexcept
    {
      return ouly::bind(ouly::bind<"value", &OptionalTest::value>());
    }
  };
  OptionalTest ot{42};
  auto         yml = ouly::yml::to_string(ot);
  REQUIRE(yml.find("value: 42") != std::string::npos);
}

TEST_CASE("yaml_output: Test write variant")
{
  using VarType = std::variant<int, std::string>;
  struct VariantTest
  {
    VarType               var;
    static constexpr auto reflect() noexcept
    {
      return ouly::bind(ouly::bind<"var", &VariantTest::var>());
    }
  };
  VariantTest vt{std::string("Hello")};
  auto        yml = ouly::yml::to_string(vt);
  REQUIRE(yml.find("Hello") != std::string::npos);
}

TEST_CASE("yaml_output: Test write tuple")
{
  struct TupleTest
  {
    std::tuple<int, std::string, double> tup;
    static constexpr auto                reflect() noexcept
    {
      return ouly::bind(ouly::bind<"tup", &TupleTest::tup>());
    }
  };
  TupleTest tt{
   {10, "test", 3.14}
  };
  auto yml = ouly::yml::to_string(tt);
  REQUIRE(yml.find("10") != std::string::npos);
  REQUIRE(yml.find("test") != std::string::npos);
  REQUIRE(yml.find("3.14") != std::string::npos);
}

TEST_CASE("yaml_output: Test write map")
{
  struct MapTest
  {
    std::map<std::string, int> m;
    static constexpr auto      reflect() noexcept
    {
      return ouly::bind(ouly::bind<"m", &MapTest::m>());
    }
  };
  MapTest mt{
   {{"key1", 100}, {"key2", 200}, {"key3", 300}}
  };
  auto yml = ouly::yml::to_string(mt);

  REQUIRE(yml.find("- - key1\n   - 100") != std::string::npos);
  REQUIRE(yml.find("- - key2\n   - 200") != std::string::npos);
  REQUIRE(yml.find("- - key3\n   - 300") != std::string::npos);
}

TEST_CASE("yaml_output: Test write empty array")
{
  struct ArrayTest
  {
    std::array<int, 3> a = {0, 0, 0};
  };
  ArrayTest at;
  auto      yml = ouly::yml::to_string(at);
  REQUIRE(yml.find("- ") != std::string::npos);
}
// NOLINTEND