#include "test_common.hpp"
#include <acl/containers/sparse_table.hpp>
#include <catch2/catch_all.hpp>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

// NOLINTBEGIN
TEST_CASE("sparse_table: Validate sparse_table emplace", "[sparse_table][emplace]")
{
  acl::sparse_table<int> table;

  auto e10 = table.emplace(10);
  auto e20 = table.emplace(20);
  auto e30 = table.emplace(30);

  REQUIRE(table.at(e10) == 10);
  REQUIRE(table.at(e20) == 20);
  REQUIRE(table.at(e30) == 30);
}

TEST_CASE("sparse_table: Custom block size", "[sparse_table][page_size]")
{
  acl::sparse_table<std::string> table;

  auto e1 = table.emplace("something");
  auto e2 = table.emplace("in");
  auto e3 = table.emplace("the");
  auto e4 = table.emplace("way");

  REQUIRE(table.at(e1) == "something");
  REQUIRE(table.at(e2) == "in");
  REQUIRE(table[e3] == "the");
}

TEST_CASE("sparse_table: Erase pages when done", "[sparse_table][shrink_to_fit]")
{
  acl::sparse_table<std::string> table;

  auto e1 = table.emplace("something");
  auto e2 = table.emplace("in");
  auto e3 = table.emplace("the");
  auto e4 = table.emplace("way");

  table.erase(e3);
  table.erase(e4);

  REQUIRE(table.size() == 2);
  table.shrink_to_fit();
}

TEST_CASE("sparse_table: Copy when copyable", "[sparse_table][assignment]")
{
  acl::sparse_table<std::string> table, table2;

  auto e1 = table.emplace("something");
  auto e2 = table.emplace("in");
  auto e3 = table.emplace("the");
  auto e4 = table.emplace("way");

  table2 = table;

  auto& el1 = table2.at(e1);
  auto& el2 = table2.at(e2);
  REQUIRE(table2.at(e1) == "something");
  REQUIRE(table2.at(e2) == "in");
  REQUIRE(table2[e3] == "the");
}

TEST_CASE("sparse_table: Random test", "[sparse_table][random]")
{
  acl::sparse_table<std::string> cont;

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
     [&](acl::sparse_table<std::string>::link link, std::string& el)
     {
       if (range_rand<std::uint32_t>(0, 100) > 50)
         choose.emplace(link.value());
     });
    for (auto& e : choose)
    {
      auto l = acl::sparse_table<std::string>::link(e);
      erase.emplace(cont[l]);
      cont.erase(l);
    }
    cont.shrink_to_fit();
    REQUIRE(cont.size() == (count + prev) - static_cast<std::uint32_t>(erase.size()));

    cont.for_each(
     [&](acl::sparse_table<std::string>::link link, std::string& el)
     {
       REQUIRE(erase.find(cont.at(link)) == erase.end());
     });
  }
}

struct selfref_2
{
  std::uint32_t value = 0;
  std::uint32_t self  = acl::link<selfref_2>::null_v;

  selfref_2(std::uint32_t v) : value(v) {}
};

template <>
struct acl::default_config<selfref_2> : public acl::cfg::self_index_member<&selfref_2::self>
{};

TEST_CASE("sparse_table: Test selfref", "[sparse_table][backref]")
{
  acl::sparse_table<selfref_2> table;

  auto e10 = table.emplace(10);

  REQUIRE(table.at(e10).value == 10);
  table.erase(e10);

  auto e20 = table.emplace(20);
  auto e30 = table.emplace(30);

  REQUIRE(table.at(e20).value == 20);
  REQUIRE(table.at(e20).self == e20.value());
  REQUIRE(table.at(e30).value == 30);
  REQUIRE(table.at(e30).self == e30.value());
}

TEST_CASE("sparse_table: Validate replace", "[sparse_table][replace]")
{
  acl::sparse_table<int> table1;

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
// NOLINTEND