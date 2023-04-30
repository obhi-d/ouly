
#include "test_common.hpp"
#include <acl/sparse_vector.hpp>
#include <catch2/catch_all.hpp>
#include <string>

template <>
struct acl::traits<pod> : acl::traits<>
{
  static constexpr std::uint32_t pool_size    = 10;
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
