#include "ouly/containers/sparse_table.hpp"
#include "catch2/catch_all.hpp"
#include "test_common.hpp"
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

// NOLINTBEGIN
TEST_CASE("sparse_table: Validate sparse_table emplace", "[sparse_table][emplace]")
{
  ouly::sparse_table<int> table;

  auto e10 = table.emplace(10);
  auto e20 = table.emplace(20);
  auto e30 = table.emplace(30);

  REQUIRE(table.at(e10) == 10);
  REQUIRE(table.at(e20) == 20);
  REQUIRE(table.at(e30) == 30);
}

TEST_CASE("sparse_table: Custom block size", "[sparse_table][page_size]")
{
  ouly::sparse_table<std::string> table;

  auto                  e1 = table.emplace("something");
  auto                  e2 = table.emplace("in");
  auto                  e3 = table.emplace("the");
  [[maybe_unused]] auto e4 = table.emplace("way");

  REQUIRE(table.at(e1) == "something");
  REQUIRE(table.at(e2) == "in");
  REQUIRE(table[e3] == "the");
}

TEST_CASE("sparse_table: Erase pages when done", "[sparse_table][shrink_to_fit]")
{
  ouly::sparse_table<std::string> table;

  [[maybe_unused]] auto e1 = table.emplace("something");
  [[maybe_unused]] auto e2 = table.emplace("in");
  [[maybe_unused]] auto e3 = table.emplace("the");
  [[maybe_unused]] auto e4 = table.emplace("way");

  table.erase(e3);
  table.erase(e4);

  REQUIRE(table.size() == 2);
  table.shrink_to_fit();
}

TEST_CASE("sparse_table: Copy when copyable", "[sparse_table][assignment]")
{
  ouly::sparse_table<std::string> table, table2;

  [[maybe_unused]] auto e1 = table.emplace("something");
  [[maybe_unused]] auto e2 = table.emplace("in");
  [[maybe_unused]] auto e3 = table.emplace("the");
  [[maybe_unused]] auto e4 = table.emplace("way");

  table2 = table;

  [[maybe_unused]] auto& el1 = table2.at(e1);
  [[maybe_unused]] auto& el2 = table2.at(e2);
  REQUIRE(table2.at(e1) == "something");
  REQUIRE(table2.at(e2) == "in");
  REQUIRE(table2[e3] == "the");
}

TEST_CASE("sparse_table: Random test", "[sparse_table][random]")
{
  ouly::sparse_table<std::string> cont;

  std::uint32_t last_offset = 0;
  for (int times = 0; times < 4; times++)
  {
    std::uint32_t prev = cont.size();
    // Insert items
    std::uint32_t count = range_rand<std::uint32_t>(10, 1000);
    // insertion
    helper::insert(cont, last_offset + 0, count);
    REQUIRE(cont.size() == count + prev);
    // emplace
    last_offset += count;

    std::unordered_set<std::string>   erase;
    std::unordered_set<std::uint32_t> choose;
    cont.for_each(
     [&](ouly::sparse_table<std::string>::link link, [[maybe_unused]] std::string& el)
     {
       if (range_rand<std::uint32_t>(0, 100) > 50)
         choose.emplace(link);
     });
    for (auto& e : choose)
    {
      auto l = ouly::sparse_table<std::string>::link(e);
      erase.emplace(cont[l]);
      cont.erase(l);
    }
    cont.shrink_to_fit();
    REQUIRE(cont.size() == (count + prev) - static_cast<std::uint32_t>(erase.size()));

    cont.for_each(
     [&](ouly::sparse_table<std::string>::link link, [[maybe_unused]] std::string& el)
     {
       REQUIRE(erase.find(cont.at(link)) == erase.end());
     });
  }
}

struct selfref_2
{
  std::uint32_t value = 0;
  std::uint32_t self  = {};

  selfref_2(std::uint32_t v) : value(v) {}
};

template <>
struct ouly::default_config<selfref_2> : public ouly::cfg::self_index_member<&selfref_2::self>
{};

TEST_CASE("sparse_table: Test selfref", "[sparse_table][backref]")
{
  ouly::sparse_table<selfref_2> table;

  auto e10 = table.emplace(10);

  REQUIRE(table.at(e10).value == 10);
  table.erase(e10);

  auto e20 = table.emplace(20);
  auto e30 = table.emplace(30);

  REQUIRE(table.at(e20).value == 20);
  REQUIRE(table.at(e20).self == e20);
  REQUIRE(table.at(e30).value == 30);
  REQUIRE(table.at(e30).self == e30);
}

TEST_CASE("sparse_table: Validate replace", "[sparse_table][replace]")
{
  ouly::sparse_table<int> table1;

  auto e10 = table1.emplace(5);
  auto e20 = table1.emplace(7);
  auto e30 = table1.emplace(11);

  table1.replace(e10, 13);
  table1.replace(e20, 17);
  table1.replace(e30, 19);

  REQUIRE(table1.at(e10) == 13);
  REQUIRE(table1.at(e20) == 17);
  REQUIRE(table1.at(e30) == 19);

  REQUIRE(table1.contains(e10) == true);
  REQUIRE(table1.contains(e20) == true);
  REQUIRE(table1.contains(e30) == true);
  REQUIRE(table1.get_if(e10) != nullptr);
  REQUIRE(table1.get_if(e20) != nullptr);

  table1.erase(e10);

  REQUIRE(table1.get_if(e10) == nullptr);

  table1.erase(e20);
  table1.erase(e30);

  REQUIRE(table1.empty() == true);
}

TEST_CASE("sparse_table: Test swap functionality", "[sparse_table][swap]")
{
  ouly::sparse_table<int> table1;
  auto                    e10 = table1.emplace(100);
  auto                    e20 = table1.emplace(200);

  ouly::sparse_table<int> table2;
  auto                    e30 = table2.emplace(300);
  auto                    e40 = table2.emplace(400);
  auto                    e50 = table2.emplace(500);

  // Store original values for verification
  int         val10 = table1.at(e10);
  int         val20 = table1.at(e20);
  int         val30 = table2.at(e30);
  int         val40 = table2.at(e40);
  int         val50 = table2.at(e50);
  std::size_t size1 = table1.size();
  std::size_t size2 = table2.size();

  // Test member swap
  table1.swap(table2);

  // Verify sizes are swapped
  REQUIRE(table1.size() == size2);
  REQUIRE(table2.size() == size1);

  // Note: Links are internal and may not be preserved across swap
  // So we verify by counting and checking if all values are present
  int                     count1 = 0, count2 = 0;
  std::unordered_set<int> values1, values2;

  table1.for_each(
   [&](int const& val)
   {
     count1++;
     values1.insert(val);
   });

  table2.for_each(
   [&](int const& val)
   {
     count2++;
     values2.insert(val);
   });

  REQUIRE(count1 == 3);
  REQUIRE(count2 == 2);
  REQUIRE(values1.count(val30) == 1);
  REQUIRE(values1.count(val40) == 1);
  REQUIRE(values1.count(val50) == 1);
  REQUIRE(values2.count(val10) == 1);
  REQUIRE(values2.count(val20) == 1);

  // Test friend swap function (swap back)
  swap(table1, table2);

  // Verify we're back to original state
  REQUIRE(table1.size() == size1);
  REQUIRE(table2.size() == size2);

  count1 = count2 = 0;
  values1.clear();
  values2.clear();

  table1.for_each(
   [&](int const& val)
   {
     count1++;
     values1.insert(val);
   });

  table2.for_each(
   [&](int const& val)
   {
     count2++;
     values2.insert(val);
   });

  REQUIRE(count1 == 2);
  REQUIRE(count2 == 3);
  REQUIRE(values1.count(val10) == 1);
  REQUIRE(values1.count(val20) == 1);
  REQUIRE(values2.count(val30) == 1);
  REQUIRE(values2.count(val40) == 1);
  REQUIRE(values2.count(val50) == 1);
}
// NOLINTEND