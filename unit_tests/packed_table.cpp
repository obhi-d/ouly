#include <acl/packed_table.hpp>
#include <catch2/catch_all.hpp>
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

TEST_CASE("packed_table: Validate packed_table emplace", "[packed_table][emplace]")
{
  acl::packed_table<int> table;

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
struct traits<std::string> : traits<>
{
  static constexpr std::uint32_t pool_size     = 2;
  static constexpr std::uint32_t idx_pool_size = 2;
};

} // namespace acl

TEST_CASE("packed_table: Custom block size", "[packed_table][page_size]")
{
  acl::packed_table<std::string> table;

  auto e1 = table.emplace("something");
  auto e2 = table.emplace("in");
  auto e3 = table.emplace("the");
  auto e4 = table.emplace("way");

  REQUIRE(table.at(e1) == "something");
  REQUIRE(table.at(e2) == "in");
  REQUIRE(table[e3] == "the");
}

TEST_CASE("packed_table: Erase on custom pages", "[packed_table][erase]")
{
  acl::packed_table<std::string> table;

  auto e1 = table.emplace("something");
  auto e2 = table.emplace("in");
  auto e3 = table.emplace("the");
  auto e4 = table.emplace("way");

  table.erase(e2);

  std::string value;
  table.for_each(
    [&value](auto l, auto const& s)
    {
      value += s + " ";
    });

  REQUIRE(table.size() == 3);
  REQUIRE(value == "something way the ");

  table.clear();
  REQUIRE(table.size() == 0);
}

TEST_CASE("packed_table: Erase pages when done", "[packed_table][shrink_to_fit]")
{
  acl::packed_table<std::string> table;

  auto e1 = table.emplace("something");
  auto e2 = table.emplace("in");
  auto e3 = table.emplace("the");
  auto e4 = table.emplace("way");

  table.erase(e3);
  table.erase(e4);

  REQUIRE(table.capacity() == 4);
  REQUIRE(table.size() == 2);
  table.shrink_to_fit();
  REQUIRE(table.capacity() == 2);
}

TEST_CASE("packed_table: Copy when copyable", "[packed_table][assignment]")
{
  acl::packed_table<std::string> table, table2;

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

TEST_CASE("packed_table: Random test", "[packed_table][random]")
{
  acl::packed_table<std::string> cont;

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
      [&](auto link, auto& el)
      {
        if (range_rand<std::uint32_t>(0, 100) > 50)
          choose.emplace(link.value());
      });
    for (auto& e : choose)
    {
      auto l = acl::packed_table<std::string>::link(e);
      erase.emplace(cont[l]);
      cont.erase(l);
    }
    cont.shrink_to_fit();
    REQUIRE(cont.size() == (count + prev) - static_cast<std::uint32_t>(erase.size()));

    cont.for_each(
      [&](auto link, auto& el)
      {
        REQUIRE(erase.find(cont.at(link)) == erase.end());
      });
  }
}

struct selfref
{
  std::uint32_t value = 0;
  std::uint32_t self  = acl::link<selfref>::null_v;

  selfref(std::uint32_t v) : value(v) {}
};

namespace acl
{
template <>
struct traits<selfref> : traits<>
{
  using offset = acl::offset<&selfref::self>;
};
} // namespace acl

TEST_CASE("packed_table: Test selfref", "[packed_table][backref]")
{
  acl::packed_table<selfref> table;

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

TEST_CASE("packed_table: Validate emplace_at", "[packed_table][emplace_at]")
{
  acl::packed_table<int> table1, table2;

  auto e10 = table1.emplace(5);
  auto e20 = table1.emplace(7);
  auto e30 = table1.emplace(11);

  table2.emplace_at(e10, 5);
  table2.emplace_at(e20, 7);
  table2.emplace_at(e30, 11);

  REQUIRE(table1.at(e10) == table2.at(e10));
  REQUIRE(table1.at(e20) == table2.at(e20));
  REQUIRE(table1.at(e30) == table2.at(e30));

  table2.erase(e10);
  table2.emplace_at(e10, 13);

  REQUIRE(table1.at(e10) == 5);
  REQUIRE(table2.at(e10) == 13);

  table2.erase(e10);
  table2.erase(e20);
  table2.emplace_at(e20, 17);

  REQUIRE(table2.contains(e10) == false);
  REQUIRE(table2.contains(e20) == true);
  REQUIRE(table2.at(e20) == 17);

  table2.erase(e20);
  table2.erase(e30);

  REQUIRE(table2.empty() == true);
}

TEST_CASE("packed_table: Validate replace", "[packed_table][replace]")
{
  acl::packed_table<int> table1;

  auto e10 = table1.emplace(5);
  auto e20 = table1.emplace(7);
  auto e30 = table1.emplace(11);

  table1.replace(e10, 13);
  table1.replace(e20, 17);
  table1.replace(e30, 19);

  REQUIRE(table1.at(e10) == 13);
  REQUIRE(table1.at(e20) == 17);
  REQUIRE(table1.at(e30) == 19);
}
