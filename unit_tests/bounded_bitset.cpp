#include "ouly/containers/bounded_bitset.hpp"
#include "catch2/catch_all.hpp"
#include <vector>

// NOLINTBEGIN

TEST_CASE("bounded_bitset: stores high bits with compact offset", "[bounded_bitset][basic]")
{
  ouly::bounded_bitset<uint32_t> bits;

  REQUIRE(bits.empty());
  REQUIRE(bits.size() == 0);
  REQUIRE(bits.base_offset() == ouly::bounded_bitset<uint32_t>::null);

  bits.set(1024);
  bits.set(1031);

  REQUIRE_FALSE(bits.empty());
  REQUIRE(bits.storage_size() == 1);
  REQUIRE(bits.size() == ouly::bounded_bitset<uint32_t>::bits_per_word);
  REQUIRE(bits.base_offset() == 1024);

  REQUIRE(bits.test(1024));
  REQUIRE(bits.test(1031));
  REQUIRE_FALSE(bits.test(1023));
  REQUIRE_FALSE(bits.test(1030));

  REQUIRE(bits.contains(1024));
  REQUIRE(bits.contains(1087));
  REQUIRE_FALSE(bits.contains(1023));
  REQUIRE_FALSE(bits.contains(1088));
}

TEST_CASE("bounded_bitset: shifts base to lower words while under offset limit", "[bounded_bitset][offset]")
{
  ouly::bounded_bitset<uint32_t, 4> bits;

  bits.set(256);
  bits.set(192);

  REQUIRE(bits.storage_size() == 2);
  REQUIRE(bits.base_offset() == 192);
  REQUIRE(bits.test(192));
  REQUIRE(bits.test(256));
  REQUIRE_FALSE(bits.test(255));
}

TEST_CASE("bounded_bitset: falls back to zero base after offset limit is reached", "[bounded_bitset][offset]")
{
  ouly::bounded_bitset<uint32_t, 1> bits;

  bits.set(256);
  bits.set(192);
  bits.set(64);

  REQUIRE(bits.base_offset() == 0);
  REQUIRE(bits.test(64));
  REQUIRE(bits.test(192));
  REQUIRE(bits.test(256));
  REQUIRE(bits.storage_size() == 5);
}

TEST_CASE("bounded_bitset: reset trims empty edge words", "[bounded_bitset][trim]")
{
  ouly::bounded_bitset<uint32_t> bits;

  bits.set(1024);
  bits.set(2048);
  REQUIRE(bits.storage_size() == 17);
  REQUIRE(bits.base_offset() == 1024);

  bits.reset(1024);
  REQUIRE(bits.storage_size() == 1);
  REQUIRE(bits.base_offset() == 2048);
  REQUIRE_FALSE(bits.test(1024));
  REQUIRE(bits.test(2048));

  bits.reset(2048);
  REQUIRE(bits.empty());
  REQUIRE(bits.base_offset() == ouly::bounded_bitset<uint32_t>::null);
}

TEST_CASE("bounded_bitset: zero offset mode behaves like a regular vector bitset", "[bounded_bitset][zero_offset]")
{
  ouly::bounded_bitset<uint32_t, 0> bits;

  bits.set(130);

  REQUIRE(bits.base_offset() == 0);
  REQUIRE(bits.storage_size() == 3);
  REQUIRE(bits.test(130));
  REQUIRE_FALSE(bits.test(129));

  bits.reset(130);
  REQUIRE(bits.empty());
  REQUIRE(bits.base_offset() == 0);
}

TEST_CASE("bounded_bitset: for_each visits set bits and skips empty words", "[bounded_bitset][for_each]")
{
  ouly::bounded_bitset<uint32_t> bits;

  bits.set(1024);
  bits.set(1025);
  bits.set(1152);
  bits.set(1183);

  std::vector<uint32_t> indices;
  bits.for_each(
   [&](uint32_t idx)
   {
     indices.push_back(idx);
   });

  REQUIRE(indices == std::vector<uint32_t>{1024, 1025, 1152, 1183});
}

TEST_CASE("bounded_bitset: for_each handles empty and zero-offset bitsets", "[bounded_bitset][for_each]")
{
  ouly::bounded_bitset<uint32_t> empty_bits;
  uint32_t                       calls = 0;

  empty_bits.for_each(
   [&](uint32_t)
   {
     ++calls;
   });
  REQUIRE(calls == 0);

  ouly::bounded_bitset<uint32_t, 0> bits;
  bits.set(1);
  bits.set(130);

  std::vector<uint32_t> indices;
  bits.for_each(
   [&](uint32_t idx)
   {
     indices.push_back(idx);
   });

  REQUIRE(indices == std::vector<uint32_t>{1, 130});
}

TEST_CASE("bounded_bitset: SIMD aliases preserve for_each behavior", "[bounded_bitset][for_each][simd]")
{
  ouly::bounded_bitset_sse2<uint32_t> sse_bits;
  ouly::bounded_bitset_avx2<uint32_t> avx_bits;

  for (auto idx : {1024U, 1088U, 4096U, 4103U})
  {
    sse_bits.set(idx);
    avx_bits.set(idx);
  }

  std::vector<uint32_t> sse_indices;
  std::vector<uint32_t> avx_indices;

  sse_bits.for_each(
   [&](uint32_t idx)
   {
     sse_indices.push_back(idx);
   });
  avx_bits.for_each(
   [&](uint32_t idx)
   {
     avx_indices.push_back(idx);
   });

  REQUIRE(sse_indices == std::vector<uint32_t>{1024, 1088, 4096, 4103});
  REQUIRE(avx_indices == sse_indices);
}

TEST_CASE("bounded_bitset: SIMD aliases support 32-bit storage words", "[bounded_bitset][for_each][simd]")
{
  ouly::bounded_bitset_sse2<uint32_t, ouly::bounded_bitset_default_offset_limit, uint32_t> bits;

  bits.set(1024);
  bits.set(1055);
  bits.set(1184);

  std::vector<uint32_t> indices;
  bits.for_each(
   [&](uint32_t idx)
   {
     indices.push_back(idx);
   });

  REQUIRE(indices == std::vector<uint32_t>{1024, 1055, 1184});
}

TEST_CASE("bounded_bitset: swap exchanges storage and offsets", "[bounded_bitset][swap]")
{
  ouly::bounded_bitset<uint32_t> bits1;
  bits1.set(1024);
  bits1.set(1030);

  ouly::bounded_bitset<uint32_t> bits2;
  bits2.set(2048);
  bits2.set(2050);

  auto const offset1 = bits1.base_offset();
  auto const offset2 = bits2.base_offset();

  bits1.swap(bits2);

  REQUIRE(bits1.base_offset() == offset2);
  REQUIRE(bits2.base_offset() == offset1);
  REQUIRE(bits1.test(2048));
  REQUIRE(bits1.test(2050));
  REQUIRE_FALSE(bits1.test(1024));
  REQUIRE(bits2.test(1024));
  REQUIRE(bits2.test(1030));
  REQUIRE_FALSE(bits2.test(2048));

  swap(bits1, bits2);

  REQUIRE(bits1.base_offset() == offset1);
  REQUIRE(bits2.base_offset() == offset2);
  REQUIRE(bits1.test(1024));
  REQUIRE(bits2.test(2048));
}

// NOLINTEND
