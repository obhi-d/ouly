
#include "ouly/containers/table.hpp"
#include "catch2/catch_all.hpp"
#include "ouly/containers/soavector.hpp"
#include <limits>
#include <string>

// NOLINTBEGIN

TEST_CASE("table: Validate table emplace", "[table][emplace]")
{
  ouly::table<std::string> v1;

  auto a1 = v1.emplace("first");
  auto a2 = v1.emplace("second");
  auto a3 = v1.emplace("third");

  REQUIRE(v1[a1] == "first");
  REQUIRE(v1[a2] == "second");
  REQUIRE(v1[a3] == "third");
}

TEST_CASE("table: Validate table erase", "[table][erase]")
{
  // using pack = ouly::pack<int, bool, std::string>;
  ouly::table<std::string> v1;

  auto                  a1 = v1.emplace("first");
  [[maybe_unused]] auto a2 = v1.emplace("second");
  auto                  a3 = v1.emplace("third");
  auto                  a4 = v1.emplace("fourth");
  [[maybe_unused]] auto a5 = v1.emplace("fifth");
  auto                  a6 = v1.emplace("sixth");

  v1.erase(a1);
  REQUIRE(a1 == v1.emplace("first"));

  v1.erase(a3);
  v1.erase(a4);
  v1.erase(a6);

  a3 = v1.emplace("third");
  a4 = v1.emplace("fourth");
  a6 = v1.emplace("sixth");

  v1.erase(a4);

  a4 = v1.emplace("jerry");
  REQUIRE(v1[a4] == "jerry");
}

TEST_CASE("table: Test swap functionality", "[table][swap]")
{
  ouly::table<std::string> table1;
  auto                     a1 = table1.emplace("first");
  auto                     a2 = table1.emplace("second");

  ouly::table<std::string> table2;
  auto                     b1 = table2.emplace("third");
  auto                     b2 = table2.emplace("fourth");
  auto                     b3 = table2.emplace("fifth");

  // Store original values
  std::string   val_a1 = table1[a1];
  std::string   val_a2 = table1[a2];
  std::string   val_b1 = table2[b1];
  std::string   val_b2 = table2[b2];
  std::string   val_b3 = table2[b3];
  std::uint32_t size1  = table1.size();
  std::uint32_t size2  = table2.size();

  // Test member swap
  table1.swap(table2);

  // Verify sizes are swapped
  REQUIRE(table1.size() == size2);
  REQUIRE(table2.size() == size1);

  // Verify values are swapped (note: indices may be reused)
  REQUIRE(table1[b1] == val_b1);
  REQUIRE(table1[b2] == val_b2);
  REQUIRE(table1[b3] == val_b3);
  REQUIRE(table2[a1] == val_a1);
  REQUIRE(table2[a2] == val_a2);

  // Test friend swap function (swap back)
  swap(table1, table2);

  // Verify we're back to original state
  REQUIRE(table1.size() == size1);
  REQUIRE(table2.size() == size2);
  REQUIRE(table1[a1] == val_a1);
  REQUIRE(table1[a2] == val_a2);
  REQUIRE(table2[b1] == val_b1);
  REQUIRE(table2[b2] == val_b2);
  REQUIRE(table2[b3] == val_b3);
}

struct pod_entry
{
  std::uint32_t id;
  std::uint32_t next;
};

TEST_CASE("table: POD erase with projection reuses indices", "[table][erase][projection]")
{
  ouly::table<pod_entry> table;

  auto first  = table.emplace(pod_entry{1U, 10U});
  auto second = table.emplace(pod_entry{2U, 20U});
  auto third  = table.emplace(pod_entry{3U, 30U});

  REQUIRE(table.size() == 3);

  table.erase(second, &pod_entry::next);
  REQUIRE(table.size() == 2);
  REQUIRE(table[second].next == std::numeric_limits<std::uint32_t>::max());

  table.erase(third, &pod_entry::next);
  REQUIRE(table.size() == 1);
  REQUIRE(table[third].next == second);

  auto reuse_third = table.emplace(pod_entry{4U, 40U});
  REQUIRE(reuse_third == third);
  REQUIRE(table.size() == 2);
  REQUIRE(table[reuse_third].id == 4U);
  REQUIRE(table[reuse_third].next == 40U);

  auto reuse_second = table.emplace(pod_entry{5U, 50U});
  REQUIRE(reuse_second == second);
  REQUIRE(table.size() == 3);
  REQUIRE(table[reuse_second].id == 5U);
  REQUIRE(table[reuse_second].next == 50U);

  REQUIRE(table[first].id == 1U);
  REQUIRE(table[first].next == 10U);
}

TEST_CASE("table: Projection policy stores free indices in member", "[table][erase][projection][policy]")
{
  ouly::table<pod_entry, &pod_entry::next> table;

  auto first  = table.emplace(pod_entry{1U, 10U});
  auto second = table.emplace(pod_entry{2U, 20U});
  auto third  = table.emplace(pod_entry{3U, 30U});

  REQUIRE(table.size() == 3);

  table.erase(second);
  REQUIRE(table.size() == 2);
  REQUIRE(table[second].next == std::numeric_limits<std::uint32_t>::max());

  table.erase(third);
  REQUIRE(table.size() == 1);
  REQUIRE(table[third].next == second);

  auto reuse_third = table.emplace(pod_entry{4U, 40U});
  REQUIRE(reuse_third == third);
  REQUIRE(table.size() == 2);
  REQUIRE(table[reuse_third].id == 4U);
  REQUIRE(table[reuse_third].next == 40U);

  auto reuse_second = table.emplace(pod_entry{5U, 50U});
  REQUIRE(reuse_second == second);
  REQUIRE(table.size() == 3);
  REQUIRE(table[reuse_second].id == 5U);
  REQUIRE(table[reuse_second].next == 50U);

  REQUIRE(table[first].id == 1U);
  REQUIRE(table[first].next == 10U);
}

// NOLINTEND
