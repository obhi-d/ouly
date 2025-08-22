#include "ouly/containers/index_map.hpp"
#include "catch2/catch_all.hpp"

// NOLINTBEGIN

TEST_CASE("index_map: Basic functionality", "[index_map][basic]")
{
  ouly::index_map<uint32_t> map;

  REQUIRE(map.empty() == true);
  REQUIRE(map.size() == 0);

  map[5]  = 100;
  map[10] = 200;
  map[15] = 300;

  REQUIRE(map.empty() == false);
  REQUIRE(map.size() == 11);
  REQUIRE(map[5] == 100);
  REQUIRE(map[10] == 200);
  REQUIRE(map[15] == 300);

  REQUIRE(map.contains(5) == true);
  REQUIRE(map.contains(10) == true);
  REQUIRE(map.contains(15) == true);
  REQUIRE(map.contains(20) == false);

  REQUIRE(map.find(5) == 100);
  REQUIRE(map.find(20) == ouly::index_map<uint32_t>::null);
}

TEST_CASE("index_map: Test swap functionality", "[index_map][swap]")
{
  ouly::index_map<uint32_t> map1;
  map1[5]  = 100;
  map1[10] = 200;

  ouly::index_map<uint32_t> map2;
  map2[15] = 300;
  map2[20] = 400;
  map2[25] = 500;

  // Store original values and sizes
  auto     size1   = map1.size();
  auto     size2   = map2.size();
  uint32_t val1_5  = map1[5];
  uint32_t val1_10 = map1[10];
  uint32_t val2_15 = map2[15];
  uint32_t val2_20 = map2[20];
  uint32_t val2_25 = map2[25];

  // Test member swap
  map1.swap(map2);

  // Verify sizes are swapped
  REQUIRE(map1.size() == size2);
  REQUIRE(map2.size() == size1);

  // Verify contents are swapped
  REQUIRE(map1.contains(15) == true);
  REQUIRE(map1.contains(20) == true);
  REQUIRE(map1.contains(25) == true);
  REQUIRE(map1.contains(5) == false);
  REQUIRE(map1.contains(10) == false);

  REQUIRE(map2.contains(5) == true);
  REQUIRE(map2.contains(10) == true);
  REQUIRE(map2.contains(15) == false);
  REQUIRE(map2.contains(20) == false);
  REQUIRE(map2.contains(25) == false);

  REQUIRE(map1[15] == val2_15);
  REQUIRE(map1[20] == val2_20);
  REQUIRE(map1[25] == val2_25);

  REQUIRE(map2[5] == val1_5);
  REQUIRE(map2[10] == val1_10);

  // Test friend swap function (swap back)
  swap(map1, map2);

  // Verify we're back to original state
  REQUIRE(map1.size() == size1);
  REQUIRE(map2.size() == size2);

  REQUIRE(map1[5] == val1_5);
  REQUIRE(map1[10] == val1_10);
  REQUIRE(map2[15] == val2_15);
  REQUIRE(map2[20] == val2_20);
  REQUIRE(map2[25] == val2_25);
}

TEST_CASE("index_map: Test swap with offset limit", "[index_map][swap][offset]")
{
  ouly::index_map<uint32_t, 8> map1;
  map1[100] = 1000;
  map1[102] = 1020;

  ouly::index_map<uint32_t, 8> map2;
  map2[200] = 2000;
  map2[205] = 2050;
  map2[210] = 2100;

  // Store original size info and values (not sizes as they may change due to offset logic)
  uint32_t val1_100 = map1[100];
  uint32_t val1_102 = map1[102];
  uint32_t val2_200 = map2[200];
  uint32_t val2_205 = map2[205];
  uint32_t val2_210 = map2[210];

  // Test member swap
  map1.swap(map2);

  // Verify contents are swapped (values should be preserved)
  REQUIRE(map1[200] == val2_200);
  REQUIRE(map1[205] == val2_205);
  REQUIRE(map1[210] == val2_210);

  REQUIRE(map2[100] == val1_100);
  REQUIRE(map2[102] == val1_102);

  // Verify that the right indices are accessible
  REQUIRE(map1.contains(200) == true);
  REQUIRE(map1.contains(205) == true);
  REQUIRE(map1.contains(210) == true);
  REQUIRE(map1.contains(100) == false);
  REQUIRE(map1.contains(102) == false);

  REQUIRE(map2.contains(100) == true);
  REQUIRE(map2.contains(102) == true);
  REQUIRE(map2.contains(200) == false);
  REQUIRE(map2.contains(205) == false);
  REQUIRE(map2.contains(210) == false);

  // Test friend swap function
  swap(map1, map2);

  // Verify we're back to original state
  REQUIRE(map1[100] == val1_100);
  REQUIRE(map1[102] == val1_102);
  REQUIRE(map2[200] == val2_200);
  REQUIRE(map2[205] == val2_205);
  REQUIRE(map2[210] == val2_210);
}

// NOLINTEND
