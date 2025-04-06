
#include "catch2/catch_all.hpp"
#include "catch2/catch_test_macros.hpp"
#include "ouly/containers/array_types.hpp"
#include "ouly/reflection/detail/base_concepts.hpp"
#include "ouly/reflection/reflection.hpp"
#include "ouly/serializers/binary_stream.hpp"
#include "ouly/serializers/config.hpp"
#include "ouly/serializers/serializers.hpp"
#include "ouly/utility/from_chars.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <compare>
#include <sstream>
#include <unordered_map>
// NOLINTBEGIN
struct FileData
{
  std::stringstream root;
};

class Stream
{
public:
  Stream(FileData& r) : owner(r) {}

  void write(void const* data, std::size_t s)
  {
    owner.get().root.write((const char*)data, (std::streamsize)s);
  }

  void read(void* data, std::size_t s)
  {
    if (!owner.get().root.read((char*)data, (std::streamsize)s))
    {
      throw std::runtime_error("Failed to read");
    }
  }

  void skip(std::size_t s)
  {
    owner.get().root.seekg(s, std::ios_base::cur);
  }

  std::reference_wrapper<FileData> owner;
};

enum class EnumTest
{
  value0 = 323,
  value1 = 43535,
  value3 = 64533,
  none   = 0
};

struct ReflTestFriend
{
  int      a  = rand();
  int      b  = rand();
  EnumTest et = EnumTest::none;

  inline auto operator<=>(ReflTestFriend const&) const noexcept = default;
};

template <>
auto ouly::reflect<ReflTestFriend>() noexcept
{
  return ouly::bind(ouly::bind<"a", &ReflTestFriend::a>(), ouly::bind<"b", &ReflTestFriend::b>(),
                    ouly::bind<"et", &ReflTestFriend::et>());
}

struct little_endian
{
  static constexpr std::endian value = std::endian::little;
};

struct big_endian
{
  static constexpr std::endian value = std::endian::big;
};

TEMPLATE_TEST_CASE("stream: Test valid stream with reflect outside", "[stream]", big_endian, little_endian)
{
  ReflTestFriend write;
  ReflTestFriend read;
  FileData       data;
  auto           stream = Stream(data);

  write.et = EnumTest::value1;

  ouly::write<TestType::value>(stream, write);
  ouly::read<TestType::value>(stream, read);
  REQUIRE(read == write);
  REQUIRE_THROWS(ouly::read<TestType::value>(stream, read));
}

class ReflTestClass
{
  int a = rand();
  int b = rand();

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
    return ouly::bind(ouly::bind<"a", &ReflTestClass::a>(), ouly::bind<"b", &ReflTestClass::b>());
  }

  inline auto operator<=>(ReflTestClass const&) const noexcept = default;
};

TEMPLATE_TEST_CASE("stream: Test valid stream with reflect member", "[stream]", big_endian, little_endian)
{
  ReflTestClass write;
  ReflTestClass read;
  FileData      data;
  auto          stream = Stream(data);

  ouly::write<TestType::value>(stream, write);
  ouly::read<TestType::value>(stream, read);
  REQUIRE(read == write);
  REQUIRE_THROWS(ouly::read<TestType::value>(stream, read));
}

struct ReflTestMember
{
  ReflTestClass first;
  ReflTestClass second;

  static auto reflect() noexcept
  {
    return ouly::bind(ouly::bind<"first", &ReflTestMember::first>(), ouly::bind<"second", &ReflTestMember::second>());
  }

  inline auto operator<=>(ReflTestMember const&) const noexcept = default;
};

TEMPLATE_TEST_CASE("stream: Test compound object", "[stream][compound]", big_endian, little_endian)
{
  ReflTestMember write;
  ReflTestMember read;
  FileData       data;
  auto           stream = Stream(data);

  ouly::write<TestType::value>(stream, write);
  ouly::read<TestType::value>(stream, read);
  REQUIRE(read == write);
  REQUIRE_THROWS(ouly::read<TestType::value>(stream, read));
}

struct ReflTestClass2
{
  ReflTestMember first;
  std::string    second;
  std::string    long_string;

  static auto reflect() noexcept
  {
    return ouly::bind(ouly::bind<"first", &ReflTestClass2::first>(), ouly::bind<"second", &ReflTestClass2::second>(),
                      ouly::bind<"long", &ReflTestClass2::long_string>());
  }

  inline auto operator<=>(ReflTestClass2 const&) const noexcept = default;
};

TEMPLATE_TEST_CASE("stream: Test compound object with simple member", "[stream][compound]", big_endian, little_endian)
{

  ReflTestClass2 write;
  ReflTestClass2 read;
  FileData       data;
  auto           stream = Stream(data);
  write.second          = "compound";
  write.long_string     = "a very long string to avoid short object optimization";

  ouly::write<TestType::value>(stream, write);
  ouly::read<TestType::value>(stream, read);
  REQUIRE(read == write);
  REQUIRE_THROWS(ouly::read<TestType::value>(stream, read));
}

TEMPLATE_TEST_CASE("stream: Test pair", "[stream][pair]", big_endian, little_endian)
{
  FileData                               data;
  std::pair<ReflTestMember, std::string> write  = {ReflTestMember(), "a random string"}, read;
  auto                                   stream = Stream(data);

  ouly::write<TestType::value>(stream, write);
  ouly::read<TestType::value>(stream, read);
  REQUIRE(read == write);
  REQUIRE_THROWS(ouly::read<TestType::value>(stream, read));
}

TEMPLATE_TEST_CASE("stream: TupleLike ", "[stream][tuple]", big_endian, little_endian)
{
  FileData data;
  using type = std::tuple<ReflTestMember, std::string, int, bool>;
  type write = type(ReflTestMember(), "random string", 4200, true);
  type read;
  auto stream = Stream(data);

  ouly::write<TestType::value>(stream, write);
  ouly::read<TestType::value>(stream, read);
  REQUIRE(read == write);
  REQUIRE_THROWS(ouly::read<TestType::value>(stream, read));
}

TEMPLATE_TEST_CASE("stream: StringMapLike ", "[stream][map]", big_endian, little_endian)
{
  FileData data;
  auto     stream = Stream(data);

  using pair_type = std::pair<int, std::string>;
  using type      = std::unordered_map<std::string, pair_type>;

  type write = {
   {  "first", pair_type(100, "first_result")},
   { "second", pair_type(353,   "second_res")},
   {"another",   pair_type(3,        "three")},
   {   "moon", pair_type(663,         "coed")},
  };

  type read;

  ouly::write<TestType::value>(stream, write);
  ouly::read<TestType::value>(stream, read);

  REQUIRE(read == write);
  REQUIRE_THROWS(ouly::read<TestType::value>(stream, read));
}

TEMPLATE_TEST_CASE("stream: ArrayLike", "[stream][map]", big_endian, little_endian)
{
  FileData data;
  auto     stream = Stream(data);

  using pair_type = std::pair<int, std::string>;
  using type      = std::unordered_map<int, pair_type>;

  type write = {
   { 52, pair_type(100, "first_result")},
   {434, pair_type(353,   "second_res")},
   { 12,   pair_type(3,        "three")},
   { 54, pair_type(663,         "coed")},
  };

  type read;

  ouly::write<TestType::value>(stream, write);
  ouly::read<TestType::value>(stream, read);

  REQUIRE(read == write);
  REQUIRE_THROWS(ouly::read<TestType::value>(stream, read));
}

TEMPLATE_TEST_CASE("stream: LinearArrayLike", "[stream][array]", big_endian, little_endian)
{
  FileData data;
  auto     stream = Stream(data);

  using type = ouly::dynamic_array<int>;
  type write = {43, 34, 2344, 3432, 34};
  type read;

  ouly::write<TestType::value>(stream, write);
  ouly::read<TestType::value>(stream, read);

  REQUIRE(read == write);
  REQUIRE_THROWS(ouly::read<TestType::value>(stream, read));
}

template <>
constexpr uint32_t ouly::cfg::magic_type_header<std::array<int, 5>> = 0x12345678;
template <>
constexpr uint32_t ouly::cfg::magic_type_header<std::array<int, 10>> = 0x664411;

TEMPLATE_TEST_CASE("stream: Invalid LinearArrayLike", "[stream][array]", big_endian, little_endian)
{
  FileData data;
  auto     stream = Stream(data);

  using type                = std::array<int, 5>;
  type                write = {43, 34, 2344, 3432, 34};
  std::array<int, 10> read;

  ouly::write<TestType::value>(stream, write);
  REQUIRE_THROWS(ouly::read<TestType::value>(stream, read));
}

TEMPLATE_TEST_CASE("stream: ArrayLike", "[stream][array]", big_endian, little_endian)
{
  FileData data;
  auto     stream = Stream(data);

  using type = std::vector<std::string>;
  type write = {"var{43}", "var{false}", "var{34}", "some string", "", "var{true}"};
  type read;

  ouly::write<TestType::value>(stream, write);
  ouly::read<TestType::value>(stream, read);

  REQUIRE(read == write);
  REQUIRE_THROWS(ouly::read<TestType::value>(stream, read));
}

TEMPLATE_TEST_CASE("stream: ArrayLike with Fastpath", "[stream][array]", big_endian, little_endian)
{
  FileData data;
  auto     stream = Stream(data);

  using type = std::vector<int>;
  type write = {19, 99, 2, 19, 44, 21333696};
  type read;

  ouly::write<TestType::value>(stream, write);
  ouly::read<TestType::value>(stream, read);

  REQUIRE(read == write);
  REQUIRE_THROWS(ouly::read<TestType::value>(stream, read));
}

TEMPLATE_TEST_CASE("stream: VariantLike", "[stream][variant]", big_endian, little_endian)
{
  FileData data;
  auto     stream = Stream(data);

  using var  = std::variant<int, bool, std::string>;
  using type = std::vector<var>;
  type write = {var{43}, var{false}, var{34}, var{"some string"}, var{5543}, var{true}};
  type read;

  ouly::write<TestType::value>(stream, write);
  ouly::read<TestType::value>(stream, read);

  REQUIRE(read == write);
  REQUIRE_THROWS(ouly::read<TestType::value>(stream, read));
}

template <>
constexpr uint32_t ouly::cfg::magic_type_header<std::vector<std::variant<int, bool, std::string>>> = 0x12345678;
template <>
constexpr uint32_t ouly::cfg::magic_type_header<std::vector<int>> = 0x664411;

TEMPLATE_TEST_CASE("stream: Invalid VariantLike ", "[stream][variant]", big_endian, little_endian)
{
  FileData data;
  auto     stream = Stream(data);

  using var              = std::variant<int, bool, std::string>;
  using type             = std::vector<var>;
  type             write = {var{43}, var{false}, var{34}, var{"some string"}, var{5543}, var{true}};
  std::vector<int> read;

  ouly::write<TestType::value>(stream, write);
  REQUIRE_THROWS(ouly::read<TestType::value>(stream, read));
}

struct ConstructedSV
{
  int id                   = -1;
  ConstructedSV() noexcept = default;
  explicit ConstructedSV(std::string_view sv)
  {
    ouly::from_chars(sv, id);
  }

  inline explicit operator std::string() const noexcept
  {
    return std::to_string(id);
  }

  inline auto operator<=>(ConstructedSV const&) const noexcept = default;
};

TEMPLATE_TEST_CASE("stream: ConstructedFromStringView", "[stream][string]", big_endian, little_endian)
{
  FileData data;
  auto     stream = Stream(data);

  using type = ouly::dynamic_array<ConstructedSV>;
  type write = {"10", "11", "12", "13"};
  type read;

  ouly::write<TestType::value>(stream, write);
  ouly::read<TestType::value>(stream, read);

  REQUIRE(read == write);
  REQUIRE_THROWS(ouly::read<TestType::value>(stream, read));
}

struct TransformSV
{
  int id = -1;

  TransformSV(int a) noexcept : id(a) {}
  TransformSV() noexcept                                     = default;
  inline auto operator<=>(TransformSV const&) const noexcept = default;
};

template <>
struct ouly::convert<TransformSV>
{
  static std::string to_type(TransformSV const& r)
  {
    return std::to_string(r.id);
  }

  static void from_type(TransformSV& r, std::string_view sv)
  {
    ouly::from_chars(sv, r.id);
  }
};

static_assert(ouly::detail::Convertible<TransformSV>, "Type not convertible");

TEMPLATE_TEST_CASE("stream: TransformFromString", "[stream][string]", big_endian, little_endian)
{
  FileData data;
  auto     stream = Stream(data);

  using type = ouly::dynamic_array<TransformSV>;
  type write = {TransformSV(11), TransformSV(100), TransformSV(13), TransformSV(300)};
  type read;

  ouly::write<TestType::value>(stream, write);
  ouly::read<TestType::value>(stream, read);

  REQUIRE(read == write);
  REQUIRE_THROWS(ouly::read<TestType::value>(stream, read));
}

struct TransformInt
{
  int id = -1;

  TransformInt(int a) noexcept : id(a) {}
  TransformInt() noexcept                                     = default;
  inline auto operator<=>(TransformInt const&) const noexcept = default;
};

template <>
struct ouly::convert<TransformInt>
{
  static int to_type(TransformInt const& r)
  {
    return r.id;
  }

  static void from_type(TransformInt& r, int sv)
  {
    r.id = sv;
  }
};

static_assert(ouly::detail::Convertible<TransformInt>, "Type not convertible");

TEMPLATE_TEST_CASE("stream: TransformFromInt", "[stream][int]", big_endian, little_endian)
{
  FileData data;
  auto     stream = Stream(data);

  using type = ouly::dynamic_array<TransformInt>;
  type write = {TransformInt(11), TransformInt(100), TransformInt(13), TransformInt(300)};
  type read;

  ouly::write<TestType::value>(stream, write);
  ouly::read<TestType::value>(stream, read);

  REQUIRE(read == write);
  REQUIRE_THROWS(ouly::read<TestType::value>(stream, read));
}

struct TransformIntInt
{
  TransformInt id = -1;

  TransformIntInt(int a) noexcept : id(a) {}
  TransformIntInt() noexcept                                     = default;
  inline auto operator<=>(TransformIntInt const&) const noexcept = default;
};

template <>
struct ouly::convert<TransformIntInt>
{
  static TransformInt to_type(TransformIntInt const& r)
  {
    return r.id;
  }

  static void from_type(TransformIntInt& r, TransformInt sv)
  {
    r.id = sv;
  }
};

static_assert(ouly::detail::Convertible<TransformIntInt>, "Type not convertible");

TEMPLATE_TEST_CASE("stream: TransformFromIntInt", "[stream][intint]", big_endian, little_endian)
{
  FileData data;
  auto     stream = Stream(data);

  using type = ouly::dynamic_array<TransformIntInt>;
  type write = {TransformIntInt(11), TransformIntInt(100), TransformIntInt(13), TransformIntInt(300)};
  type read;

  ouly::write<TestType::value>(stream, write);
  ouly::read<TestType::value>(stream, read);

  REQUIRE(read == write);
  REQUIRE_THROWS(ouly::read<TestType::value>(stream, read));
}

TEMPLATE_TEST_CASE("stream: BoolLike", "[stream][bool]", big_endian, little_endian)
{
  FileData data;
  auto     stream = Stream(data);

  using type = std::array<bool, 4>;
  type write = {true, false, false, true};
  type read;

  ouly::write<TestType::value>(stream, write);
  ouly::read<TestType::value>(stream, read);

  REQUIRE(read == write);
  REQUIRE_THROWS(ouly::read<TestType::value>(stream, read));
}

TEMPLATE_TEST_CASE("stream: BoolLike Invalid", "[stream][bool]", big_endian, little_endian)
{
  FileData data;
  auto     stream = Stream(data);

  using type             = std::array<bool, 4>;
  type             write = {true, false, false, true};
  std::vector<int> read;

  ouly::write<TestType::value>(stream, write);
  REQUIRE_THROWS(ouly::read<TestType::value>(stream, read));
}

TEMPLATE_TEST_CASE("stream: SignedIntLike", "[stream][int]", big_endian, little_endian)
{
  FileData data;
  auto     stream = Stream(data);

  using type = std::array<std::int64_t, 4>;
  type write = {-434, 2, 65, -53};
  type read;

  ouly::write<TestType::value>(stream, write);
  ouly::read<TestType::value>(stream, read);

  REQUIRE(read == write);
  REQUIRE_THROWS(ouly::read<TestType::value>(stream, read));
}

template <>
constexpr uint32_t ouly::cfg::magic_type_header<std::array<std::int64_t, 4>> = 0x12345678;
template <>
constexpr uint32_t ouly::cfg::magic_type_header<std::array<bool, 4>> = 0x664411;

TEMPLATE_TEST_CASE("stream: SignedIntLike Invalid", "[stream][int]", big_endian, little_endian)
{
  FileData data;
  auto     stream = Stream(data);

  using type                = std::array<std::int64_t, 4>;
  type                write = {-434, 2, 65, -53};
  std::array<bool, 4> read;

  ouly::write<TestType::value>(stream, write);
  REQUIRE_THROWS(ouly::read<TestType::value>(stream, read));
}

TEMPLATE_TEST_CASE("stream: FloatLike", "[stream][float]", big_endian, little_endian)
{
  FileData data;
  auto     stream = Stream(data);

  using type = std::array<float, 4>;
  type write = {434.442f, 757.10f, 10.745f, 424.40f};
  type read  = {};

  ouly::write<TestType::value>(stream, write);
  ouly::read<TestType::value>(stream, read);

  REQUIRE(read[0] == Catch::Approx(434.442f));
  REQUIRE(read[1] == Catch::Approx(757.10f));
  REQUIRE(read[2] == Catch::Approx(10.745f));
  REQUIRE(read[3] == Catch::Approx(424.40f));

  REQUIRE_THROWS(ouly::read<TestType::value>(stream, read));
}

TEMPLATE_TEST_CASE("stream: PointerLike", "[stream][pointer]", big_endian, little_endian)
{
  FileData data;
  auto     stream = Stream(data);

  struct pointer
  {
    std::shared_ptr<std::string> a;
    std::unique_ptr<std::string> b;
  };

  using type = pointer;

  type write;
  type read;

  write.a = std::make_shared<std::string>("shared");
  write.b = std::make_unique<std::string>("unique");

  ouly::write<TestType::value>(stream, write);
  ouly::read<TestType::value>(stream, read);

  REQUIRE(read.a);
  REQUIRE(read.b);
  REQUIRE(*read.a == "shared");
  REQUIRE(*read.b == "unique");
}

TEMPLATE_TEST_CASE("stream: NullPointerLike", "[stream][pointer]", big_endian, little_endian)
{
  FileData data;
  auto     stream = Stream(data);

  struct pointer
  {
    std::shared_ptr<std::string> a;
    std::unique_ptr<std::string> b;
  };

  using type = pointer;

  type write;
  type read;

  ouly::write<TestType::value>(stream, write);
  ouly::read<TestType::value>(stream, read);

  REQUIRE(!read.a);
  REQUIRE(!read.b);
}

TEMPLATE_TEST_CASE("stream: OptionalLike", "[stream][optional]", big_endian, little_endian)
{
  FileData data;
  auto     stream = Stream(data);

  struct pointer
  {
    std::optional<std::string> a;
    std::optional<std::string> b;
  };

  using type = pointer;

  type write;
  write.a = "something";
  type read;

  ouly::write<TestType::value>(stream, write);
  ouly::read<TestType::value>(stream, read);

  REQUIRE(read.a);
  REQUIRE(!read.b);
  REQUIRE(*read.a == "something");
}

class CustomClass
{
public:
  CustomClass() noexcept = default;
  CustomClass(int a) noexcept : value(a) {}

  friend Stream& operator>>(Stream& ser, CustomClass& cc)
  {
    ser.read(&cc.value, sizeof(cc.value));
    return ser;
  }
  friend Stream& operator<<(Stream& ser, CustomClass const& cc)
  {
    ser.write(&cc.value, sizeof(cc.value));
    return ser;
  }

  inline int get() const
  {
    return value;
  }

private:
  int value = 0;
};

TEMPLATE_TEST_CASE("stream: SerializableClass", "[stream][optional]", big_endian, little_endian)
{
  FileData data;
  auto     stream = Stream(data);

  using type = std::vector<CustomClass>;
  type write = {CustomClass(10), CustomClass(12), CustomClass(13)};

  type read;

  ouly::write<TestType::value>(stream, write);
  ouly::read<TestType::value>(stream, read);

  REQUIRE(read.size() == 3);
  REQUIRE(read[0].get() == 10);
  REQUIRE(read[1].get() == 12);
  REQUIRE(read[2].get() == 13);
}

// NOLINTEND