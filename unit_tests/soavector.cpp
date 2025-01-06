#include "test_common.hpp"
#include <acl/containers/soavector.hpp>
#include <catch2/catch_all.hpp>
#include <string>

// NOLINTBEGIN
TEST_CASE("soavector: Validate soavector emplace", "[soavector][emplace]")
{
  using pack = acl::pack<int, bool, std::string>;
  acl::soavector<pack> v1;
  v1.emplace_back(100, true, "first");
  v1.emplace_back(200, false, "second");
  v1.emplace_back(300, false, "third");
  acl::soavector<acl::pack<int, bool, std::string>> v2 = {
   {100,  true,  "first"},
   {200, false, "second"},
   {300, false,  "third"}
  };
  REQUIRE(v1 == v2);
}

TEST_CASE("soavector: Validate soavector assign", "[soavector][assign]")
{
  using pack = acl::pack<int, bool, std::string>;
  acl::soavector<pack> v1, v2;

  for (std::uint32_t i = 0; i < 1000; ++i)
    v1.emplace_back(std::rand(), std::rand() > 10000, std::to_string(std::rand()));

  v2.assign(v1.begin(), v1.end());
  REQUIRE(v1 == v2);
  auto saved = pack{std::rand(), std::rand() > 10000, std::to_string(std::rand())};
  v1.assign(10, saved);
  v2.assign(10, saved);
  REQUIRE(v1.size() == 10);
  REQUIRE(v1 == v2);
  REQUIRE(v1.back<0>() == std::get<0>(saved));
  REQUIRE(v1.back<1>() == std::get<1>(saved));
  REQUIRE(v2.back<2>() == std::get<2>(saved));
  v2.clear();
  REQUIRE(v2.size() == 0);
  REQUIRE(v2.capacity() != 0);
  v2.shrink_to_fit();
  REQUIRE(v2.capacity() == 0);
}

TEST_CASE("soavector: Validate soavector insert", "[soavector][insert]")
{
  using pack = acl::pack<int, bool, std::string>;
  acl::soavector<pack> v1;

  auto saved = pack{std::rand(), std::rand() > 10000, std::to_string(std::rand())};
  v1.insert(v1.size(), saved);
  v1.insert(v1.size(), pack{std::rand(), std::rand() > 10000, std::to_string(std::rand())});
  v1.insert(v1.size(), pack{std::rand(), std::rand() > 10000, std::to_string(std::rand())});
  v1.insert(2, saved);

  REQUIRE(v1.at<0>(2) == std::get<0>(saved));
  REQUIRE(v1.at<1>(2) == std::get<1>(saved));
  REQUIRE(v1.at<2>(2) == std::get<2>(saved));
  REQUIRE(v1.size() == 4);

  auto saved2 = pack{std::rand(), std::rand() > 10000, std::to_string(std::rand())};
  v1.insert(1, 10, saved);
  for (std::uint32_t i = 0; i < 10; ++i)
  {
    REQUIRE(v1.at<0>(i + 1) == std::get<0>(saved));
    REQUIRE(v1.at<1>(i + 1) == std::get<1>(saved));
    REQUIRE(v1.at<2>(i + 1) == std::get<2>(saved));
  }
}

TEST_CASE("soavector: Validate soavector erase", "[soavector][erase]")
{
  using pack = acl::pack<int, bool, std::string>;
  acl::soavector<pack> v1;

  std::array<pack, 10> saved;
  for (std::uint32_t i = 0; i < 10; ++i)
    saved[i] = pack{std::rand(), std::rand() > 10000, std::to_string(std::rand())};

  v1.insert(0, saved.begin(), saved.end());
  REQUIRE(v1.size() == 10);
  v1.erase(2);
  REQUIRE(v1.size() == 9);
  REQUIRE(v1.at<0>(2) == std::get<0>(saved[3]));
  REQUIRE(v1.at<1>(2) == std::get<1>(saved[3]));
  REQUIRE(v1.at<2>(2) == std::get<2>(saved[3]));
  auto                 v2 = v1;
  std::array<pack, 10> saved2;
  for (std::uint32_t i = 0; i < 10; ++i)
    saved2[i] = pack{std::rand(), std::rand() > 10000, std::to_string(std::rand())};

  v1.insert(1, saved2.begin(), saved2.end());
  v1.erase(1, 11);
  REQUIRE(v1.size() == 9);
  REQUIRE(v2 == v1);
}

// NOLINTEND