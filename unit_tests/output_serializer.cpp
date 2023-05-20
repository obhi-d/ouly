
#include "acl/output_serializer.hpp"
#include "acl/array_types.hpp"
#include <catch2/catch_all.hpp>
#include <compare>
#include <map>
#include <nlohmann/json.hpp>
#include <unordered_map>

using json = nlohmann::json;

class Serializer
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

  void next() noexcept
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

struct ReflTestFriend
{
  int a = 0;
  int b = 0;

  inline auto operator<=>(ReflTestFriend const&) const noexcept = default;
};

template <>
auto acl::reflect<ReflTestFriend>() noexcept
{
  return acl::bind(acl::bind<"a", &ReflTestFriend::a>(), acl::bind<"b", &ReflTestFriend::b>());
}

TEST_CASE("output_serializer: Basic test")
{
  ReflTestFriend example;
  example.a = 4121;
  example.b = 534;

  Serializer                         instance;
  acl::output_serializer<Serializer> ser(instance);
  ser(example);

  REQUIRE(instance.get() == R"({ "a": 4121, "b": 534 })");
}

TEST_CASE("output_serializer: Basic test with internal decl")
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

  Serializer                         instance;
  acl::output_serializer<Serializer> ser(instance);
  ser(example);

  REQUIRE(instance.get() == R"({ "first": { "a": 4121, "b": 534 }, "second": "String Value" })");
}

TEST_CASE("output_serializer: Test tuple")
{
  std::tuple<int, std::string, int, bool> example = {10, "everything", 343, false};

  Serializer                         instance;
  acl::output_serializer<Serializer> ser(instance);
  ser(example);

  REQUIRE(instance.get() == R"([ 10, "everything", 343, false ])");
}

TEST_CASE("output_serializer: String map")
{
  std::unordered_map<std::string, std::string> example = {
    {"everything", "is"}, {"supposed", "to"}, {"work", "just fine"}};

  Serializer                         instance;
  acl::output_serializer<Serializer> ser(instance);
  ser(example);

  json j = json::parse(instance.get());
  REQUIRE(j["everything"] == "is");
  REQUIRE(j["supposed"] == "to");
  REQUIRE(j["work"] == "just fine");
}

TEST_CASE("output_serializer: ArrayLike")
{
  std::vector<int> example = {2, 3, 5, 8, 13};

  Serializer                         instance;
  acl::output_serializer<Serializer> ser(instance);
  ser(example);

  json j = json::parse(instance.get());
  REQUIRE(j.size() == 5);
  for (size_t i = 0; i < j.size(); ++i)
    REQUIRE(j[i] == example[i]);
}

TEST_CASE("output_serializer: VariantLike")
{

  std::vector<std::variant<int, std::string, bool>> example = {2, "string", false, 8, "moo"};

  Serializer                         instance;
  acl::output_serializer<Serializer> ser(instance);
  ser(example);

  json j = json::parse(instance.get());
  REQUIRE(j.size() == 5);
  REQUIRE(j[0].at(1).get<int>() == std::get<int>(example[0]));
  REQUIRE(j[1].at(1).get<std::string>() == std::get<std::string>(example[1]));
  REQUIRE(j[2].at(1).get<bool>() == std::get<bool>(example[2]));
  REQUIRE(j[3].at(1).get<int>() == std::get<int>(example[3]));
  REQUIRE(j[4].at(1).get<std::string>() == std::get<std::string>(example[4]));
}

TEST_CASE("output_serializer: CastableToStringView")
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
      return std::string_view(myBuffer, (size_t)length);
    }
  };

  auto example = ReflEx("reflex output");

  Serializer                         instance;
  acl::output_serializer<Serializer> ser(instance);
  ser(example);

  REQUIRE(instance.get() == R"("reflex output")");
}

TEST_CASE("output_serializer: CastableToString")
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

  Serializer                         instance;
  acl::output_serializer<Serializer> ser(instance);
  ser(example);

  REQUIRE(instance.get() == R"("reflex output")");
}

struct ReflexToStr
{
  int value = 0;
};

template <>
std::string acl::to_string<ReflexToStr>(ReflexToStr const& a)
{
  return std::to_string(a.value);
}

TEST_CASE("output_serializer: TransformToString")
{
  ReflexToStr example;
  example.value = 455232;

  Serializer                         instance;
  acl::output_serializer<Serializer> ser(instance);
  ser(example);

  REQUIRE(instance.get() == R"("455232")");
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
std::string_view acl::to_string_view<ReflexToSV>(ReflexToSV const& a)
{
  return std::string_view(a.myBuffer, (size_t)a.length);
}

TEST_CASE("output_serializer: TransformToStringView")
{
  auto example = ReflexToSV("reflex output");

  Serializer                         instance;
  acl::output_serializer<Serializer> ser(instance);
  ser(example);

  REQUIRE(instance.get() == R"("reflex output")");
}

TEST_CASE("output_serializer: PointerLike")
{
  struct ReflEx
  {
    std::string*                 first  = new std::string("first");
    std::unique_ptr<std::string> second = std::make_unique<std::string>("second");
    std::shared_ptr<std::string> third  = std::make_shared<std::string>("third");
    std::string*                 last   = nullptr;

    static auto reflect() noexcept
    {
      return acl::bind(acl::bind<"first", &ReflEx::first>(), acl::bind<"second", &ReflEx::second>(),
                       acl::bind<"third", &ReflEx::third>(), acl::bind<"last", &ReflEx::last>());
    }
  };

  ReflEx                             example;
  Serializer                         instance;
  acl::output_serializer<Serializer> ser(instance);
  ser(example);

  json j = json::parse(instance.get());
  REQUIRE(j["first"] == "first");
  REQUIRE(j["second"] == "second");
  REQUIRE(j["third"] == "third");
  REQUIRE(j["last"] == json());
}

TEST_CASE("output_serializer: OptionalLike")
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

  ReflEx                             example;
  Serializer                         instance;
  acl::output_serializer<Serializer> ser(instance);
  ser(example);

  json j = json::parse(instance.get());
  REQUIRE(j["first"] == "first");
  REQUIRE(j["last"] == json());
}

TEST_CASE("output_serializer: VariantLike Monostate")
{
  std::variant<std::monostate, int, std::string, bool> example;

  Serializer                         instance;
  acl::output_serializer<Serializer> ser(instance);
  ser(example);

  json j = json::parse(instance.get());
  REQUIRE(j.at(0) == 0);
  REQUIRE(j.at(1) == json());
}

class CustomClass
{
public:
  CustomClass() noexcept = default;
  CustomClass(int a) : value(a) {}

  friend Serializer& operator<<(Serializer& ser, CustomClass const& cc)
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

TEST_CASE("output_serializer: OutputSerializableClass")
{
  std::vector<CustomClass>           integers = {CustomClass(31), CustomClass(5454), CustomClass(323)};
  Serializer                         instance;
  acl::output_serializer<Serializer> ser(instance);
  ser(integers);

  json j = json::parse(instance.get());
  REQUIRE(j.is_array());
  REQUIRE(j.size() == 3);
  REQUIRE(j[0] == 31);
  REQUIRE(j[1] == 5454);
  REQUIRE(j[2] == 323);
}
