
#include "acl/serializers/input_serializer.hpp"
#include "acl/containers/array_types.hpp"
#include <catch2/catch_all.hpp>
#include <charconv>
#include <nlohmann/json.hpp>
#include <unordered_map>

using json = nlohmann::json;

struct InputData
{
  json                  root;
  acl::serializer_error ec = acl::serializer_error::none;
};

class Serializer
{
public:
  Serializer() noexcept = default;
  Serializer(InputData& r) : owner(r), value(std::cref(r.root)) {}
  Serializer(InputData& r, json const& source) : owner(r), value(std::cref(source)) {}

  bool failed() const noexcept
  {
    return false;
  }

  bool is_object() const noexcept
  {
    return value.get().is_object();
  }

  bool is_array() const noexcept
  {
    return value.get().is_array();
  }

  bool is_null() const noexcept
  {
    return value.get().is_null();
  }

  uint32_t size() const noexcept
  {
    return static_cast<uint32_t>(value.get().size());
  }

  template <typename L>
  requires(acl::function_traits<L>::arity == 2) bool for_each(L&& lambda) const noexcept
  {
    ACL_ASSERT(value.get().is_object());
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

  void error(std::string_view type, std::error_code iec)
  {
    owner.get().ec = (acl::serializer_error)iec.value();
  }

private:
  std::reference_wrapper<InputData>  owner;
  std::reference_wrapper<json const> value;
};

struct ReflTestFriend
{
  int a = 0;
  int b = 0;
};

template <>
auto acl::reflect<ReflTestFriend>() noexcept
{
  return acl::bind(acl::bind<"a", &ReflTestFriend::a>(), acl::bind<"b", &ReflTestFriend::b>());
}

TEST_CASE("input_serializer: Test valid stream in with reflect outside")
{
  json j = "{ \"a\": 100, \"b\": 200 }"_json;

  InputData input;
  input.root      = j;
  auto serializer = Serializer(input);
  auto ser        = acl::input_serializer<Serializer>(serializer);

  ReflTestFriend myStruct;

  ser(myStruct);

  REQUIRE(myStruct.a == 100);
  REQUIRE(myStruct.b == 200);
}

TEST_CASE("input_serializer: Test partial stream in with reflect outside")
{
  json j = "{ \"a\": 100 }"_json;

  InputData input;
  input.root      = j;
  auto serializer = Serializer(input);
  auto ser        = acl::input_serializer<Serializer>(serializer);

  ReflTestFriend myStruct;

  ser(myStruct);

  REQUIRE(myStruct.a == 100);
  REQUIRE(myStruct.b == 0);
}

TEST_CASE("input_serializer: Test fail stream in with reflect outside")
{
  json j = "{ \"a\": \"is_string\" }"_json;

  InputData input;
  input.root      = j;
  auto serializer = Serializer(input);
  auto ser        = acl::input_serializer<Serializer>(serializer);

  ReflTestFriend myStruct;

  ser(myStruct);

  REQUIRE(input.ec == acl::serializer_error::failed_to_parse_value);
  REQUIRE(myStruct.a == 0);
  REQUIRE(myStruct.b == 0);
}

class ReflTestClass
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
    return acl::bind(acl::bind<"a", &ReflTestClass::a>(), acl::bind<"b", &ReflTestClass::b>());
  }
};

TEST_CASE("input_serializer: Test valid stream in with reflect member")
{
  json j = "{ \"a\": 100, \"b\": 200 }"_json;

  InputData input;
  input.root      = j;
  auto serializer = Serializer(input);
  auto ser        = acl::input_serializer<Serializer>(serializer);

  ReflTestClass myStruct;

  ser(myStruct);

  REQUIRE(myStruct.get_a() == 100);
  REQUIRE(myStruct.get_b() == 200);
}

struct ReflTestMember
{
  ReflTestClass first;
  ReflTestClass second;

  static auto reflect() noexcept
  {
    return acl::bind(acl::bind<"first", &ReflTestMember::first>(), acl::bind<"second", &ReflTestMember::second>());
  }
};

TEST_CASE("input_serializer: Test 1 level scoped class")
{
  json j = R"({ "first":{ "a": 100, "b": 200 }, "second":{ "a": 300, "b": 400 } } )"_json;

  InputData input;
  input.root      = j;
  auto serializer = Serializer(input);
  auto ser        = acl::input_serializer<Serializer>(serializer);

  ReflTestMember myStruct;

  ser(myStruct);

  REQUIRE(myStruct.first.get_a() == 100);
  REQUIRE(myStruct.first.get_b() == 200);
  REQUIRE(myStruct.second.get_a() == 300);
  REQUIRE(myStruct.second.get_b() == 400);
}

TEST_CASE("input_serializer: Test partial 1 level scoped class")
{
  json j = R"({ "first":{ "a": 100, "b": 200 } } )"_json;

  InputData input;
  input.root      = j;
  auto serializer = Serializer(input);
  auto ser        = acl::input_serializer<Serializer>(serializer);

  ReflTestMember myStruct;

  ser(myStruct);

  REQUIRE(myStruct.first.get_a() == 100);
  REQUIRE(myStruct.first.get_b() == 200);
  REQUIRE(myStruct.second.get_a() == 0);
  REQUIRE(myStruct.second.get_b() == 1);
}

struct ReflTestClass2
{
  ReflTestMember first;
  std::string    second;

  static auto reflect() noexcept
  {
    return acl::bind(acl::bind<"first", &ReflTestClass2::first>(), acl::bind<"second", &ReflTestClass2::second>());
  }
};

TEST_CASE("input_serializer: Test 2 level scoped class")
{
  json j = R"({ "first":{ "first":{ "a": 100, "b": 200 }, "second":{ "a": 300, "b": 400 } }, "second":"value" })"_json;

  InputData input;
  input.root      = j;
  auto serializer = Serializer(input);
  auto ser        = acl::input_serializer<Serializer>(serializer);

  ReflTestClass2 myStruct;

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

  std::pair<ReflTestMember, std::string> myStruct;

  ser(myStruct);

  REQUIRE(myStruct.first.first.get_a() == 100);
  REQUIRE(myStruct.first.first.get_b() == 200);
  REQUIRE(myStruct.first.second.get_a() == 300);
  REQUIRE(myStruct.first.second.get_b() == 400);
  REQUIRE(myStruct.second == "value");
}

TEST_CASE("input_serializer: TupleLike ")
{
  json j = R"([ { "first":{ "a": 100, "b": 200 }, "second":{ "a": 300, "b": 400 } }, "value", 324, true ])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Serializer(input);
  auto ser        = acl::input_serializer<Serializer>(serializer);

  std::tuple<ReflTestMember, std::string, int, bool> myStruct = {};

  ser(myStruct);

  REQUIRE(std::get<0>(myStruct).first.get_a() == 100);
  REQUIRE(std::get<0>(myStruct).first.get_b() == 200);
  REQUIRE(std::get<0>(myStruct).second.get_a() == 300);
  REQUIRE(std::get<0>(myStruct).second.get_b() == 400);
  REQUIRE(std::get<1>(myStruct) == "value");
  REQUIRE(std::get<2>(myStruct) == 324);
  REQUIRE(std::get<3>(myStruct) == true);
}

TEST_CASE("input_serializer: Invalid TupleLike ")
{
  json j = R"({ "first": "invalid" })"_json;

  InputData input;
  input.root      = j;
  auto serializer = Serializer(input);
  auto ser        = acl::input_serializer<Serializer>(serializer);

  std::tuple<ReflTestMember, std::string, int, bool> myStruct = {};

  ser(myStruct);

  REQUIRE(input.ec == acl::serializer_error::invalid_type);
}

TEST_CASE("input_serializer: StringMapLike ")
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

TEST_CASE("input_serializer: StringMapLike Invalid ")
{
  using pair_type = std::pair<int, std::string>;
  using type      = std::unordered_map<std::string, pair_type>;
  json j          = R"({ "first":"invalid", "second":[ 300, "300" ] , "third":[ 400, "400" ] })"_json;

  InputData input;
  input.root      = j;
  auto serializer = Serializer(input);
  auto ser        = acl::input_serializer<Serializer>(serializer);

  type myMap = {};

  ser(myMap);

  REQUIRE(input.ec == acl::serializer_error::failed_streaming_map);
  REQUIRE(myMap.empty());
}

TEST_CASE("input_serializer: StringMapLike Invalid  Subelement")
{
  using pair_type = std::pair<int, std::string>;
  using type      = std::unordered_map<std::string, pair_type>;
  json j          = R"([ "invalid", [ 300, "300" ] , [ 400, "400" ] ])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Serializer(input);
  auto ser        = acl::input_serializer<Serializer>(serializer);

  type myMap = {};

  ser(myMap);

  REQUIRE(input.ec == acl::serializer_error::invalid_type);
  REQUIRE(myMap.empty());
}

TEST_CASE("input_serializer: ArrayLike")
{
  using pair_type = std::pair<int, std::string>;
  using type      = std::unordered_map<int, pair_type>;
  json j          = R"([ [11, [ 100, "100"]], [13, [ 300, "300" ]] , [15, [ 400, "400" ]] ])"_json;

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

TEST_CASE("input_serializer: ArrayLike (no emplace)")
{
  acl::dynamic_array<int> myArray;
  json                    j = R"([ 11, 100, 13, 300 ])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Serializer(input);
  auto ser        = acl::input_serializer<Serializer>(serializer);

  ser(myArray);

  REQUIRE(myArray.size() == 4);
  REQUIRE(myArray[0] == 11);
  REQUIRE(myArray[1] == 100);
  REQUIRE(myArray[2] == 13);
  REQUIRE(myArray[3] == 300);
}

TEST_CASE("input_serializer: ArrayLike Invalid ")
{
  acl::dynamic_array<int> myArray;
  json                    j = R"({ })"_json;

  InputData input;
  input.root      = j;
  auto serializer = Serializer(input);
  auto ser        = acl::input_serializer<Serializer>(serializer);

  ser(myArray);

  REQUIRE(myArray.empty());
  REQUIRE(input.ec == acl::serializer_error::invalid_type);
}

TEST_CASE("input_serializer: ArrayLike (no emplace) Invalid Subelement ")
{
  acl::dynamic_array<int> myArray;
  json                    j = R"([ "string", 100, 13, 300 ])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Serializer(input);
  auto ser        = acl::input_serializer<Serializer>(serializer);

  ser(myArray);

  REQUIRE(myArray.empty());
  REQUIRE(input.ec == acl::serializer_error::failed_streaming_array);
}

TEST_CASE("input_serializer: ArrayLike Invalid Subelement ")
{
  using pair_type = std::pair<int, std::string>;
  using type      = std::unordered_map<int, pair_type>;
  json j          = R"([ [11, [ 100, 100]], [13, [ 300, "300" ]] , [15, [ 400, "400" ]] ])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Serializer(input);
  auto ser        = acl::input_serializer<Serializer>(serializer);

  type myMap = {};

  ser(myMap);

  REQUIRE(myMap.empty());
  REQUIRE(input.ec == acl::serializer_error::failed_streaming_array);
}

TEST_CASE("input_serializer: VariantLike ")
{
  std::vector<std::variant<int, bool, std::string>> variantList;

  json j = R"([ [0, 100 ], [1, true], [2, "100" ], [ 1, false ] ])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Serializer(input);
  auto ser        = acl::input_serializer<Serializer>(serializer);

  ser(variantList);

  REQUIRE(variantList.size() == 4);
  REQUIRE(std::holds_alternative<int>(variantList[0]));
  REQUIRE(std::holds_alternative<bool>(variantList[1]));
  REQUIRE(std::holds_alternative<std::string>(variantList[2]));
  REQUIRE(std::holds_alternative<bool>(variantList[3]));
  REQUIRE(std::get<int>(variantList[0]) == 100);
  REQUIRE(std::get<bool>(variantList[1]) == true);
  REQUIRE(std::get<std::string>(variantList[2]) == "100");
  REQUIRE(std::get<bool>(variantList[3]) == false);
}

TEST_CASE("input_serializer: VariantLike Invalid")
{
  std::variant<int, bool, std::string> variant;

  json j = R"([ "value", "100" ])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Serializer(input);
  auto ser        = acl::input_serializer<Serializer>(serializer);

  ser(variant);

  REQUIRE(variant.index() == 0);
  REQUIRE(input.ec == acl::serializer_error::variant_index_is_not_int);
}

TEST_CASE("input_serializer: VariantLike Invalid Type")
{
  std::variant<int, bool, std::string> variant;

  json j = R"({ "value": "100" })"_json;

  InputData input;
  input.root      = j;
  auto serializer = Serializer(input);
  auto ser        = acl::input_serializer<Serializer>(serializer);

  ser(variant);

  REQUIRE(variant.index() == 0);
  REQUIRE(input.ec == acl::serializer_error::invalid_type);
}

TEST_CASE("input_serializer: VariantLike Invalid Size")
{
  std::variant<int, bool, std::string> variant;

  json j = R"([ 0, "value", "100" ])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Serializer(input);
  auto ser        = acl::input_serializer<Serializer>(serializer);

  ser(variant);

  REQUIRE(variant.index() == 0);
  REQUIRE(input.ec == acl::serializer_error::variant_invalid_format);
}

struct ConstructedSV
{
  int id                   = -1;
  ConstructedSV() noexcept = default;
  explicit ConstructedSV(std::string_view sv)
  {
    std::from_chars(sv.data(), sv.data() + sv.length(), id);
  }
};

TEST_CASE("input_serializer: ConstructedFromStringView")
{
  acl::dynamic_array<ConstructedSV> myArray;
  json                              j = R"([ "11", "100", "13", "300" ])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Serializer(input);
  auto ser        = acl::input_serializer<Serializer>(serializer);

  ser(myArray);

  REQUIRE(myArray.size() == 4);
  REQUIRE(myArray[0].id == 11);
  REQUIRE(myArray[1].id == 100);
  REQUIRE(myArray[2].id == 13);
  REQUIRE(myArray[3].id == 300);
}

TEST_CASE("input_serializer: ConstructedFromStringView Invalid")
{
  acl::dynamic_array<ConstructedSV> myArray;
  json                              j = R"([ 11, "100", "13", "300" ])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Serializer(input);
  auto ser        = acl::input_serializer<Serializer>(serializer);

  ser(myArray);

  REQUIRE(myArray.empty());
  REQUIRE(input.ec == acl::serializer_error::failed_streaming_array);
}

struct TransformSV
{
  int id                 = -1;
  TransformSV() noexcept = default;
};

template <>
TransformSV& acl::from_string<TransformSV>(TransformSV& r, std::string_view sv)
{
  std::from_chars(sv.data(), sv.data() + sv.length(), r.id);
  return r;
}

template <>
TransformSV acl::from_string<TransformSV>(std::string_view sv)
{
  TransformSV r;
  std::from_chars(sv.data(), sv.data() + sv.length(), r.id);
  return r;
}

TEST_CASE("input_serializer: TransformFromString")
{
  acl::dynamic_array<TransformSV> myArray;
  json                            j = R"([ "11", "100", "13", "300" ])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Serializer(input);
  auto ser        = acl::input_serializer<Serializer>(serializer);

  ser(myArray);

  REQUIRE(myArray.size() == 4);
  REQUIRE(myArray[0].id == 11);
  REQUIRE(myArray[1].id == 100);
  REQUIRE(myArray[2].id == 13);
  REQUIRE(myArray[3].id == 300);
}

TEST_CASE("input_serializer: TransformFromString Invalid")
{
  acl::dynamic_array<TransformSV> myArray;
  json                            j = R"([ 11, "100", "13", "300" ])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Serializer(input);
  auto ser        = acl::input_serializer<Serializer>(serializer);

  ser(myArray);

  REQUIRE(myArray.size() == 0);
  REQUIRE(input.ec == acl::serializer_error::failed_streaming_array);
}

TEST_CASE("input_serializer: BoolLike")
{
  std::array<bool, 4> myArray = {};
  json                j       = R"([ false, true, false, true ])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Serializer(input);
  auto ser        = acl::input_serializer<Serializer>(serializer);

  ser(myArray);

  REQUIRE(myArray.size() == 4);
  REQUIRE(myArray[0] == false);
  REQUIRE(myArray[1] == true);
  REQUIRE(myArray[2] == false);
  REQUIRE(myArray[3] == true);
}

TEST_CASE("input_serializer: BoolLike Invaild")
{
  std::array<bool, 4> myArray = {};
  json                j       = R"([ 1, true, false, true ])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Serializer(input);
  auto ser        = acl::input_serializer<Serializer>(serializer);

  ser(myArray);

  REQUIRE(input.ec == acl::serializer_error::failed_streaming_array);
}

TEST_CASE("input_serializer: SignedIntLike")
{
  std::array<int, 4> myArray = {};
  json               j       = R"([ -40, -10, 10, 40 ])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Serializer(input);
  auto ser        = acl::input_serializer<Serializer>(serializer);

  ser(myArray);

  REQUIRE(myArray[0] == -40);
  REQUIRE(myArray[1] == -10);
  REQUIRE(myArray[2] == 10);
  REQUIRE(myArray[3] == 40);
}

TEST_CASE("input_serializer: SignedIntLike Invalid")
{
  std::array<int, 4> myArray = {};
  json               j       = R"([ "-40", -10, 10, 40 ])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Serializer(input);
  auto ser        = acl::input_serializer<Serializer>(serializer);

  ser(myArray);

  REQUIRE(input.ec == acl::serializer_error::failed_streaming_array);
}

TEST_CASE("input_serializer: UnsignedIntLike")
{
  std::array<uint32_t, 4> myArray = {};
  json                    j       = R"([ 40, 10, 10, 40 ])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Serializer(input);
  auto ser        = acl::input_serializer<Serializer>(serializer);

  ser(myArray);

  REQUIRE(myArray[0] == 40);
  REQUIRE(myArray[1] == 10);
  REQUIRE(myArray[2] == 10);
  REQUIRE(myArray[3] == 40);
}

TEST_CASE("input_serializer: UnsignedIntLike Invalid")
{
  std::array<uint32_t, 4> myArray = {};
  json                    j       = R"([ true, 10, 10, 40 ])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Serializer(input);
  auto ser        = acl::input_serializer<Serializer>(serializer);

  ser(myArray);

  REQUIRE(input.ec == acl::serializer_error::failed_streaming_array);
}

TEST_CASE("input_serializer: FloatLike")
{
  std::array<float, 4> myArray = {};
  json                 j       = R"([ 434.442, 757.10, 10.745, 424.40 ])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Serializer(input);
  auto ser        = acl::input_serializer<Serializer>(serializer);

  ser(myArray);

  REQUIRE(myArray[0] == Catch::Approx(434.442f));
  REQUIRE(myArray[1] == Catch::Approx(757.10f));
  REQUIRE(myArray[2] == Catch::Approx(10.745f));
  REQUIRE(myArray[3] == Catch::Approx(424.40f));
}

TEST_CASE("input_serializer: FloatLike Invalid")
{
  std::array<float, 4> myArray = {};
  json                 j       = R"([ 434, 757.10, 10.745, 424.40 ])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Serializer(input);
  auto ser        = acl::input_serializer<Serializer>(serializer);

  ser(myArray);

  REQUIRE(input.ec == acl::serializer_error::failed_streaming_array);
}

TEST_CASE("input_serializer: PointerLike")
{
  struct pointer
  {
    std::shared_ptr<std::string> a;
    std::unique_ptr<std::string> b;
    std::string*                 c = nullptr;

    static auto reflect() noexcept
    {
      return acl::bind(acl::bind<"a", &pointer::a>(), acl::bind<"b", &pointer::b>(), acl::bind<"c", &pointer::c>());
    }
  };

  pointer pvalue;
  json    j = R"({ "a":"A_value", "b":"B_value", "c":"C_value" })"_json;

  InputData input;
  input.root      = j;
  auto serializer = Serializer(input);
  auto ser        = acl::input_serializer<Serializer>(serializer);

  ser(pvalue);

  REQUIRE(pvalue.a);
  REQUIRE(pvalue.b);
  REQUIRE(pvalue.c);
  REQUIRE(*pvalue.a == "A_value");
  REQUIRE(*pvalue.b == "B_value");
  REQUIRE(*pvalue.c == "C_value");

  delete pvalue.c;
}

TEST_CASE("input_serializer: PointerLike Null")
{
  struct pointer
  {
    std::shared_ptr<std::string> a;
    std::unique_ptr<std::string> b;
    std::string*                 c = nullptr;

    static auto reflect() noexcept
    {
      return acl::bind(acl::bind<"a", &pointer::a>(), acl::bind<"b", &pointer::b>(), acl::bind<"c", &pointer::c>());
    }
  };

  pointer pvalue;
  json    j = R"({ "a":null, "b":null, "c":null })"_json;

  InputData input;
  input.root      = j;
  auto serializer = Serializer(input);
  auto ser        = acl::input_serializer<Serializer>(serializer);

  ser(pvalue);

  REQUIRE(!pvalue.a);
  REQUIRE(!pvalue.b);
  REQUIRE(!pvalue.c);
}

TEST_CASE("input_serializer: OptionalLike")
{
  struct pointer
  {
    std::optional<std::string> a;
    std::optional<std::string> b;

    static auto reflect() noexcept
    {
      return acl::bind(acl::bind<"a", &pointer::a>(), acl::bind<"b", &pointer::b>());
    }
  };

  pointer pvalue;
  json    j = R"({ "a":"A_value", "b":null })"_json;

  InputData input;
  input.root      = j;
  auto serializer = Serializer(input);
  auto ser        = acl::input_serializer<Serializer>(serializer);

  ser(pvalue);

  REQUIRE(pvalue.a);
  REQUIRE(!pvalue.b);
  REQUIRE(*pvalue.a == "A_value");
}

class CustomClass
{
public:
  friend Serializer& operator>>(Serializer& ser, CustomClass& cc)
  {
    cc.value = (int)(*ser.as_int64());
    return ser;
  }

  inline int get() const
  {
    return value;
  }

private:
  int value;
};

TEST_CASE("input_serializer: InputSerializableClass")
{
  std::vector<CustomClass> integers;
  json                     j = R"([ 34, 542, 234 ])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Serializer(input);
  auto ser        = acl::input_serializer<Serializer>(serializer);

  ser(integers);

  REQUIRE(integers.size() == 3);
  REQUIRE(integers[0].get() == 34);
  REQUIRE(integers[1].get() == 542);
  REQUIRE(integers[2].get() == 234);
}
