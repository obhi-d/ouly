
#include "acl/containers/array_types.hpp"
#include "acl/reflection/reflection.hpp"
#include "acl/serializers/serializers.hpp"
#include "acl/utility/optional_ref.hpp"
#include <catch2/catch_all.hpp>
#include <charconv>
#include <nlohmann/json.hpp>
#include <unordered_map>

using json = nlohmann::json;

// NOLINTBEGIN
struct InputData
{
  json root;
};

class Stream
{
public:
  Stream() noexcept = default;
  Stream(InputData& r) : owner(r), value(std::cref(r.root)) {}
  Stream(InputData& r, json const& source) : owner(r), value(std::cref(source)) {}

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
  void for_each_entry(L&& lambda) const
  {
    for (auto const& value : value.get())
    {
      auto stream = Stream(owner, value);
      lambda(stream);
    }
  }

  std::optional<Stream> at(std::string_view name) const noexcept
  {
    auto it = value.get().find(name);
    if (it != value.get().end())
      return Stream(owner, *it);
    return {};
  }

  std::optional<Stream> at(std::size_t index) const noexcept
  {
    if (index < value.get().size())
    {
      return Stream(owner, value.get().at(index));
    }
    return {};
  }

  auto as_double() const noexcept
  {
    return acl::optional_ref(value.get().get_ptr<json::number_float_t const*>());
  }

  auto as_uint64() const noexcept
  {
    return acl::optional_ref(value.get().get_ptr<json::number_unsigned_t const*>());
  }

  auto as_int64() const noexcept
  {
    return acl::optional_ref(value.get().get_ptr<json::number_integer_t const*>());
  }

  auto as_bool() const noexcept
  {
    return acl::optional_ref(value.get().get_ptr<json::boolean_t const*>());
  }

  auto as_string() const noexcept
  {
    return acl::optional_ref(value.get().get_ptr<json::string_t const*>());
  }

private:
  std::reference_wrapper<InputData>  owner;
  std::reference_wrapper<json const> value;
};

enum class EnumTest
{
  value0 = 323,
  value1 = 43535,
  value3 = 64533
};

struct ReflTestFriend
{
  int      a  = 0;
  int      b  = 0;
  EnumTest et = EnumTest::value0;
};

template <>
constexpr auto acl::reflect<ReflTestFriend>() noexcept
{
  return acl::bind(acl::bind<"a", &ReflTestFriend::a>(), acl::bind<"b", &ReflTestFriend::b>(),
                   acl::bind<"et", &ReflTestFriend::et>());
}

TEST_CASE("structured_input_serializer: Test valid stream in with reflect outside")
{
  json j = "{ \"a\": 100, \"b\": 200, \"et\": 64533 }"_json;

  InputData input;
  input.root                = j;
  auto           serializer = Stream(input);
  ReflTestFriend myStruct;
  acl::read(serializer, myStruct);

  REQUIRE(myStruct.a == 100);
  REQUIRE(myStruct.b == 200);
  REQUIRE(myStruct.et == EnumTest::value3);
}

TEST_CASE("structured_input_serializer: Test partial stream in with reflect outside")
{
  json j = "{ \"a\": 100 }"_json;

  InputData input;
  input.root      = j;
  auto serializer = Stream(input);

  ReflTestFriend myStruct;

  acl::read(serializer, myStruct);

  REQUIRE(myStruct.a == 100);
  REQUIRE(myStruct.b == 0);
}

TEST_CASE("structured_input_serializer: Test fail stream in with reflect outside")
{
  json j = "{ \"a\": \"is_string\" }"_json;

  InputData input;
  input.root      = j;
  auto serializer = Stream(input);

  ReflTestFriend myStruct;

  REQUIRE_THROWS(acl::read(serializer, myStruct));
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

TEST_CASE("structured_input_serializer: Test valid stream in with reflect member")
{
  json j = "{ \"a\": 100, \"b\": 200 }"_json;

  InputData input;
  input.root      = j;
  auto serializer = Stream(input);

  ReflTestClass myStruct;

  acl::read(serializer, myStruct);

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

TEST_CASE("structured_input_serializer: Test 1 level scoped class")
{
  json j = R"({ "first":{ "a": 100, "b": 200 }, "second":{ "a": 300, "b": 400 } } )"_json;

  InputData input;
  input.root      = j;
  auto serializer = Stream(input);

  ReflTestMember myStruct;

  acl::read(serializer, myStruct);

  REQUIRE(myStruct.first.get_a() == 100);
  REQUIRE(myStruct.first.get_b() == 200);
  REQUIRE(myStruct.second.get_a() == 300);
  REQUIRE(myStruct.second.get_b() == 400);
}

TEST_CASE("structured_input_serializer: Test partial 1 level scoped class")
{
  json j = R"({ "first":{ "a": 100, "b": 200 } } )"_json;

  InputData input;
  input.root      = j;
  auto serializer = Stream(input);

  ReflTestMember myStruct;

  acl::read(serializer, myStruct);

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

TEST_CASE("structured_input_serializer: Test 2 level scoped class")
{
  json j = R"({ "first":{ "first":{ "a": 100, "b": 200 }, "second":{ "a": 300, "b": 400 } }, "second":"value" })"_json;

  InputData input;
  input.root      = j;
  auto serializer = Stream(input);

  ReflTestClass2 myStruct;

  acl::read(serializer, myStruct);

  REQUIRE(myStruct.first.first.get_a() == 100);
  REQUIRE(myStruct.first.first.get_b() == 200);
  REQUIRE(myStruct.first.second.get_a() == 300);
  REQUIRE(myStruct.first.second.get_b() == 400);
  REQUIRE(myStruct.second == "value");
}

TEST_CASE("structured_input_serializer: Test pair")
{
  json j = R"([ { "first":{ "a": 100, "b": 200 }, "second":{ "a": 300, "b": 400 } }, "value" ])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Stream(input);

  std::pair<ReflTestMember, std::string> myStruct;

  acl::read(serializer, myStruct);

  REQUIRE(myStruct.first.first.get_a() == 100);
  REQUIRE(myStruct.first.first.get_b() == 200);
  REQUIRE(myStruct.first.second.get_a() == 300);
  REQUIRE(myStruct.first.second.get_b() == 400);
  REQUIRE(myStruct.second == "value");
}

TEST_CASE("structured_input_serializer: TupleLike ")
{
  json j = R"([ { "first":{ "a": 100, "b": 200 }, "second":{ "a": 300, "b": 400 } }, "value", 324, true ])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Stream(input);

  std::tuple<ReflTestMember, std::string, int, bool> myStruct = {};

  acl::read(serializer, myStruct);

  REQUIRE(std::get<0>(myStruct).first.get_a() == 100);
  REQUIRE(std::get<0>(myStruct).first.get_b() == 200);
  REQUIRE(std::get<0>(myStruct).second.get_a() == 300);
  REQUIRE(std::get<0>(myStruct).second.get_b() == 400);
  REQUIRE(std::get<1>(myStruct) == "value");
  REQUIRE(std::get<2>(myStruct) == 324);
  REQUIRE(std::get<3>(myStruct) == true);
}

TEST_CASE("structured_input_serializer: Invalid TupleLike ")
{
  json j = R"({ "first": "invalid" })"_json;

  InputData input;
  input.root      = j;
  auto serializer = Stream(input);

  std::tuple<ReflTestMember, std::string, int, bool> myStruct = {};

  REQUIRE_THROWS(acl::read(serializer, myStruct));
}

TEST_CASE("structured_input_serializer: ArrayLike (no emplace)")
{
  acl::dynamic_array<int> myArray;
  json                    j = R"([ 11, 100, 13, 300 ])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Stream(input);

  acl::read(serializer, myArray);

  REQUIRE(myArray.size() == 4);
  REQUIRE(myArray[0] == 11);
  REQUIRE(myArray[1] == 100);
  REQUIRE(myArray[2] == 13);
  REQUIRE(myArray[3] == 300);
}

TEST_CASE("structured_input_serializer: ArrayLike Invalid ")
{
  acl::dynamic_array<int> myArray;
  json                    j = R"({ })"_json;

  InputData input;
  input.root      = j;
  auto serializer = Stream(input);

  REQUIRE_THROWS(acl::read(serializer, myArray));

  REQUIRE(myArray.empty());
}

TEST_CASE("structured_input_serializer: ArrayLike (no emplace) Invalid Subelement ")
{
  acl::dynamic_array<int> myArray;
  json                    j = R"([ "string", 100, 13, 300 ])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Stream(input);

  REQUIRE_THROWS(acl::read(serializer, myArray));

  REQUIRE(myArray.empty());
}

TEST_CASE("structured_input_serializer: VariantLike ")
{
  std::vector<std::variant<int, bool, std::string>> variantList;

  json j =
   R"([ {"type":0, "value":100 }, {"type":1, "value":true}, {"type":2, "value":"100" }, { "type":1, "value":false } ])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Stream(input);

  acl::read(serializer, variantList);

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

TEST_CASE("structured_input_serializer: VariantLike Invalid")
{
  std::variant<int, bool, std::string> variant;

  json j = R"({ "type":"value", "value":"100" })"_json;

  InputData input;
  input.root      = j;
  auto serializer = Stream(input);

  REQUIRE_THROWS(acl::read(serializer, variant));
}

TEST_CASE("structured_input_serializer: VariantLike Missing Type")
{
  std::variant<int, bool, std::string> variant;

  json j = R"({ "value": "100" })"_json;

  InputData input;
  input.root      = j;
  auto serializer = Stream(input);

  REQUIRE_THROWS(acl::read(serializer, variant));
}

TEST_CASE("structured_input_serializer: VariantLike Missing Value")
{
  std::variant<int, bool, std::string> variant;

  json j = R"({ "type": 1 })"_json;

  InputData input;
  input.root      = j;
  auto serializer = Stream(input);

  REQUIRE_THROWS(acl::read(serializer, variant));
}

TEST_CASE("structured_input_serializer: VariantLike Invalid Type")
{
  std::variant<int, bool, std::string> variant;

  json j = R"([ 0, "value", "100" ])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Stream(input);

  REQUIRE_THROWS(acl::read(serializer, variant));
}

struct ConstructedSV
{
  int id                   = -1;
  ConstructedSV() noexcept = default;
  explicit ConstructedSV(std::string_view sv)
  {
    std::from_chars(sv.data(), sv.data() + sv.length(), id);
  }

  inline explicit operator std::string() const noexcept
  {
    return std::to_string(id);
  }
};

TEST_CASE("structured_input_serializer: ConstructedFromStringView")
{
  acl::dynamic_array<ConstructedSV> myArray;
  json                              j = R"([ "11", "100", "13", "300" ])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Stream(input);

  acl::read(serializer, myArray);

  REQUIRE(myArray.size() == 4);
  REQUIRE(myArray[0].id == 11);
  REQUIRE(myArray[1].id == 100);
  REQUIRE(myArray[2].id == 13);
  REQUIRE(myArray[3].id == 300);
}

TEST_CASE("structured_input_serializer: ConstructedFromStringView Invalid")
{
  acl::dynamic_array<ConstructedSV> myArray;
  json                              j = R"([ 11, "100", "13", "300" ])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Stream(input);

  REQUIRE_THROWS(acl::read(serializer, myArray));

  REQUIRE(myArray.empty());
}

struct TransformSV
{
  int id                 = -1;
  TransformSV() noexcept = default;
};

template <>
struct acl::convert<TransformSV>
{
  static std::string to_string(TransformSV const& r)
  {
    return std::to_string(r.id);
  }

  static void from_string(TransformSV& r, std::string_view sv)
  {
    std::from_chars(sv.data(), sv.data() + sv.length(), r.id);
  }
};

TEST_CASE("structured_input_serializer: TransformFromString")
{
  acl::dynamic_array<TransformSV> myArray;
  json                            j = R"([ "11", "100", "13", "300" ])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Stream(input);

  acl::read(serializer, myArray);

  REQUIRE(myArray.size() == 4);
  REQUIRE(myArray[0].id == 11);
  REQUIRE(myArray[1].id == 100);
  REQUIRE(myArray[2].id == 13);
  REQUIRE(myArray[3].id == 300);
}

TEST_CASE("structured_input_serializer: TransformFromString Invalid")
{
  acl::dynamic_array<TransformSV> myArray;
  json                            j = R"([ 11, "100", "13", "300" ])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Stream(input);

  REQUIRE_THROWS(acl::read(serializer, myArray));
}

TEST_CASE("structured_input_serializer: BoolLike")
{
  std::array<bool, 4> myArray = {};
  json                j       = R"([ false, true, false, true ])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Stream(input);

  acl::read(serializer, myArray);

  REQUIRE(myArray.size() == 4);
  REQUIRE(myArray[0] == false);
  REQUIRE(myArray[1] == true);
  REQUIRE(myArray[2] == false);
  REQUIRE(myArray[3] == true);
}

TEST_CASE("structured_input_serializer: BoolLike Invaild")
{
  std::array<bool, 4> myArray = {};
  json                j       = R"([ 1, true, false, true ])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Stream(input);

  REQUIRE_THROWS(acl::read(serializer, myArray));
}

TEST_CASE("structured_input_serializer: SignedIntLike")
{
  std::array<int, 4> myArray = {};
  json               j       = R"([ -40, -10, 10, 40 ])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Stream(input);

  acl::read(serializer, myArray);

  REQUIRE(myArray[0] == -40);
  REQUIRE(myArray[1] == -10);
  REQUIRE(myArray[2] == 10);
  REQUIRE(myArray[3] == 40);
}

TEST_CASE("structured_input_serializer: SignedIntLike Invalid")
{
  std::array<int, 4> myArray = {};
  json               j       = R"([ "-40", -10, 10, 40 ])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Stream(input);

  REQUIRE_THROWS(acl::read(serializer, myArray));
}

TEST_CASE("structured_input_serializer: UnsignedIntLike")
{
  std::array<uint32_t, 4> myArray = {};
  json                    j       = R"([ 40, 10, 10, 40 ])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Stream(input);

  acl::read(serializer, myArray);

  REQUIRE(myArray[0] == 40);
  REQUIRE(myArray[1] == 10);
  REQUIRE(myArray[2] == 10);
  REQUIRE(myArray[3] == 40);
}

TEST_CASE("structured_input_serializer: UnsignedIntLike Invalid")
{
  std::array<uint32_t, 4> myArray = {};
  json                    j       = R"([ true, 10, 10, 40 ])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Stream(input);

  REQUIRE_THROWS(acl::read(serializer, myArray));
}

TEST_CASE("structured_input_serializer: FloatLike")
{
  std::array<float, 4> myArray = {};
  json                 j       = R"([ 434.442, 757.10, 10.745, 424.40 ])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Stream(input);

  acl::read(serializer, myArray);

  REQUIRE(myArray[0] == Catch::Approx(434.442f));
  REQUIRE(myArray[1] == Catch::Approx(757.10f));
  REQUIRE(myArray[2] == Catch::Approx(10.745f));
  REQUIRE(myArray[3] == Catch::Approx(424.40f));
}

TEST_CASE("structured_input_serializer: FloatLike Invalid")
{
  std::array<float, 4> myArray = {};
  json                 j       = R"([ 434, 757.10, 10.745, 424.40 ])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Stream(input);

  REQUIRE_THROWS(acl::read(serializer, myArray));
}

TEST_CASE("structured_input_serializer: PointerLike")
{
  struct pointer
  {
    std::shared_ptr<std::string> a;
    std::unique_ptr<std::string> b;
  };

  pointer pvalue;
  json    j = R"({ "a":"A_value", "b":"B_value", "c":"C_value" })"_json;

  InputData input;
  input.root      = j;
  auto serializer = Stream(input);

  acl::read(serializer, pvalue);

  REQUIRE(pvalue.a);
  REQUIRE(pvalue.b);
  REQUIRE(*pvalue.a == "A_value");
  REQUIRE(*pvalue.b == "B_value");
}

TEST_CASE("structured_input_serializer: PointerLike Null")
{
  struct pointer
  {
    std::shared_ptr<std::string> a;
    std::unique_ptr<std::string> b;

    static auto reflect() noexcept
    {
      return acl::bind(acl::bind<"a", &pointer::a>(), acl::bind<"b", &pointer::b>());
    }
  };

  pointer pvalue;
  json    j = R"({ "a":null, "b":null, "c":null })"_json;

  InputData input;
  input.root      = j;
  auto serializer = Stream(input);

  acl::read(serializer, pvalue);

  REQUIRE(!pvalue.a);
  REQUIRE(!pvalue.b);
}

TEST_CASE("structured_input_serializer: OptionalLike")
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
  auto serializer = Stream(input);

  acl::read(serializer, pvalue);

  REQUIRE(pvalue.a);
  REQUIRE(!pvalue.b);
  REQUIRE(*pvalue.a == "A_value");
}

class CustomClass
{
public:
  friend Stream& operator>>(Stream& ser, CustomClass& cc)
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

TEST_CASE("structured_input_serializer: InputSerializableClass")
{
  std::vector<CustomClass> integers;
  json                     j = R"([ 34, 542, 234 ])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Stream(input);

  acl::read(serializer, integers);

  REQUIRE(integers.size() == 3);
  REQUIRE(integers[0].get() == 34);
  REQUIRE(integers[1].get() == 542);
  REQUIRE(integers[2].get() == 234);
}

TEST_CASE("structured_input_serializer: UnorderedMap basic")
{
  std::unordered_map<std::string, int> map;
  json                                 j = R"([
     ["key1", 100],
     ["key2", 200],
     ["key3", 300]
  ])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Stream(input);

  acl::read(serializer, map);

  REQUIRE(map.size() == 3);
  REQUIRE(map["key1"] == 100);
  REQUIRE(map["key2"] == 200);
  REQUIRE(map["key3"] == 300);
}

TEST_CASE("structured_input_serializer: UnorderedMap with complex values")
{
  std::unordered_map<std::string, ReflTestMember> map;
  json                                            j = R"([
    ["obj1", {
      "first": { "a": 10, "b": 20 },
      "second": { "a": 30, "b": 40 }
    } ],
    ["obj2", {
      "first": { "a": 50, "b": 60 },
      "second": { "a": 70, "b": 80 }
    } ]
  ])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Stream(input);

  acl::read(serializer, map);

  REQUIRE(map.size() == 2);

  auto& obj1 = map["obj1"];
  REQUIRE(obj1.first.get_a() == 10);
  REQUIRE(obj1.first.get_b() == 20);
  REQUIRE(obj1.second.get_a() == 30);
  REQUIRE(obj1.second.get_b() == 40);

  auto& obj2 = map["obj2"];
  REQUIRE(obj2.first.get_a() == 50);
  REQUIRE(obj2.first.get_b() == 60);
  REQUIRE(obj2.second.get_a() == 70);
  REQUIRE(obj2.second.get_b() == 80);
}

TEST_CASE("structured_input_serializer: UnorderedMap invalid value type")
{
  std::unordered_map<std::string, int> map;
  json                                 j = R"([
    ["key1", "not an int"],
    ["key2", 200]
  ])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Stream(input);

  REQUIRE_THROWS(acl::read(serializer, map));
}

TEST_CASE("structured_input_serializer: UnorderedMap nested maps")
{
  std::unordered_map<std::string, std::unordered_map<std::string, int>> nested;
  json                                                                  j = R"([
    ["map1", [
      ["a", 1],
      ["b", 2]
    ]],
    ["map2", [
      ["c", 3],
      ["d", 4]
    ]]
  ])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Stream(input);

  acl::read(serializer, nested);

  REQUIRE(nested.size() == 2);
  REQUIRE(nested["map1"].size() == 2);
  REQUIRE(nested["map2"].size() == 2);
  REQUIRE(nested["map1"]["a"] == 1);
  REQUIRE(nested["map1"]["b"] == 2);
  REQUIRE(nested["map2"]["c"] == 3);
  REQUIRE(nested["map2"]["d"] == 4);
}

TEST_CASE("structured_input_serializer: UnorderedMap empty")
{
  std::unordered_map<std::string, int> map;
  json                                 j = R"([])"_json;

  InputData input;
  input.root      = j;
  auto serializer = Stream(input);

  acl::read(serializer, map);

  REQUIRE(map.empty());
}
// NOLINTEND