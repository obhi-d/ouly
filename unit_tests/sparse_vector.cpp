
#include "test_common.hpp"
#include <acl/containers/sparse_vector.hpp>
#include <catch2/catch_all.hpp>
#include <string>

// NOLINTBEGIN
template <>
struct acl::default_config<pod>
{
  static constexpr std::uint32_t pool_size_v  = 10;
  static constexpr pod           null_v       = pod{};
  static constexpr bool          assume_pod_v = true;
};

TEST_CASE("sparse_vector: Validate sparse_vector emplace", "[sparse_vector][emplace]")
{
  acl::sparse_vector<pod> v1;
  v1.emplace_at(1, 100, 120);
  v1.emplace_at(10, 200, 220);
  v1.emplace_at(30, 300, 320);
  REQUIRE(v1.max_pools() == 4);
  REQUIRE(v1.contains(0) == false);
  REQUIRE(v1.contains(1) == true);
  REQUIRE(v1.contains(2) == false);
  REQUIRE(v1.contains(11) == false);
  REQUIRE(v1.contains(10) == true);
  REQUIRE(v1.contains(32) == false);
  REQUIRE(v1.contains(42) == false);
  REQUIRE(v1.contains(30) == true);
  REQUIRE(v1[30].a == 300);
  REQUIRE(v1[30].b == 320);
  REQUIRE(v1[1].a == 100);
  REQUIRE(v1[1].b == 120);
  REQUIRE(v1[10].a == 200);
  REQUIRE(v1[10].b == 220);
}

TEST_CASE("sparse_vector: Test view", "[sparse_vector][view]")
{
  acl::sparse_vector<pod> v1;
  v1.emplace_at(1, 100, 120);
  v1.emplace_at(10, 200, 220);
  v1.emplace_at(30, 300, 320);

  auto view = v1.view();
  auto h0   = view[1];
  REQUIRE(h0.a == 100);
  REQUIRE(h0.b == 120);

  auto h3 = view[3];
  REQUIRE(h3.a == 0);
  REQUIRE(h3.b == 0);

  auto p = pod();
  h3     = view(43, p);
  REQUIRE(h3.a == 0);
  REQUIRE(h3.b == 0);

  h3 = view[30];
  REQUIRE(h3.a == 300);
  REQUIRE(h3.b == 320);
}

TEST_CASE("sparse_vector: Erase sparse_vector element", "[sparse_vector][erase]")
{
  acl::sparse_vector<pod> v1;
  v1.emplace_at(1, 100, 120);
  v1.emplace_at(10, 200, 220);
  v1.emplace_at(30, 300, 320);
  v1.emplace_at(2, 500, 10);
  v1.emplace_at(3, 5, 12);
  REQUIRE(v1.max_pools() == 4);
  REQUIRE(v1[2].a == 500);
  REQUIRE(v1[2].b == 10);
  v1.erase(2);
  REQUIRE(v1.contains(2) == false);
  v1.emplace_at(2, 1, 2);
  REQUIRE(v1[2].a == 1);
  REQUIRE(v1[2].b == 2);
  auto index = v1.index(2);
  REQUIRE(v1.view().contains(index) == true);
  REQUIRE(v1.view()[index].a == 1);
}

TEST_CASE("sparse_vector: Copy sparse_vector to another", "[sparse_vector][copy]")
{
  acl::sparse_vector<pod> v1;
  acl::sparse_vector<pod> v2;
  std::vector<pod>        ref;
  std::vector<bool>       idx;
  std::uint32_t           stop = range_rand<std::uint32_t>(10, 1000);

  for (std::uint32_t i = 0; i < stop; ++i)
  {
    pod data = {range_rand(0, 100), range_rand(0, 100)};

    if (range_rand(0, 4) > 2)
    {
      idx.emplace_back(true);
      ref.push_back(data);
      v1.emplace_at(i, data);
    }
    else
    {
      ref.push_back(pod{});
      idx.emplace_back(false);
    }
  }

  v2 = v1;
  REQUIRE(v2.max_pools() == v1.max_pools());
  for (std::uint32_t i = 0; i < stop; ++i)
  {
    REQUIRE(v2.contains(i) == idx[i]);
    if (idx[i])
      REQUIRE(v2.at(i) == ref[i]);
  }

  for (std::uint32_t i = 0; i < stop; ++i)
  {
    if (range_rand(0, 4) > 2 && v2.contains(i))
    {
      v2.erase(i);
      ref[i] = pod{};
      idx[i] = false;
    }
  }

  for (std::uint32_t i = 0; i < stop; ++i)
  {
    REQUIRE(v2.contains(i) == idx[i]);
    if (idx[i])
      REQUIRE(v2.at(i) == ref[i]);
  }
}

TEST_CASE("sparse_vector: Move sparse_vector to another", "[sparse_vector][move]")
{
  acl::sparse_vector<pod> v1;
  acl::sparse_vector<pod> v2;
  std::vector<pod>        ref;
  std::vector<bool>       idx;
  std::uint32_t           stop = range_rand<std::uint32_t>(10, 1000);

  for (std::uint32_t i = 0; i < stop; ++i)
  {
    pod data = {range_rand(0, 100), range_rand(0, 100)};

    if (range_rand(0, 4) > 2)
    {
      idx.emplace_back(true);
      ref.push_back(data);
      v1.emplace_at(i, data);
    }
    else
    {
      ref.push_back(pod{});
      idx.emplace_back(false);
    }
  }

  v2 = std::move(v1);
  REQUIRE(v2.max_pools() != v1.max_pools());
  for (std::uint32_t i = 0; i < stop; ++i)
  {
    REQUIRE(v2.contains(i) == idx[i]);
    if (idx[i])
      REQUIRE(v2.at(i) == ref[i]);
  }
  v1 = std::move(v2);
  REQUIRE(v2.max_pools() != v1.max_pools());
  for (std::uint32_t i = 0; i < stop; ++i)
  {
    REQUIRE(v1.contains(i) == idx[i]);
    if (idx[i])
      REQUIRE(v1.at(i) == ref[i]);
  }
}

struct untracked_pod
{
  static constexpr std::uint32_t pool_size_v            = 4;
  static constexpr pod           null_v                 = pod{};
  static constexpr bool          disble_pool_tracking_v = true;
};

TEST_CASE("sparse_vector: for_each", "[sparse_vector][for_each]")
{
  acl::sparse_vector<std::string, untracked_pod> v1;
  std::uint32_t                                  stop = range_rand<std::uint32_t>(100, 1000);

  for (std::uint32_t i = 0; i < stop; ++i)
  {
    v1.emplace_back(std::to_string(i));
  }

  for (int i = 0; i < 20; ++i)
  {
    uint32_t start = stop / range_rand<std::uint32_t>(2, 200);
    uint32_t end   = stop / range_rand<std::uint32_t>(1, 2);
    v1.for_each(
     [&start, end](std::string const& v)
     {
       REQUIRE(v == std::to_string(start++));
     },
     start, end);
  }
}

TEST_CASE("sparse_vector: unordered_merge", "[sparse_vector][unordered_merge]")
{
  for (int n = 0; n < 20; ++n)
  {
    acl::sparse_vector<std::string, untracked_pod> v1;
    acl::sparse_vector<std::string, untracked_pod> v2;
    std::unordered_set<std::string>                check;
    std::uint32_t                                  stop = range_rand<std::uint32_t>(10, 200);

    for (std::uint32_t i = 0; i < stop; ++i)
    {
      auto val = std::to_string(i);
      check.emplace(val);
      v1.emplace_back(val);
    }

    for (std::uint32_t i = 0; i < stop; ++i)
    {
      auto val = std::to_string(i + stop);
      check.emplace(val);
      v2.emplace_back(val);
    }

    v1.unordered_merge(std::move(v2));

    uint32_t start = 0;
    v1.for_each(
     [&](std::string const& v)
     {
       REQUIRE(check.count(v) == 1);
     });

    for (int i = 0; i < 20; ++i)
    {
      uint32_t start = range_rand<std::uint32_t>(0, stop);
      uint32_t end   = range_rand<std::uint32_t>(0, stop);
      if (start > end)
        std::swap(start, end);
      v1.for_each(
       [&](std::string const& v)
       {
         REQUIRE(check.count(v) == 1);
       },
       start, end);
    }

    v1.for_each(
     [&](std::string const& v)
     {
       REQUIRE(check.erase(v) == 1);
     });

    REQUIRE(check.empty() == true);
  }
}

TEST_CASE("sparse_vector: unordered_merge iterator", "[sparse_vector][unordered_merge]")
{
  acl::sparse_vector<std::string, untracked_pod>              merged;
  std::vector<acl::sparse_vector<std::string, untracked_pod>> v1;
  std::unordered_set<std::string>                             check;
  for (int n = 0; n < 20; ++n)
  {
    std::uint32_t stop = range_rand<std::uint32_t>(10, 200);
    v1.emplace_back();
    auto& v = v1.back();
    for (std::uint32_t i = 0; i < stop; ++i)
    {
      auto val = std::to_string(i + n * 200);
      check.emplace(val);
      v.emplace_back(val);
    }
  }

  merged.unordered_merge(v1.begin(), v1.end());
  merged.for_each(
   [&](std::string const& v)
   {
     REQUIRE(check.erase(v) == 1);
   });

  REQUIRE(check.empty() == true);
}

TEST_CASE("sparse_vector: fill", "[sparse_vector][fill]")
{
  acl::sparse_vector<int, acl::config<acl::cfg::pool_size<20>>> vv;
  for (int n = 0; n < 200; ++n)
  {
    std::uint32_t stop = range_rand<std::uint32_t>(10, 200);
    vv.emplace_back(stop);
  }

  decltype(vv) other = vv;

  vv.for_each(
   [&](uint32_t idx, int const& v)
   {
     REQUIRE(other.at(idx) == v);
   });

  vv.fill(0xbadf00d);
  vv.for_each(
   [&](int const& v)
   {
     REQUIRE(0xbadf00d == v);
   });
}
// NOLINTEND