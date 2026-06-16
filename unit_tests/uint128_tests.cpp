#include "catch2/catch_all.hpp"
#include "ouly/utility/uint128.hpp"
#include <array>
#include <bit>
#include <limits>

// NOLINTBEGIN

using ouly::uint128_t;

TEST_CASE("uint128_t - Construction and assignment", "[uint128]")
{
  SECTION("Default construction")
  {
    constexpr uint128_t value{};
    CHECK(value.high() == 0);
    CHECK(value.low() == 0);
    CHECK_FALSE(static_cast<bool>(value));
  }

  SECTION("Construction from two uint64_t")
  {
    constexpr uint128_t value{0x123456789ABCDEF0ULL, 0xFEDCBA9876543210ULL};
    CHECK(value.high() == 0x123456789ABCDEF0ULL);
    CHECK(value.low() == 0xFEDCBA9876543210ULL);
    CHECK(static_cast<bool>(value));
  }

  SECTION("Construction from unsigned integer types")
  {
    constexpr uint128_t from_uint8{static_cast<uint8_t>(255)};
    CHECK(from_uint8.high() == 0);
    CHECK(from_uint8.low() == 255);

    constexpr uint128_t from_uint32{UINT32_MAX};
    CHECK(from_uint32.high() == 0);
    CHECK(from_uint32.low() == UINT32_MAX);

    constexpr uint128_t from_uint64{UINT64_MAX};
    CHECK(from_uint64.high() == 0);
    CHECK(from_uint64.low() == UINT64_MAX);
  }

  SECTION("Construction from signed integer types")
  {
    constexpr uint128_t from_positive{static_cast<int32_t>(42)};
    CHECK(from_positive.high() == 0);
    CHECK(from_positive.low() == 42);

    constexpr uint128_t from_negative{static_cast<int32_t>(-1)};
    CHECK(from_negative.high() == UINT64_MAX);
    CHECK(from_negative.low() == UINT64_MAX);
  }

  SECTION("Copy and move semantics")
  {
    uint128_t original{100, 200};
    uint128_t copy = original;
    CHECK(copy.high() == 100);
    CHECK(copy.low() == 200);

    uint128_t moved = std::move(original);
    CHECK(moved.high() == 100);
    CHECK(moved.low() == 200);
  }
}

TEST_CASE("uint128_t - Comparison operators", "[uint128]")
{
  SECTION("Equality")
  {
    constexpr uint128_t a{100, 200};
    constexpr uint128_t b{100, 200};
    constexpr uint128_t c{100, 201};
    constexpr uint128_t d{101, 200};

    CHECK(a == b);
    CHECK_FALSE(a == c);
    CHECK_FALSE(a == d);
    CHECK(a != c);
    CHECK(a != d);
  }

  SECTION("Three-way comparison")
  {
    constexpr uint128_t small{0, 100};
    constexpr uint128_t medium{0, 200};
    constexpr uint128_t large{1, 0};

    CHECK(small < medium);
    CHECK(medium < large);
    CHECK(small <= medium);
    CHECK(medium <= large);
    CHECK(medium <= medium);

    CHECK(large > medium);
    CHECK(medium > small);
    CHECK(large >= medium);
    CHECK(medium >= small);
    CHECK(medium >= medium);
  }
}

TEST_CASE("uint128_t - Arithmetic operators", "[uint128]")
{
  SECTION("Addition")
  {
    uint128_t a{0, 100};
    uint128_t b{0, 50};
    a += b;
    CHECK(a.high() == 0);
    CHECK(a.low() == 150);

    // Test overflow from low to high
    uint128_t c{0, UINT64_MAX};
    uint128_t d{0, 2};
    c += d;
    CHECK(c.high() == 1);
    CHECK(c.low() == 1);

    // Test binary operator
    uint128_t e = uint128_t{10, 20} + uint128_t{5, 30};
    CHECK(e.high() == 15);
    CHECK(e.low() == 50);
  }

  SECTION("Subtraction")
  {
    uint128_t a{0, 100};
    uint128_t b{0, 50};
    a -= b;
    CHECK(a.high() == 0);
    CHECK(a.low() == 50);

    // Test borrow from high to low
    uint128_t c{1, 0};
    uint128_t d{0, 1};
    c -= d;
    CHECK(c.high() == 0);
    CHECK(c.low() == UINT64_MAX);

    // Test binary operator
    uint128_t e = uint128_t{10, 50} - uint128_t{5, 30};
    CHECK(e.high() == 5);
    CHECK(e.low() == 20);
  }

  SECTION("Multiplication")
  {
    uint128_t a{0, 10};
    uint128_t b{0, 20};
    a *= b;
    CHECK(a.high() == 0);
    CHECK(a.low() == 200);

    // Test multiplication that overflows to high part
    uint128_t c{0, UINT32_MAX};
    uint128_t d{0, UINT32_MAX};
    c *= d;
    CHECK(c.high() != 0);

    // Test binary operator
    uint128_t e = uint128_t{0, 7} * uint128_t{0, 6};
    CHECK(e.low() == 42);
  }

  SECTION("Division")
  {
    uint128_t a{0, 100};
    uint128_t b{0, 10};
    a /= b;
    CHECK(a.high() == 0);
    CHECK(a.low() == 10);

    // Test division with high bits
    uint128_t c{1, 0};
    uint128_t d{0, 2};
    c /= d;
    CHECK(c.high() == 0);
    CHECK(c.low() == (UINT64_C(1) << 63));

    // Test binary operator
    uint128_t e = uint128_t{0, 144} / uint128_t{0, 12};
    CHECK(e.low() == 12);
  }

  SECTION("Modulo")
  {
    uint128_t a{0, 105};
    uint128_t b{0, 10};
    a %= b;
    CHECK(a.high() == 0);
    CHECK(a.low() == 5);

    // Test binary operator
    uint128_t c = uint128_t{0, 17} % uint128_t{0, 5};
    CHECK(c.low() == 2);
  }

  SECTION("Increment and decrement")
  {
    uint128_t a{0, 10};
    ++a;
    CHECK(a.low() == 11);

    a++;
    CHECK(a.low() == 12);

    --a;
    CHECK(a.low() == 11);

    a--;
    CHECK(a.low() == 10);

    // Test overflow
    uint128_t b{0, UINT64_MAX};
    ++b;
    CHECK(b.high() == 1);
    CHECK(b.low() == 0);
  }

  SECTION("Unary negation")
  {
    uint128_t a{0, 1};
    uint128_t neg = -a;
    CHECK(neg.high() == UINT64_MAX);
    CHECK(neg.low() == UINT64_MAX);

    uint128_t b{0, 0};
    uint128_t neg_zero = -b;
    CHECK(neg_zero.high() == 0);
    CHECK(neg_zero.low() == 0);
  }
}

TEST_CASE("uint128_t - Bitwise operators", "[uint128]")
{
  SECTION("AND operation")
  {
    uint128_t a{0xFF00FF00FF00FF00ULL, 0x00FF00FF00FF00FFULL};
    uint128_t b{0xF0F0F0F0F0F0F0F0ULL, 0x0F0F0F0F0F0F0F0FULL};
    a &= b;
    CHECK(a.high() == 0xF000F000F000F000ULL);
    CHECK(a.low() == 0x000F000F000F000FULL);
  }

  SECTION("OR operation")
  {
    uint128_t a{0xFF00000000000000ULL, 0x00000000000000FFULL};
    uint128_t b{0x00FF000000000000ULL, 0x0000000000FF0000ULL};
    a |= b;
    CHECK(a.high() == 0xFFFF000000000000ULL);
    CHECK(a.low() == 0x0000000000FF00FFULL);
  }

  SECTION("XOR operation")
  {
    uint128_t a{0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL};
    uint128_t b{0xAAAAAAAAAAAAAAAAULL, 0x5555555555555555ULL};
    a ^= b;
    CHECK(a.high() == 0x5555555555555555ULL);
    CHECK(a.low() == 0xAAAAAAAAAAAAAAAAULL);
  }

  SECTION("NOT operation")
  {
    uint128_t a{0x0F0F0F0F0F0F0F0FULL, 0xF0F0F0F0F0F0F0F0ULL};
    uint128_t result = ~a;
    CHECK(result.high() == 0xF0F0F0F0F0F0F0F0ULL);
    CHECK(result.low() == 0x0F0F0F0F0F0F0F0FULL);
  }
}

TEST_CASE("uint128_t - Shift operators", "[uint128]")
{
  SECTION("Left shift - within low 64 bits")
  {
    uint128_t a{0, 0x1};
    a <<= 4;
    CHECK(a.high() == 0);
    CHECK(a.low() == 0x10);
  }

  SECTION("Left shift - crossing to high bits")
  {
    uint128_t a{0, 0x1};
    a <<= 65;
    CHECK(a.high() == 0x2);
    CHECK(a.low() == 0);
  }

  SECTION("Left shift - 64 bits exactly")
  {
    uint128_t a{0, 0xFF};
    a <<= 64;
    CHECK(a.high() == 0xFF);
    CHECK(a.low() == 0);
  }

  SECTION("Left shift - overflow")
  {
    uint128_t a{0, 1};
    a <<= 128;
    CHECK(a.high() == 0);
    CHECK(a.low() == 0);
  }

  SECTION("Right shift - within low 64 bits")
  {
    uint128_t a{0, 0x100};
    a >>= 4;
    CHECK(a.high() == 0);
    CHECK(a.low() == 0x10);
  }

  SECTION("Right shift - from high to low bits")
  {
    uint128_t a{0x2, 0};
    a >>= 1;
    CHECK(a.high() == 0x1);
    CHECK(a.low() == 0);

    a >>= 64;
    CHECK(a.high() == 0);
    CHECK(a.low() == 0x1);
  }

  SECTION("Right shift - 64 bits exactly")
  {
    uint128_t a{0xFF, 0};
    a >>= 64;
    CHECK(a.high() == 0);
    CHECK(a.low() == 0xFF);
  }

  SECTION("Shift binary operators")
  {
    uint128_t a{0, 0x8};
    uint128_t left  = a << 2;
    uint128_t right = a >> 1;
    CHECK(left.low() == 0x20);
    CHECK(right.low() == 0x4);
  }
}

TEST_CASE("uint128_t - Byte conversion", "[uint128]")
{
  SECTION("Big-endian byte conversion")
  {
    uint128_t value{0x0102030405060708ULL, 0x090A0B0C0D0E0F10ULL};
    auto      bytes = value.bytes(std::endian::big);

    CHECK(bytes[0] == std::byte{0x01});
    CHECK(bytes[1] == std::byte{0x02});
    CHECK(bytes[7] == std::byte{0x08});
    CHECK(bytes[8] == std::byte{0x09});
    CHECK(bytes[15] == std::byte{0x10});

    // Round trip
    auto reconstructed = uint128_t::from_bytes(bytes, std::endian::big);
    CHECK(reconstructed == value);
  }

  SECTION("Little-endian byte conversion")
  {
    uint128_t value{0x0102030405060708ULL, 0x090A0B0C0D0E0F10ULL};
    auto      bytes = value.bytes(std::endian::little);

    CHECK(bytes[0] == std::byte{0x10});
    CHECK(bytes[1] == std::byte{0x0F});
    CHECK(bytes[7] == std::byte{0x09});
    CHECK(bytes[8] == std::byte{0x08});
    CHECK(bytes[15] == std::byte{0x01});

    // Round trip
    auto reconstructed = uint128_t::from_bytes(bytes, std::endian::little);
    CHECK(reconstructed == value);
  }

  SECTION("Native endian byte conversion")
  {
    uint128_t value{0xAABBCCDDEEFF0011ULL, 0x2233445566778899ULL};
    auto      bytes         = value.bytes(std::endian::native);
    auto      reconstructed = uint128_t::from_bytes(bytes, std::endian::native);
    CHECK(reconstructed == value);
  }
}

TEST_CASE("uint128_t - std::numeric_limits", "[uint128]")
{
  SECTION("Basic properties")
  {
    CHECK(std::numeric_limits<uint128_t>::is_specialized);
    CHECK(std::numeric_limits<uint128_t>::is_integer);
    CHECK(std::numeric_limits<uint128_t>::is_exact);
    CHECK_FALSE(std::numeric_limits<uint128_t>::is_signed);
    CHECK(std::numeric_limits<uint128_t>::is_bounded);
    CHECK(std::numeric_limits<uint128_t>::is_modulo);
    CHECK(std::numeric_limits<uint128_t>::digits == 128);
    CHECK(std::numeric_limits<uint128_t>::digits10 == 38);
  }

  SECTION("Min and max values")
  {
    auto min_val = std::numeric_limits<uint128_t>::min();
    CHECK(min_val.high() == 0);
    CHECK(min_val.low() == 0);

    auto max_val = std::numeric_limits<uint128_t>::max();
    CHECK(max_val.high() == UINT64_MAX);
    CHECK(max_val.low() == UINT64_MAX);
  }
}

TEST_CASE("uint128_t - std::hash", "[uint128]")
{
  SECTION("Hash consistency")
  {
    uint128_t a{0x123456789ABCDEF0ULL, 0xFEDCBA9876543210ULL};
    uint128_t b{0x123456789ABCDEF0ULL, 0xFEDCBA9876543210ULL};
    uint128_t c{0x123456789ABCDEF0ULL, 0xFEDCBA9876543211ULL};

    std::hash<uint128_t> hasher;
    CHECK(hasher(a) == hasher(b));
    CHECK(hasher(a) != hasher(c));
  }

  SECTION("Hash distribution")
  {
    std::hash<uint128_t> hasher;
    uint128_t            val1{0, 1};
    uint128_t            val2{0, 2};
    uint128_t            val3{1, 0};

    // Hashes should be different (not a guarantee, but very likely)
    size_t h1 = hasher(val1);
    size_t h2 = hasher(val2);
    size_t h3 = hasher(val3);

    CHECK(h1 != h2);
    CHECK(h2 != h3);
    CHECK(h1 != h3);
  }
}

TEST_CASE("uint128_t - Type traits", "[uint128]")
{
  SECTION("ouly::is_uint128")
  {
    CHECK(ouly::is_uint128_v<uint128_t>);
    CHECK_FALSE(ouly::is_uint128_v<uint64_t>);
    CHECK_FALSE(ouly::is_uint128_v<int>);
  }

  SECTION("ouly::is_extended_integral")
  {
    CHECK(ouly::is_extended_integral_v<uint128_t>);
    CHECK(ouly::is_extended_integral_v<uint64_t>);
    CHECK(ouly::is_extended_integral_v<int>);
    CHECK_FALSE(ouly::is_extended_integral_v<float>);
    CHECK_FALSE(ouly::is_extended_integral_v<double>);
  }

  SECTION("ouly::is_extended_unsigned")
  {
    CHECK(ouly::is_extended_unsigned_v<uint128_t>);
    CHECK(ouly::is_extended_unsigned_v<uint64_t>);
    CHECK(ouly::is_extended_unsigned_v<uint32_t>);
    CHECK_FALSE(ouly::is_extended_unsigned_v<int64_t>);
    CHECK_FALSE(ouly::is_extended_unsigned_v<int32_t>);
  }
}

TEST_CASE("uint128_t - Edge cases", "[uint128]")
{
  SECTION("Division by 1")
  {
    uint128_t a{0x123456789ABCDEF0ULL, 0xFEDCBA9876543210ULL};
    uint128_t b{0, 1};
    uint128_t result = a / b;
    CHECK(result == a);
  }

  SECTION("Multiplication by 0")
  {
    uint128_t a{0x123456789ABCDEF0ULL, 0xFEDCBA9876543210ULL};
    uint128_t b{0, 0};
    uint128_t result = a * b;
    CHECK(result.high() == 0);
    CHECK(result.low() == 0);
  }

  SECTION("Multiplication by 1")
  {
    uint128_t a{0x123456789ABCDEF0ULL, 0xFEDCBA9876543210ULL};
    uint128_t b{0, 1};
    uint128_t result = a * b;
    CHECK(result == a);
  }

  SECTION("XOR with self")
  {
    uint128_t a{0x123456789ABCDEF0ULL, 0xFEDCBA9876543210ULL};
    uint128_t result = a ^ a;
    CHECK(result.high() == 0);
    CHECK(result.low() == 0);
  }

  SECTION("Swap functionality")
  {
    uint128_t a{100, 200};
    uint128_t b{300, 400};
    ouly::swap(a, b);
    CHECK(a.high() == 300);
    CHECK(a.low() == 400);
    CHECK(b.high() == 100);
    CHECK(b.low() == 200);
  }
}

TEST_CASE("uint128_t - Constexpr operations", "[uint128]")
{
  SECTION("Constexpr construction and arithmetic")
  {
    constexpr uint128_t a{10, 20};
    constexpr uint128_t b{5, 10};
    constexpr uint128_t sum  = a + b;
    constexpr uint128_t diff = a - b;

    CHECK(sum.high() == 15);
    CHECK(sum.low() == 30);
    CHECK(diff.high() == 5);
    CHECK(diff.low() == 10);
  }

  SECTION("Constexpr bitwise operations")
  {
    constexpr uint128_t a{0xFF, 0xFF};
    constexpr uint128_t b{0xF0, 0xF0};
    constexpr uint128_t result = a & b;

    CHECK(result.high() == 0xF0);
    CHECK(result.low() == 0xF0);
  }

  SECTION("Constexpr shift operations")
  {
    constexpr uint128_t a{0, 1};
    constexpr uint128_t shifted = a << 8;

    CHECK(shifted.low() == 256);
  }
}

// NOLINTEND
