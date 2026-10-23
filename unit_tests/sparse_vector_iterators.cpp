#include "catch2/catch_all.hpp" // NOLINT(misc-include-cleaner)
#include "ouly/containers/sparse_vector.hpp"
#include "test_common.hpp"

// NOLINTBEGIN
template <>
struct ouly::default_config<pod>
{
  static constexpr std::uint32_t pool_size_v  = 16;
  static constexpr pod           null_v       = pod{};
  static constexpr bool          assume_pod_v = true;
};

TEST_CASE("sparse_vector: range-based iteration skips holes", "[sparse_vector][iterators]")
{
  ouly::sparse_vector<pod> v;
  // Emplace sparse entries
  v.emplace_at(1, 10, 20);
  v.emplace_at(5, 50, 60);
  v.emplace_at(32, 320, 640);

  // There are holes between indices; iterator should skip them
  std::vector<int> seen;
  for (auto const& x : v)
  {
    seen.push_back(x.a);
  }
  REQUIRE(seen == std::vector<int>{10, 50, 320});
}
TEST_CASE("sparse_vector: iteration skips missing pages", "[sparse_vector][iterators]")
{
  ouly::sparse_vector<pod> v;
  v.emplace_at(2, 1, 1);
  v.emplace_at(16 * 40 + 3, 2, 2); // 39 pages between the two entries are never allocated
  v.emplace_at(16 * 41, 3, 3);

  std::vector<int> seen;
  for (auto const& x : v)
  {
    seen.push_back(x.a);
  }
  REQUIRE(seen == std::vector<int>{1, 2, 3});

  ouly::sparse_vector<pod> copy;
  copy.emplace_at(100, 9, 9);
  copy = v; // must release copy's own pages (checked by LeakSanitizer)
  REQUIRE(copy.size() == v.size());
  ouly::sparse_vector<pod> moved;
  moved.emplace_at(200, 9, 9);
  moved = std::move(copy);
  REQUIRE(moved.size() == v.size());
}
// NOLINTEND
