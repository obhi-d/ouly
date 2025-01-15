
#include "acl/containers/array_types.hpp"
#include "acl/serializers/serializers.hpp"
#include "acl/utility/transforms.hpp"
#include <acl/reflection/reflection.hpp>
#include <catch2/catch_all.hpp>
#include <compare>
#include <map>
#include <nlohmann/json.hpp>
#include <unordered_map>

using json = nlohmann::json;

// NOLINTBEGIN
class Stream
{
public:
  void begin_array() noexcept
  {
    val_ += "[ ";
  }

  void end_array() noexcept
  {
    val_ += " ]";
  }

  void begin_object() noexcept
  {
    val_ += "{ ";
  }

  void end_object() noexcept
  {
    val_ += " }";
  }

  void key(std::string_view key) noexcept
  {
    val_ += '"';
    val_ += key;
    val_ += "\": ";
  }

  void as_string(std::string_view sv) noexcept
  {
    val_ += '"';
    val_ += sv;
    val_ += '"';
  }

  void as_uint64(uint64_t sv) noexcept
  {
    val_ += std::to_string(sv);
  }

  void as_int64(int64_t sv) noexcept
  {
    val_ += std::to_string(sv);
  }

  void as_double(double sv) noexcept
  {
    val_ += std::to_string(sv);
  }

  void as_bool(bool v) noexcept
  {
    val_ += v ? "true" : "false";
  }

  void as_null() noexcept
  {
    val_ += "null";
  }

  void next_map_entry() noexcept
  {
    val_ += ", ";
  }

  void next_array_entry() noexcept
  {
    val_ += ", ";
  }

  std::string const& get()
  {
    return val_;
  }

private:
  std::string val_;
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

  inline auto operator<=>(ReflTestFriend const&) const noexcept = default;
};

template <>
auto acl::reflect<ReflTestFriend>() noexcept
{
  return acl::bind(acl::bind<"a", &ReflTestFriend::a>(), acl::bind<"b", &ReflTestFriend::b>(),
                   acl::bind<"et", &ReflTestFriend::et>());
}

TEST_CASE("structured_output_serializer: Basic test")
{

  ReflTestFriend example;
  example.a  = 4121;
  example.b  = 534;
  example.et = EnumTest::value1;

  Stream stream;
  acl::write(stream, example);

  REQUIRE(stream.get() == R"({ "a": 4121, "b": 534, "et": 43535 })");
}

TEST_CASE("structured_output_serializer: Basic test with internal decl")
{

  struct ReflTestMember
  {
    ReflTestFriend first;
    std::string    second;

    inline auto operator<=>(ReflTestMember const&) const noexcept = default;

    static auto reflect() noexcept
    {
      return acl::bind(acl::bind<"first", &ReflTestMember::first>(), acl::bind<"second", &ReflTestMember::second>());
    }
  };

  ReflTestMember example;
  example.first.a = 4121;
  example.first.b = 534;
  example.second  = "String Value";

  Stream stream;
  acl::write(stream, example);

  REQUIRE(stream.get() == R"({ "first": { "a": 4121, "b": 534, "et": 323 }, "second": "String Value" })");
}

TEST_CASE("structured_output_serializer: Test tuple")
{
  std::tuple<int, std::string, int, bool> example = {10, "everything", 343, false};

  Stream stream;
  acl::write(stream, example);

  REQUIRE(stream.get() == R"([ 10, "everything", 343, false ])");
}

TEST_CASE("structured_output_serializer: String map")
{
  std::unordered_map<std::string, std::string> example = {
   {"everything",        "is"},
   {  "supposed",        "to"},
   {      "work", "just fine"}
  };

  Stream stream;
  acl::write(stream, example);

  json j = json::parse(stream.get());

  // Readback the json into another unordered map
  std::unordered_map<std::string, std::string> readback;
  for (auto const& value : j)
    readback[value[0]] = value[1];
  // Ensure all elements of example match readback
  for (auto const& [key, value] : example)
    REQUIRE(readback[key] == value);
}

TEST_CASE("structured_output_serializer: ArrayLike")
{
  std::vector<int> example = {2, 3, 5, 8, 13};

  Stream stream;
  acl::write(stream, example);

  json j = json::parse(stream.get());
  REQUIRE(j.size() == 5);
  for (std::size_t i = 0; i < j.size(); ++i)
    REQUIRE(j[i] == example[i]);
}

TEST_CASE("structured_output_serializer: VariantLike")
{

  std::vector<std::variant<int, std::string, bool>> example = {2, "string", false, 8, "moo"};

  Stream stream;
  acl::write(stream, example);

  json j = json::parse(stream.get());
  REQUIRE(j.size() == 5);
  REQUIRE(j[0].at("value").get<int>() == std::get<int>(example[0]));
  REQUIRE(j[1].at("value").get<std::string>() == std::get<std::string>(example[1]));
  REQUIRE(j[2].at("value").get<bool>() == std::get<bool>(example[2]));
  REQUIRE(j[3].at("value").get<int>() == std::get<int>(example[3]));
  REQUIRE(j[4].at("value").get<std::string>() == std::get<std::string>(example[4]));
}

using custom_variant = std::variant<int, std::string, bool, double>;

template <>
struct acl::index_transform<custom_variant>
{
  static constexpr uint32_t to_index(std::string_view ref)
  {
    if (ref == "int")
      return 0;
    if (ref == "string")
      return 1;
    if (ref == "bool")
      return 2;
    if (ref == "double")
      return 3;
    return 0;
  }

  static constexpr std::string_view from_index(std::size_t ref)
  {
    switch (ref)
    {
    case 0:
      return "int";
    case 1:
      return "string";
    case 2:
      return "bool";
    case 3:
      return "double";
    }
    return "int";
  }
};

TEST_CASE("structured_output_serializer: VariantLike with custom index")
{

  {
    custom_variant example = {2};
    Stream         stream;
    acl::write(stream, example);

    REQUIRE(stream.get().find("int") != std::string::npos);
  }

  {
    custom_variant example = {2.0};
    Stream         stream;
    acl::write(stream, example);

    REQUIRE(stream.get().find("double") != std::string::npos);
  }

  {
    custom_variant example = {true};
    Stream         stream;
    acl::write(stream, example);

    REQUIRE(stream.get().find("bool") != std::string::npos);
  }

  {
    custom_variant example = {"string"};
    Stream         stream;
    acl::write(stream, example);

    REQUIRE(stream.get().find("string") != std::string::npos);
  }
}

template <typename T>
concept RR = requires {
  T(std::declval<std::string_view>());
  (std::string_view) std::declval<T>();
};

TEST_CASE("structured_output_serializer: CastableToStringView")
{
  struct ReflEx
  {
    char myBuffer[20];
    int  length = 0;

    ReflEx(std::string_view sv)
    {
      std::memcpy(myBuffer, sv.data(), sv.length());
      length = (int)sv.length();
    }

    inline explicit operator std::string_view() const
    {
      return std::string_view(myBuffer, (std::size_t)length);
    }
  };

  static_assert(RR<ReflEx>, "Convert");
  auto example = ReflEx("reflex output");

  Stream stream;

  acl::write(stream, example);

  REQUIRE(stream.get() == R"("reflex output")");
}

TEST_CASE("structured_output_serializer: CastableToString")
{
  struct ReflEx
  {
    char myBuffer[20] = {};
    int  length       = 0;

    ReflEx(std::string_view sv)
    {
      std::memcpy(myBuffer, sv.data(), sv.length());
      length = (int)sv.length();
    }

    inline explicit operator std::string() const
    {
      return std::string(myBuffer);
    }
  };

  auto example = ReflEx("reflex output");

  Stream stream;

  acl::write(stream, example);

  REQUIRE(stream.get() == R"("reflex output")");
}

struct ReflexToStr
{
  int value = 0;
};

template <>
struct acl::convert<ReflexToStr>
{
  static auto to_string(ReflexToStr const& a) -> std::string
  {
    return std::to_string(a.value);
  }

  static auto from_string(ReflexToStr& a, std::string_view v) -> void
  {
    a.value = std::stoi(std::string(v));
  }
};

TEST_CASE("structured_output_serializer: TransformToString")
{
  ReflexToStr example;
  example.value = 455232;

  Stream stream;

  acl::write(stream, example);

  REQUIRE(stream.get() == R"("455232")");
}

struct ReflexToSV
{
  char myBuffer[20] = {};
  int  length       = 0;

  ReflexToSV(std::string_view sv)
  {
    std::memcpy(myBuffer, sv.data(), sv.length());
    length = (int)sv.length();
  }
};

template <>
struct acl::convert<ReflexToSV>
{
  static auto to_string(ReflexToSV const& a) -> std::string
  {
    return std::string(a.myBuffer, (std::size_t)a.length);
  }

  static auto from_string(ReflexToSV& a, std::string_view v) -> void
  {
    std::memcpy(a.myBuffer, v.data(), v.length());
    a.length = (int)v.length();
  }
};

TEST_CASE("structured_output_serializer: TransformToStringView")
{
  auto example = ReflexToSV("reflex output");

  Stream stream;

  acl::write(stream, example);

  REQUIRE(stream.get() == R"("reflex output")");
}

TEST_CASE("structured_output_serializer: PointerLike")
{
  struct ReflEx
  {
    std::string*                 first  = new std::string("first");
    std::unique_ptr<std::string> second = std::make_unique<std::string>("second");
    std::shared_ptr<std::string> third  = std::make_shared<std::string>("third");
    std::string*                 last   = nullptr;

    ~ReflEx() noexcept
    {
      delete first;
    }

    static auto reflect() noexcept
    {
      return acl::bind(acl::bind<"first", &ReflEx::first>(), acl::bind<"second", &ReflEx::second>(),
                       acl::bind<"third", &ReflEx::third>(), acl::bind<"last", &ReflEx::last>());
    }
  };

  ReflEx example;
  Stream stream;

  acl::write(stream, example);

  json j = json::parse(stream.get());
  REQUIRE(j["first"] == "first");
  REQUIRE(j["second"] == "second");
  REQUIRE(j["third"] == "third");
  REQUIRE(j["last"] == json());
}

TEST_CASE("structured_output_serializer: OptionalLike")
{
  struct ReflEx
  {
    std::optional<std::string> first = std::string("first");
    std::optional<std::string> last;

    static auto reflect() noexcept
    {
      return acl::bind(acl::bind<"first", &ReflEx::first>(), acl::bind<"last", &ReflEx::last>());
    }
  };

  ReflEx example;
  Stream stream;

  acl::write(stream, example);

  json j = json::parse(stream.get());
  REQUIRE(j["first"] == "first");
  REQUIRE(j["last"] == json());
}

TEST_CASE("structured_output_serializer: VariantLike Monostate")
{
  std::variant<std::monostate, int, std::string, bool> example;

  Stream stream;

  acl::write(stream, example);

  json j = json::parse(stream.get());
  REQUIRE(j.at("type") == "0");
  REQUIRE(j.at("value") == json());
}

class CustomClass
{
public:
  CustomClass() noexcept = default;
  CustomClass(int a) : value(a) {}

  friend Stream& operator<<(Stream& ser, CustomClass const& cc)
  {
    ser.as_int64(cc.value);
    return ser;
  }

  inline int get() const
  {
    return value;
  }

private:
  int value;
};

TEST_CASE("structured_output_serializer: OutputSerializableClass")
{
  std::vector<CustomClass> integers = {CustomClass(31), CustomClass(5454), CustomClass(323)};
  Stream                   stream;
  acl::write(stream, integers);

  json j = json::parse(stream.get());
  REQUIRE(j.is_array());
  REQUIRE(j.size() == 3);
  REQUIRE(j[0] == 31);
  REQUIRE(j[1] == 5454);
  REQUIRE(j[2] == 323);
}

TEST_CASE("structured_output_serializer: Unordered map")
{
  std::unordered_map<int, std::string> example = {
   {1,   "one"},
   {2,   "two"},
   {3, "three"}
  };

  Stream stream;
  acl::write(stream, example);

  json j = json::parse(stream.get());
  REQUIRE(j.is_array());
  REQUIRE(j.size() == 3);

  // Since unordered_map order is not guaranteed, check each pair exists
  bool found_all = true;
  for (const auto& pair : example)
  {
    bool found_pair = false;
    for (const auto& elem : j)
    {
      if (elem[0] == pair.first && elem[1] == pair.second)
      {
        found_pair = true;
        break;
      }
    }
    found_all &= found_pair;
  }
  REQUIRE(found_all);
}

// NOLINTEND