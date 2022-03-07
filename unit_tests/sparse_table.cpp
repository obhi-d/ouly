#include <acl/sparse_table.hpp>
#include <catch2/catch.hpp>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

template <typename IntTy>
IntTy range_rand(IntTy iBeg, IntTy iEnd)
{
  return static_cast<IntTy>(iBeg + (((double)rand() / (double)RAND_MAX) * (iEnd - iBeg)));
}

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

namespace acl
{
template <>
struct pool_traits<std::string> : pool_traits<>
{
  static constexpr std::uint32_t pool_size     = 2;
  static constexpr std::uint32_t idx_pool_size = 2;
};
} // namespace acl

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

  table.remove(e3);
  table.remove(e4);

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

  REQUIRE(table2.at(e1) == "something");
  REQUIRE(table2.at(e2) == "in");
  REQUIRE(table2[e3] == "the");
}

namespace helper
{
template <typename Cont>
static void insert(Cont& cont, std::uint32_t offset, std::uint32_t count)
{
  for (std::uint32_t i = 0; i < count; ++i)
  {
    cont.emplace(std::to_string(i + offset) + ".o");
  }
}
}; // namespace helper

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

    std::unordered_set<std::string>   remove;
    std::unordered_set<std::uint32_t> choose;
    cont.for_each(
      [&](auto link, auto& el)
      {
        if (range_rand<std::uint32_t>(0, 100) > 50)
          choose.emplace(link.value());
      });
    for (auto& e : choose)
    {
      auto l = acl::sparse_table<std::string>::link(e);
      remove.emplace(cont[l]);
      cont.remove(l);
    }
    cont.shrink_to_fit();
    REQUIRE(cont.size() == (count + prev) - static_cast<std::uint32_t>(remove.size()));

    cont.for_each(
      [&](auto link, auto& el)
      {
        REQUIRE(remove.find(cont.at(link)) == remove.end());
      });
  }
}

struct selfref_2
{
  std::uint32_t value = 0;
  std::uint32_t self  = acl::link<selfref_2>::null;

  selfref_2(std::uint32_t v) : value(v) {}
};

namespace acl
{
template <>
struct pool_traits<selfref_2> : pool_traits<>
{
  using offset = acl::offset<&selfref_2::self>;
};
} // namespace acl

TEST_CASE("sparse_table: Test selfref", "[sparse_table][backref]")
{
  acl::sparse_table<selfref_2> table;

  auto e10 = table.emplace(10);

  REQUIRE(table.at(e10).value == 10);
  table.remove(e10);

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

  table1.remove(e10);
  table1.remove(e20);
  table1.remove(e30);

  REQUIRE(table1.empty() == true);
}
