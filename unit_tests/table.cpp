
#include <acl/containers/soavector.hpp>
#include <acl/containers/table.hpp>
#include <catch2/catch_all.hpp>
#include <string>

TEST_CASE("table: Validate table emplace", "[table][emplace]")
{
  acl::table<std::string> v1;

  auto a1 = v1.emplace("first");
  auto a2 = v1.emplace("second");
  auto a3 = v1.emplace("third");

  REQUIRE(v1[a1] == "first");
  REQUIRE(v1[a2] == "second");
  REQUIRE(v1[a3] == "third");
}

TEST_CASE("table: Validate table erase", "[table][erase]")
{
  using pack = acl::pack<int, bool, std::string>;
  acl::table<std::string> v1;

  auto a1 = v1.emplace("first");
  auto a2 = v1.emplace("second");
  auto a3 = v1.emplace("third");
  auto a4 = v1.emplace("fourth");
  auto a5 = v1.emplace("fifth");
  auto a6 = v1.emplace("sixth");

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
