
#include "acl/containers/array_types.hpp"
#include "acl/serializers/binary_input_serializer.hpp"
#include "acl/serializers/binary_output_serializer.hpp"
#include <array>
#include <catch2/catch_all.hpp>
#include <charconv>
#include <compare>
#include <sstream>
#include <unordered_map>
// NOLINTBEGIN
struct FileData
{
	std::stringstream			root;
	acl::serializer_error ec = acl::serializer_error::none;
};

class Serializer
{
public:
	Serializer() noexcept = default;
	Serializer(FileData& r) : owner(r) {}

	void write(void const* data, std::size_t s)
	{
		owner.get().root.write((const char*)data, (std::streamsize)s);
	}

	bool read(void* data, std::size_t s)
	{
		return (bool)owner.get().root.read((char*)data, (std::streamsize)s);
	}

	void error(std::string_view, std::error_code ec)
	{
		owner.get().ec = (acl::serializer_error)ec.value();
	}

	bool failed() const
	{
		return owner.get().root.fail();
	}

	std::reference_wrapper<FileData> owner;
};

enum class EnumTest
{
	value0 = 323,
	value1 = 43535,
	value3 = 64533,
	none	 = 0
};

struct ReflTestFriend
{
	int			 a	= rand();
	int			 b	= rand();
	EnumTest et = EnumTest::none;

	inline auto operator<=>(ReflTestFriend const&) const noexcept = default;
};

template <>
auto acl::reflect<ReflTestFriend>() noexcept
{
	return acl::bind(acl::bind<"a", &ReflTestFriend::a>(), acl::bind<"b", &ReflTestFriend::b>(),
									 acl::bind<"et", &ReflTestFriend::et>());
}

struct little_endian
{
	static constexpr std::endian value = std::endian::little;
};

struct big_endian
{
	static constexpr std::endian value = std::endian::big;
};

TEMPLATE_TEST_CASE("serializer: Test valid stream with reflect outside", "[serializer]", big_endian, little_endian)
{
	ReflTestFriend obj;
	obj.et = EnumTest::value1;

	FileData data;
	auto		 serializer = Serializer(data);
	auto		 out				= acl::binary_output_serializer<Serializer, TestType::value>(serializer);
	out << obj;
	auto					 in = acl::binary_input_serializer<Serializer, TestType::value>(serializer);
	ReflTestFriend read;
	in >> read;
	REQUIRE(read.a == obj.a);
	REQUIRE(read.b == obj.b);
	REQUIRE(read.et == obj.et);
	in >> read;
	REQUIRE(data.ec != acl::serializer_error::none);
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
		return acl::bind(acl::bind<"a", &ReflTestClass::a>(), acl::bind<"b", &ReflTestClass::b>());
	}

	inline auto operator<=>(ReflTestClass const&) const noexcept = default;
};

TEMPLATE_TEST_CASE("serializer: Test valid stream with reflect member", "[serializer]", big_endian, little_endian)
{
	ReflTestClass obj;

	FileData data;
	auto		 serializer = Serializer(data);
	auto		 out				= acl::binary_output_serializer<Serializer, TestType::value>(serializer);
	out << obj;
	auto					in = acl::binary_input_serializer<Serializer, TestType::value>(serializer);
	ReflTestClass read;
	in >> read;
	REQUIRE(read == obj);

	in >> read;
	REQUIRE(data.ec != acl::serializer_error::none);
}

struct ReflTestMember
{
	ReflTestClass first;
	ReflTestClass second;

	static auto reflect() noexcept
	{
		return acl::bind(acl::bind<"first", &ReflTestMember::first>(), acl::bind<"second", &ReflTestMember::second>());
	}

	inline auto operator<=>(ReflTestMember const&) const noexcept = default;
};

TEMPLATE_TEST_CASE("serializer: Test compound object", "[serializer][compound]", big_endian, little_endian)
{
	ReflTestMember test;
	FileData			 data;
	auto					 serializer = Serializer(data);
	auto					 out				= acl::binary_output_serializer<Serializer, TestType::value>(serializer);
	out << test;
	auto					 in = acl::binary_input_serializer<Serializer, TestType::value>(serializer);
	ReflTestMember read;
	in >> read;

	REQUIRE(read == test);

	in >> read;
	REQUIRE(data.ec != acl::serializer_error::none);
}

struct ReflTestClass2
{
	ReflTestMember first;
	std::string		 second;
	std::string		 long_string;

	static auto reflect() noexcept
	{
		return acl::bind(acl::bind<"first", &ReflTestClass2::first>(), acl::bind<"second", &ReflTestClass2::second>(),
										 acl::bind<"long", &ReflTestClass2::long_string>());
	}

	inline auto operator<=>(ReflTestClass2 const&) const noexcept = default;
};

TEMPLATE_TEST_CASE("serializer: Test compound object with simple member", "[serializer][compound]", big_endian,
									 little_endian)
{

	ReflTestClass2 test;
	test.second			 = "compound";
	test.long_string = "a very long string to avoid short object optimization";
	FileData data;
	auto		 serializer = Serializer(data);
	auto		 out				= acl::binary_output_serializer<Serializer, TestType::value>(serializer);
	auto		 in					= acl::binary_input_serializer<Serializer, TestType::value>(serializer);
	out << test;
	ReflTestClass2 read;
	in >> read;

	REQUIRE(read == test);

	in >> read;
	REQUIRE(data.ec != acl::serializer_error::none);
}

TEMPLATE_TEST_CASE("serializer: Test pair", "[serializer][pair]", big_endian, little_endian)
{
	FileData data;
	auto		 serializer = Serializer(data);
	auto		 out				= acl::binary_output_serializer<Serializer, TestType::value>(serializer);
	auto		 in					= acl::binary_input_serializer<Serializer, TestType::value>(serializer);

	std::pair<ReflTestMember, std::string> write = {ReflTestMember(), "a random string"}, read;

	out << write;
	in >> read;

	REQUIRE(read == write);

	in >> read;
	REQUIRE(data.ec != acl::serializer_error::none);
}

TEMPLATE_TEST_CASE("serializer: TupleLike ", "[serializer][tuple]", big_endian, little_endian)
{
	FileData data;
	auto		 serializer = Serializer(data);
	auto		 out				= acl::binary_output_serializer<Serializer, TestType::value>(serializer);
	auto		 in					= acl::binary_input_serializer<Serializer, TestType::value>(serializer);

	using type = std::tuple<ReflTestMember, std::string, int, bool>;
	type write = type(ReflTestMember(), "random string", 4200, true);
	type read;

	out << write;
	in >> read;

	REQUIRE(read == write);

	in >> read;
	REQUIRE(data.ec != acl::serializer_error::none);
}

TEMPLATE_TEST_CASE("serializer: StringMapLike ", "[serializer][map]", big_endian, little_endian)
{
	FileData data;
	auto		 serializer = Serializer(data);
	auto		 out				= acl::binary_output_serializer<Serializer, TestType::value>(serializer);
	auto		 in					= acl::binary_input_serializer<Serializer, TestType::value>(serializer);

	using pair_type = std::pair<int, std::string>;
	using type			= std::unordered_map<std::string, pair_type>;

	type write = {
	 {	"first", pair_type(100, "first_result")},
	 { "second", pair_type(353,	 "second_res")},
	 {"another",	 pair_type(3,				"three")},
	 {		"moon", pair_type(663,				 "coed")},
	};

	type read;

	out << write;
	in >> read;

	REQUIRE(read == write);

	in >> read;
	REQUIRE(data.ec != acl::serializer_error::none);
}

TEMPLATE_TEST_CASE("serializer: ArrayLike", "[serializer][map]", big_endian, little_endian)
{
	FileData data;
	auto		 serializer = Serializer(data);
	auto		 out				= acl::binary_output_serializer<Serializer, TestType::value>(serializer);
	auto		 in					= acl::binary_input_serializer<Serializer, TestType::value>(serializer);

	using pair_type = std::pair<int, std::string>;
	using type			= std::unordered_map<int, pair_type>;

	type write = {
	 { 52, pair_type(100, "first_result")},
	 {434, pair_type(353,		"second_res")},
	 { 12,		pair_type(3,				 "three")},
	 { 54, pair_type(663,				 "coed")},
	};

	type read;

	out << write;
	in >> read;

	REQUIRE(read == write);

	in >> read;
	REQUIRE(data.ec != acl::serializer_error::none);
}

TEMPLATE_TEST_CASE("serializer: LinearArrayLike", "[serializer][array]", big_endian, little_endian)
{
	FileData data;
	auto		 serializer = Serializer(data);
	auto		 out				= acl::binary_output_serializer<Serializer, TestType::value>(serializer);
	auto		 in					= acl::binary_input_serializer<Serializer, TestType::value>(serializer);

	using type = acl::dynamic_array<int>;
	type write = {43, 34, 2344, 3432, 34};
	type read;

	out << write;
	in >> read;

	REQUIRE(read == write);

	in >> read;
	REQUIRE(data.ec != acl::serializer_error::none);
}

TEMPLATE_TEST_CASE("serializer: Invalid LinearArrayLike", "[serializer][array]", big_endian, little_endian)
{
	FileData data;
	auto		 serializer = Serializer(data);
	auto		 out				= acl::binary_output_serializer<Serializer, TestType::value>(serializer);
	auto		 in					= acl::binary_input_serializer<Serializer, TestType::value>(serializer);

	using type								= std::array<int, 5>;
	type								write = {43, 34, 2344, 3432, 34};
	std::array<int, 10> read;

	out << write;
	in >> read;

	REQUIRE(data.ec != acl::serializer_error::none);
}

TEMPLATE_TEST_CASE("serializer: ArrayLike", "[serializer][array]", big_endian, little_endian)
{
	FileData data;
	auto		 serializer = Serializer(data);
	auto		 out				= acl::binary_output_serializer<Serializer, TestType::value>(serializer);
	auto		 in					= acl::binary_input_serializer<Serializer, TestType::value>(serializer);

	using type = std::vector<std::string>;
	type write = {"var{43}", "var{false}", "var{34}", "some string", "", "var{true}"};
	type read;

	out << write;
	in >> read;

	REQUIRE(read == write);

	in >> read;
	REQUIRE(data.ec != acl::serializer_error::none);
}

TEMPLATE_TEST_CASE("serializer: ArrayLike with Fastpath", "[serializer][array]", big_endian, little_endian)
{
	FileData data;
	auto		 serializer = Serializer(data);
	auto		 out				= acl::binary_output_serializer<Serializer, TestType::value>(serializer);
	auto		 in					= acl::binary_input_serializer<Serializer, TestType::value>(serializer);

	using type = std::vector<int>;
	type write = {19, 99, 2, 19, 44, 21333696};
	type read;

	out << write;
	in >> read;

	REQUIRE(read == write);

	in >> read;
	REQUIRE(data.ec != acl::serializer_error::none);
}

TEMPLATE_TEST_CASE("serializer: VariantLike", "[serializer][variant]", big_endian, little_endian)
{
	FileData data;
	auto		 serializer = Serializer(data);
	auto		 out				= acl::binary_output_serializer<Serializer, TestType::value>(serializer);
	auto		 in					= acl::binary_input_serializer<Serializer, TestType::value>(serializer);

	using var	 = std::variant<int, bool, std::string>;
	using type = std::vector<var>;
	type write = {var{43}, var{false}, var{34}, var{"some string"}, var{5543}, var{true}};
	type read;

	out << write;
	in >> read;

	REQUIRE(read == write);

	in >> read;
	REQUIRE(data.ec != acl::serializer_error::none);
}

TEMPLATE_TEST_CASE("serializer: Invalid VariantLike ", "[serializer][variant]", big_endian, little_endian)
{
	FileData data;
	auto		 serializer = Serializer(data);
	auto		 out				= acl::binary_output_serializer<Serializer, TestType::value>(serializer);
	auto		 in					= acl::binary_input_serializer<Serializer, TestType::value>(serializer);

	using var							 = std::variant<int, bool, std::string>;
	using type						 = std::vector<var>;
	type						 write = {var{43}, var{false}, var{34}, var{"some string"}, var{5543}, var{true}};
	std::vector<int> read;

	out << write;
	in >> read;

	REQUIRE(data.ec != acl::serializer_error::none);
}

struct ConstructedSV
{
	int id									 = -1;
	ConstructedSV() noexcept = default;
	explicit ConstructedSV(std::string_view sv)
	{
		std::from_chars(sv.data(), sv.data() + sv.length(), id);
	}

	inline explicit operator std::string() const noexcept
	{
		return std::to_string(id);
	}

	inline auto operator<=>(ConstructedSV const&) const noexcept = default;
};

TEMPLATE_TEST_CASE("serializer: ConstructedFromStringView", "[serializer][string]", big_endian, little_endian)
{
	FileData data;
	auto		 serializer = Serializer(data);
	auto		 out				= acl::binary_output_serializer<Serializer, TestType::value>(serializer);
	auto		 in					= acl::binary_input_serializer<Serializer, TestType::value>(serializer);

	using type = acl::dynamic_array<ConstructedSV>;
	type write = {"10", "11", "12", "13"};
	type read;

	out << write;
	in >> read;

	REQUIRE(read == write);

	in >> read;
	REQUIRE(data.ec != acl::serializer_error::none);
}

struct TransformSV
{
	int id = -1;

	TransformSV(int a) noexcept : id(a) {}
	TransformSV() noexcept																		 = default;
	inline auto operator<=>(TransformSV const&) const noexcept = default;
};

template <>
std::string acl::to_string<TransformSV>(TransformSV const& r)
{
	return std::to_string(r.id);
}

template <>
void acl::from_string<TransformSV>(TransformSV& r, std::string_view sv)
{
	std::from_chars(sv.data(), sv.data() + sv.length(), r.id);
}

TEMPLATE_TEST_CASE("serializer: TransformFromString", "[serializer][string]", big_endian, little_endian)
{
	FileData data;
	auto		 serializer = Serializer(data);
	auto		 out				= acl::binary_output_serializer<Serializer, TestType::value>(serializer);
	auto		 in					= acl::binary_input_serializer<Serializer, TestType::value>(serializer);

	using type = acl::dynamic_array<TransformSV>;
	type write = {TransformSV(11), TransformSV(100), TransformSV(13), TransformSV(300)};
	type read;

	out << write;
	in >> read;

	REQUIRE(read == write);

	in >> read;
	REQUIRE(data.ec != acl::serializer_error::none);
}

TEMPLATE_TEST_CASE("serializer: BoolLike", "[serializer][bool]", big_endian, little_endian)
{
	FileData data;
	auto		 serializer = Serializer(data);
	auto		 out				= acl::binary_output_serializer<Serializer, TestType::value>(serializer);
	auto		 in					= acl::binary_input_serializer<Serializer, TestType::value>(serializer);

	using type = std::array<bool, 4>;
	type write = {true, false, false, true};
	type read;

	out << write;
	in >> read;

	REQUIRE(read == write);

	in >> read;
	REQUIRE(data.ec != acl::serializer_error::none);
}

TEMPLATE_TEST_CASE("serializer: BoolLike Invalid", "[serializer][bool]", big_endian, little_endian)
{
	FileData data;
	auto		 serializer = Serializer(data);
	auto		 out				= acl::binary_output_serializer<Serializer, TestType::value>(serializer);
	auto		 in					= acl::binary_input_serializer<Serializer, TestType::value>(serializer);

	using type						 = std::array<bool, 4>;
	type						 write = {true, false, false, true};
	std::vector<int> read;

	out << write;
	in >> read;

	REQUIRE(data.ec != acl::serializer_error::none);
}

TEMPLATE_TEST_CASE("serializer: SignedIntLike", "[serializer][int]", big_endian, little_endian)
{
	FileData data;
	auto		 serializer = Serializer(data);
	auto		 out				= acl::binary_output_serializer<Serializer, TestType::value>(serializer);
	auto		 in					= acl::binary_input_serializer<Serializer, TestType::value>(serializer);

	using type = std::array<std::int64_t, 4>;
	type write = {-434, 2, 65, -53};
	type read;

	out << write;
	in >> read;

	REQUIRE(read == write);

	in >> read;
	REQUIRE(data.ec != acl::serializer_error::none);
}

TEMPLATE_TEST_CASE("serializer: SignedIntLike Invalid", "[serializer][int]", big_endian, little_endian)
{
	FileData data;
	auto		 serializer = Serializer(data);
	auto		 out				= acl::binary_output_serializer<Serializer, TestType::value>(serializer);
	auto		 in					= acl::binary_input_serializer<Serializer, TestType::value>(serializer);

	using type								= std::array<std::int64_t, 4>;
	type								write = {-434, 2, 65, -53};
	std::array<bool, 4> read;

	out << write;
	in >> read;

	REQUIRE(data.ec != acl::serializer_error::none);
}

TEMPLATE_TEST_CASE("serializer: FloatLike", "[serializer][float]", big_endian, little_endian)
{
	FileData data;
	auto		 serializer = Serializer(data);
	auto		 out				= acl::binary_output_serializer<Serializer, TestType::value>(serializer);
	auto		 in					= acl::binary_input_serializer<Serializer, TestType::value>(serializer);

	using type = std::array<float, 4>;
	type write = {434.442f, 757.10f, 10.745f, 424.40f};
	type read	 = {};

	out << write;
	in >> read;

	REQUIRE(read[0] == Catch::Approx(434.442f));
	REQUIRE(read[1] == Catch::Approx(757.10f));
	REQUIRE(read[2] == Catch::Approx(10.745f));
	REQUIRE(read[3] == Catch::Approx(424.40f));

	in >> read;
	REQUIRE(data.ec != acl::serializer_error::none);
}

TEMPLATE_TEST_CASE("serializer: PointerLike", "[serializer][pointer]", big_endian, little_endian)
{
	FileData data;
	auto		 serializer = Serializer(data);
	auto		 out				= acl::binary_output_serializer<Serializer, TestType::value>(serializer);
	auto		 in					= acl::binary_input_serializer<Serializer, TestType::value>(serializer);

	struct pointer
	{
		std::shared_ptr<std::string> a;
		std::unique_ptr<std::string> b;
		std::string*								 c = nullptr;

		static auto reflect() noexcept
		{
			return acl::bind(acl::bind<"a", &pointer::a>(), acl::bind<"b", &pointer::b>(), acl::bind<"c", &pointer::c>());
		}
	};

	using type = pointer;

	type write;
	type read;

	write.a = std::make_shared<std::string>("shared");
	write.b = std::make_unique<std::string>("unique");
	write.c = new std::string("new");

	out << write;
	in >> read;

	REQUIRE(read.a);
	REQUIRE(read.b);
	REQUIRE(read.c);
	REQUIRE(*read.a == "shared");
	REQUIRE(*read.b == "unique");
	REQUIRE(*read.c == "new");

	delete write.c;
	delete read.c;
}

TEMPLATE_TEST_CASE("serializer: NullPointerLike", "[serializer][pointer]", big_endian, little_endian)
{
	FileData data;
	auto		 serializer = Serializer(data);
	auto		 out				= acl::binary_output_serializer<Serializer, TestType::value>(serializer);
	auto		 in					= acl::binary_input_serializer<Serializer, TestType::value>(serializer);

	struct pointer
	{
		std::shared_ptr<std::string> a;
		std::unique_ptr<std::string> b;
		std::string*								 c = nullptr;

		static auto reflect() noexcept
		{
			return acl::bind(acl::bind<"a", &pointer::a>(), acl::bind<"b", &pointer::b>(), acl::bind<"c", &pointer::c>());
		}
	};

	using type = pointer;

	type write;
	type read;

	out << write;
	in >> read;

	REQUIRE(!read.a);
	REQUIRE(!read.b);
	REQUIRE(!read.c);
}

TEMPLATE_TEST_CASE("serializer: OptionalLike", "[serializer][optional]", big_endian, little_endian)
{
	FileData data;
	auto		 serializer = Serializer(data);
	auto		 out				= acl::binary_output_serializer<Serializer, TestType::value>(serializer);
	auto		 in					= acl::binary_input_serializer<Serializer, TestType::value>(serializer);

	struct pointer
	{
		std::optional<std::string> a;
		std::optional<std::string> b;

		static auto reflect() noexcept
		{
			return acl::bind(acl::bind<"a", &pointer::a>(), acl::bind<"b", &pointer::b>());
		}
	};

	using type = pointer;

	type write;
	write.a = "something";
	type read;

	out << write;
	in >> read;

	REQUIRE(read.a);
	REQUIRE(!read.b);
	REQUIRE(*read.a == "something");
}

class CustomClass
{
public:
	CustomClass() noexcept = default;
	CustomClass(int a) noexcept : value(a) {}

	friend Serializer& operator>>(Serializer& ser, CustomClass& cc)
	{
		ser.read(&cc.value, sizeof(cc.value));
		return ser;
	}
	friend Serializer& operator<<(Serializer& ser, CustomClass const& cc)
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

TEMPLATE_TEST_CASE("serializer: SerializableClass", "[serializer][optional]", big_endian, little_endian)
{
	FileData data;
	auto		 serializer = Serializer(data);
	auto		 out				= acl::binary_output_serializer<Serializer, TestType::value>(serializer);
	auto		 in					= acl::binary_input_serializer<Serializer, TestType::value>(serializer);

	using type = std::vector<CustomClass>;
	type write = {CustomClass(10), CustomClass(12), CustomClass(13)};

	type read;

	out << write;
	in >> read;

	REQUIRE(read.size() == 3);
	REQUIRE(read[0].get() == 10);
	REQUIRE(read[1].get() == 12);
	REQUIRE(read[2].get() == 13);
}
// NOLINTEND