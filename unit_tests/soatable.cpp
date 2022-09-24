
#include <acl/soatable.hpp>
#include <catch2/catch.hpp>
#include <string>

template <typename IntTy>
IntTy range_rand(IntTy iBeg, IntTy iEnd)
{
  return static_cast<IntTy>(iBeg + (((double)rand() / (double)RAND_MAX) * (iEnd - iBeg)));
}

TEST_CASE("soatable: Validate soatable emplace", "[soatable][emplace]")
{
  acl::soatable<acl::pack<int, bool>> table;
  auto                                link = table.emplace(10, true);
  CHECK(std::get<0>(table[link]) == 10);
  CHECK(std::get<1>(table[link]) == true);

  auto link2 = table.emplace(20, false);
  auto link3 = table.emplace(30, false);
  auto link4 = table.emplace(40, true);
  table.remove(link3);
  CHECK(std::get<0>(table[link2]) == 20);
  CHECK(std::get<1>(table[link2]) == false);
  CHECK(std::get<0>(table[link4]) == 40);
  CHECK(std::get<1>(table[link4]) == true);
  table.emplace_at(link3, 30, false);
  CHECK(std::get<0>(table[link3]) == 30);
  CHECK(std::get<1>(table[link3]) == false);
  table.replace(link3, 50, true);
  CHECK(std::get<0>(table[link3]) == 50);
  CHECK(std::get<1>(table[link3]) == true);
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

TEST_CASE("soatable: Validate soatable for_each", "[soatable][for_each]")
{
  acl::soatable<std::string> table;
  auto                       e1 = table.emplace("something");
  auto                       e2 = table.emplace("in");
  auto                       e3 = table.emplace("the");
  auto                       e4 = table.emplace("way");

  table.remove(e2);

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

TEST_CASE("soatable: Erase pages when done", "[soatable][shrink_to_fit]")
{
  acl::soatable<std::string> table;

  auto e1 = table.emplace("something");
  auto e2 = table.emplace("in");
  auto e3 = table.emplace("the");
  auto e4 = table.emplace("way");

  table.remove(e3);
  table.remove(e4);

  REQUIRE(table.capacity() == 4);
  REQUIRE(table.size() == 2);
  table.shrink_to_fit();
  REQUIRE(table.capacity() == 2);
}

TEST_CASE("soatable: Copy when copyable", "[soatable][assignment]")
{
  acl::soatable<std::string> table, table2;

  auto e1 = table.emplace("something");
  auto e2 = table.emplace("in");
  auto e3 = table.emplace("the");
  auto e4 = table.emplace("way");

  table2 = table;

  REQUIRE(table2.get<0>(e1) == "something");
  REQUIRE(table2.get<0>(e2) == "in");
  REQUIRE(table2.get<0>(e3) == "the");
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

TEST_CASE("soatable: Random test", "[soatable][random]")
{
  acl::soatable<std::string> cont;

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
      [&](auto link, auto const& el)
      {
        if (range_rand<std::uint32_t>(0, 100) > 50)
          choose.emplace(link.value());
      });
    for (auto& e : choose)
    {
      auto l = acl::soatable<std::string>::link(e);
      remove.emplace(cont.get<0>(l));
      cont.remove(l);
    }
    cont.shrink_to_fit();
    REQUIRE(cont.size() == (count + prev) - static_cast<std::uint32_t>(remove.size()));

    cont.for_each(
      [&](auto link, auto const& el)
      {
        REQUIRE(remove.find(cont.get<0>(link)) == remove.end());
      });
  }
}

struct selfref
{
  std::uint32_t value = 0;
  std::uint32_t self  = acl::link<selfref>::null_v;

  selfref(std::uint32_t v) : value(v) {}
};

TEST_CASE("soatable: Test selfref", "[soatable][backref]")
{
  acl::soatable<selfref> table;

  auto e10 = table.emplace(10);

  REQUIRE(table.get<0>(e10).value == 10);
  table.remove(e10);

  auto e20 = table.emplace(20);
  auto e30 = table.emplace(30);

  REQUIRE(table.get<0>(e20).value == 20);
  REQUIRE(table.get<0>(e30).value == 30);
}

TEST_CASE("soatable: Validate emplace_at", "[soatable][emplace_at]")
{
  acl::soatable<int> table1, table2;

  auto e10 = table1.emplace(5);
  auto e20 = table1.emplace(7);
  auto e30 = table1.emplace(11);

  table2.emplace_at(e10, 5);
  table2.emplace_at(e20, 7);
  table2.emplace_at(e30, 11);

  REQUIRE(table1.get<0>(e10) == table2.get<0>(e10));
  REQUIRE(table1.get<0>(e20) == table2.get<0>(e20));
  REQUIRE(table1.get<0>(e30) == table2.get<0>(e30));

  table2.remove(e10);
  table2.emplace_at(e10, 13);

  REQUIRE(table1.get<0>(e10) == 5);
  REQUIRE(table2.get<0>(e10) == 13);

  table2.remove(e10);
  table2.remove(e20);
  table2.emplace_at(e20, 17);

  REQUIRE(table2.contains(e10) == false);
  REQUIRE(table2.contains(e20) == true);
  REQUIRE(table2.get<0>(e20) == 17);

  table2.remove(e20);
  table2.remove(e30);

  REQUIRE(table2.empty() == true);
}

TEST_CASE("soatable: Validate replace", "[soatable][replace]")
{
  acl::soatable<int> table1;

  auto e10 = table1.emplace(5);
  auto e20 = table1.emplace(7);
  auto e30 = table1.emplace(11);

  table1.replace(e10, 13);
  table1.replace(e20, 17);
  table1.replace(e30, 19);

  REQUIRE(table1.get<0>(e10) == 13);
  REQUIRE(table1.get<0>(e20) == 17);
  REQUIRE(table1.get<0>(e30) == 19);
}
