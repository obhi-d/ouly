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
  REQUIRE(seen.size() == 33);
  REQUIRE(seen[1] == 10);
  REQUIRE(seen[5] == 50);
  REQUIRE(seen[32] == 320);
}
// NOLINTEND
