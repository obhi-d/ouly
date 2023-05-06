
#include "acl/input_serializer.hpp"
#include <catch2/catch_all.hpp>
#include <nlohmann/json.hpp>
#include <unordered_map>

using json = nlohmann::json;

struct InputData
{
  json root;
  bool fail_bit = false;
};

class Serializer
{
public:
  Serializer() noexcept = default;
  Serializer(InputData& r) : owner(r), value(std::cref(r.root)) {}
  Serializer(InputData& r, json const& source) : owner(r), value(std::cref(source)) {}

  bool is_object() const noexcept
  {
    return value.get().is_object();
  }

  bool is_array() const noexcept
  {
    return value.get().is_array();
  }

  uint32_t size() const noexcept
  {
    return static_cast<uint32_t>(value.get().size());
  }

  template <typename L>
  requires(acl::function_traits<L>::arity == 2) bool for_each(L&& lambda) const noexcept
  {
    assert(value.get().is_object());
    for (auto const& [key, value] : value.get().items())
    {
      if (!lambda(std::string_view(key), Serializer(owner, value)))
        return false;
    }
    return true;
  }

  template <typename L>
  requires(acl::function_traits<L>::arity == 1) bool for_each(L&& lambda) const noexcept
  {
    assert(value.get().is_object());
    for (auto const& value : value.get())
    {
      if (!lambda(Serializer(owner, value)))
        return false;
    }
    return true;
  }

  std::optional<Serializer> at(std::string_view name) const noexcept
  {
    auto it = value.get().find(name);
    if (it != value.get().end())
      return Serializer(owner, *it);
    return {};
  }

  std::optional<Serializer> at(uint32_t idx) const noexcept
  {
    if (idx >= value.get().size())
      return {};
    return Serializer(owner, value.get().at(idx));
  }

  auto as_double() const noexcept
  {
    return value.get().get_ptr<json::number_float_t const*>();
  }

  auto as_uint64() const noexcept
  {
    return value.get().get_ptr<json::number_unsigned_t const*>();
  }

  auto as_int64() const noexcept
  {
    return value.get().get_ptr<json::number_integer_t const*>();
  }

  auto as_bool() const noexcept
  {
    return value.get().get_ptr<json::boolean_t const*>();
  }

  auto as_string() const noexcept
  {
    return value.get().get_ptr<json::string_t const*>();
  }

  void error(std::string_view type, acl::input_error_code iec)
  {
    owner.get().fail_bit = true;
  }

private:
  std::reference_wrapper<InputData>  owner;
  std::reference_wrapper<json const> value;
};

struct MyStruct
{
  int a = 0;
  int b = 0;
};

template <>
auto acl::reflect<MyStruct>() noexcept
{
  return acl::bind(acl::bind<"a", &MyStruct::a>(), acl::bind<"b", &MyStruct::b>());
}

TEST_CASE("input_serializer: Test valid stream in with reflect outside")
{
  json j = "{ \"a\": 100, \"b\": 200 }"_json;

  InputData input;
  input.root      = j;
  auto serializer = Serializer(input);
  auto ser        = acl::input_serializer<Serializer>(serializer);

  MyStruct myStruct;

  ser(myStruct);

  REQUIRE(myStruct.a == 100);
  REQUIRE(myStruct.b == 200);
}

class MyClass
{
  int a = 0;
  int b = 1;

public:
  int get_a() const
  {
    return a;
  }
  int get_b() const
  {
    return b;
  }
  static auto reflect() noexcept
  {
    return acl::bind(acl::bind<"a", &MyClass::a>(), acl::bind<"b", &MyClass::b>());
  }
};

TEST_CASE("input_serializer: Test valid stream in with reflect member")
{
  json j = "{ \"a\": 100, \"b\": 200 }"_json;

  InputData input;
  input.root      = j;
  auto serializer = Serializer(input);
  auto ser        = acl::input_serializer<Serializer>(serializer);

  MyClass myStruct;

  ser(myStruct);

  REQUIRE(myStruct.get_a() == 100);
  REQUIRE(myStruct.get_b() == 200);
}

struct MyScopeClass
{
  MyClass first;
  MyClass second;

  static auto reflect() noexcept
  {
    return acl::bind(acl::bind<"first", &MyScopeClass::first>(), acl::bind<"second", &MyScopeClass::second>());
  }
};

TEST_CASE("input_serializer: Test 1 level scoped class")
{
  json j = R"({ "first":{ "a": 100, "b": 200 }, "second":{ "a": 300, "b": 400 } } )"_json;

  InputData input;
  input.root      = j;
  auto serializer = Serializer(input);
  auto ser        = acl::input_serializer<Serializer>(serializer);

  MyScopeClass myStruct;

  ser(myStruct);

  REQUIRE(myStruct.first.get_a() == 100);
  REQUIRE(myStruct.first.get_b() == 200);
  REQUIRE(myStruct.second.get_a() == 300);
  REQUIRE(myStruct.second.get_b() == 400);
}

struct MyScopeClass2
{
  MyScopeClass first;
  std::string  second;

  static auto reflect() noexcept
  {
    return acl::bind(acl::bind<"first", &MyScopeClass2::first>(), acl::bind<"second", &MyScopeClass2::second>());
  }
};

TEST_CASE("input_serializer: Test 2 level scoped class")
{
  json j = R"({ "first":{ "first":{ "a": 100, "b": 200 }, "second":{ "a": 300, "b": 400 } }, "second":"value" })"_json;

  InputData input;
  input.root      = j;
  auto serializer = Serializer(input);
  auto ser        = acl::input_serializer<Serializer>(serializer);

  MyScopeClass2 myStruct;

  ser(myStruct);

  REQUIRE(myStruct.first.first.get_a() == 100);
  REQUIRE(myStruct.first.first.get_b() == 200);
  REQUIRE(myStruct.first.second.get_a() == 300);
  REQUIRE(myStruct.first.second.get_b() == 400);
  REQUIRE(myStruct.second == "value");
}

TEST_CASE("input_serializer: Test pair")
{
  json j = R"([ { "first":{ "a": 100, "b": 200 }, "second":{ "a": 300, "b": 400 } }, "value" ])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Serializer(input);
  auto ser        = acl::input_serializer<Serializer>(serializer);

  std::pair<MyScopeClass, std::string> myStruct;

  ser(myStruct);

  REQUIRE(myStruct.first.first.get_a() == 100);
  REQUIRE(myStruct.first.first.get_b() == 200);
  REQUIRE(myStruct.first.second.get_a() == 300);
  REQUIRE(myStruct.first.second.get_b() == 400);
  REQUIRE(myStruct.second == "value");
}

TEST_CASE("input_serializer: Test tuple")
{
  json j = R"([ { "first":{ "a": 100, "b": 200 }, "second":{ "a": 300, "b": 400 } }, "value", 324, true ])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Serializer(input);
  auto ser        = acl::input_serializer<Serializer>(serializer);

  std::tuple<MyScopeClass, std::string, int, bool> myStruct = {};

  ser(myStruct);

  REQUIRE(std::get<0>(myStruct).first.get_a() == 100);
  REQUIRE(std::get<0>(myStruct).first.get_b() == 200);
  REQUIRE(std::get<0>(myStruct).second.get_a() == 300);
  REQUIRE(std::get<0>(myStruct).second.get_b() == 400);
  REQUIRE(std::get<1>(myStruct) == "value");
  REQUIRE(std::get<2>(myStruct) == 324);
  REQUIRE(std::get<3>(myStruct) == true);
}

TEST_CASE("input_serializer: Test name_value_pair_list")
{
  using pair_type = std::pair<int, std::string>;
  using type      = std::unordered_map<std::string, pair_type>;
  json j          = R"({ "first":[ 100, "100"], "second":[ 300, "300" ] , "third":[ 400, "400" ] })"_json;

  InputData input;
  input.root      = j;
  auto serializer = Serializer(input);
  auto ser        = acl::input_serializer<Serializer>(serializer);

  type myMap = {};

  ser(myMap);

  REQUIRE(myMap["first"] == pair_type(100, "100"));
  REQUIRE(myMap["second"] == pair_type(300, "300"));
  REQUIRE(myMap["third"] == pair_type(400, "400"));
}

template <>
int& acl::from_string<int>(int& v, std::string_view sv)
{
  std::from_chars(sv.data(), sv.data() + sv.length(), v);
  return v;
}

template <>
int acl::from_string<int>(std::string_view sv)
{
  int v;
  std::from_chars(sv.data(), sv.data() + sv.length(), v);
  return v;
}

TEST_CASE("input_serializer: Test name_value_pair_list")
{
  using pair_type = std::pair<int, std::string>;
  using type      = std::unordered_map<int, pair_type>;
  json j          = R"({ "11":[ 100, "100"], "13":[ 300, "300" ] , "15":[ 400, "400" ] })"_json;

  InputData input;
  input.root      = j;
  auto serializer = Serializer(input);
  auto ser        = acl::input_serializer<Serializer>(serializer);

  type myMap = {};

  ser(myMap);

  REQUIRE(myMap[11] == pair_type(100, "100"));
  REQUIRE(myMap[13] == pair_type(300, "300"));
  REQUIRE(myMap[15] == pair_type(400, "400"));
}
