#include <catch2/catch_all.hpp>
#include <acl/math/vml.hpp>

TEST_CASE("Validate is_valid", "[is_valid]")
{
  REQUIRE(acl::rect(12.0f, 4.0f, 4.0f, 1.0f)[0][0] == 12.0f);

  REQUIRE(acl::is_valid(acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, 13.0f))) ==
          true);
  REQUIRE(acl::is_valid(acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(-5.0f, 15.0f, 13.0f))) == false);
  REQUIRE(acl::is_valid(acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, -13.0f))) == false);
  REQUIRE(acl::is_valid(acl::make_aabb_from_min_max(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(0.1f, 5.0f, 13.0f))) == false);
  REQUIRE(acl::is_valid(acl::make_aabb_from_min_max(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(-1.0f, 5.0f, 13.0f))) == false);
  REQUIRE(acl::is_valid(acl::make_aabb_from_min_max(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(1.0f, -5.0f, 13.0f))) == false);
  REQUIRE(acl::is_valid(acl::make_aabb_from_min_max(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(1.0f, 1.0f, 13.0f))) == false);
  REQUIRE(acl::is_valid(acl::make_aabb_from_min_max(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(1.0f, 2.0f, 1.0f))) == false);
  REQUIRE(acl::is_valid(acl::make_aabb_from_min_max(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(1.0f, 2.0f, -8.0f))) == false);
}

TEST_CASE("Validate center", "[center]")
{
  REQUIRE(acl::x(acl::center(acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, 13.0f)))) == 1.0f);
  REQUIRE(acl::y(acl::center(acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, 13.0f)))) == 2.0f);
  REQUIRE(acl::z(acl::center(acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, 13.0f)))) == 3.0f);
  REQUIRE(acl::w(acl::center(acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, 13.0f)))) == 0.0f);
}

TEST_CASE("Validate size", "[size]")
{
  REQUIRE(acl::x(acl::size(
            acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, 13.0f)))) == 10.0f);
  REQUIRE(acl::y(acl::size(
            acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, 13.0f)))) == 30.0f);
  REQUIRE(acl::z(acl::size(
            acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, 13.0f)))) == 26.0f);
  REQUIRE(acl::w(acl::size(
            acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, 13.0f)))) == 0.0f);
}

TEST_CASE("Validate half_size", "[half_size]")
{
  REQUIRE(acl::x(acl::half_size(
            acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, 13.0f)))) == 5.0f);
  REQUIRE(acl::y(acl::half_size(
            acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, 13.0f)))) == 15.0f);
  REQUIRE(acl::z(acl::half_size(
            acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, 13.0f)))) == 13.0f);
  REQUIRE(acl::w(acl::half_size(
            acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, 13.0f)))) == 0.0f);
}

TEST_CASE("Validate corner", "[corner]")
{
  REQUIRE(acl::x(acl::corner(
            acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, 13.0f)), 0)) == -4.0f);
  REQUIRE(acl::y(acl::corner(
            acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, 13.0f)), 0)) == -13.0f);
  REQUIRE(acl::z(acl::corner(
            acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, 13.0f)), 0)) == -10.0f);
  REQUIRE(acl::w(acl::corner(
            acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, 13.0f)), 0)) == 0.0f);

  REQUIRE(acl::x(acl::corner(
            acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, 13.0f)), 1)) == -4.0f);
  REQUIRE(acl::y(acl::corner(
            acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, 13.0f)), 1)) == -13.0f);
  REQUIRE(acl::z(acl::corner(
            acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, 13.0f)), 1)) == 16.0f);
  REQUIRE(acl::w(acl::corner(
            acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, 13.0f)), 1)) == 0.0f);

  REQUIRE(acl::x(acl::corner(
            acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, 13.0f)), 2)) == -4.0f);
  REQUIRE(acl::y(acl::corner(
            acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, 13.0f)), 2)) == 17.0f);
  REQUIRE(acl::z(acl::corner(
            acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, 13.0f)), 2)) == -10.0f);
  REQUIRE(acl::w(acl::corner(
            acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, 13.0f)), 2)) == 0.0f);

  REQUIRE(acl::x(acl::corner(
            acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, 13.0f)), 3)) == -4.0f);
  REQUIRE(acl::y(acl::corner(
            acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, 13.0f)), 3)) == 17.0f);
  REQUIRE(acl::z(acl::corner(
            acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, 13.0f)), 3)) == 16.0f);
  REQUIRE(acl::w(acl::corner(
            acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, 13.0f)), 3)) == 0.0f);

  REQUIRE(acl::x(acl::corner(
            acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, 13.0f)), 4)) == 6.0f);
  REQUIRE(acl::y(acl::corner(
            acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, 13.0f)), 4)) == -13.0f);
  REQUIRE(acl::z(acl::corner(
            acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, 13.0f)), 4)) == -10.0f);
  REQUIRE(acl::w(acl::corner(
            acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, 13.0f)), 4)) == 0.0f);

  REQUIRE(acl::x(acl::corner(
            acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, 13.0f)), 5)) == 6.0f);
  REQUIRE(acl::y(acl::corner(
            acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, 13.0f)), 5)) == -13.0f);
  REQUIRE(acl::z(acl::corner(
            acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, 13.0f)), 5)) == 16.0f);
  REQUIRE(acl::w(acl::corner(
            acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, 13.0f)), 5)) == 0.0f);

  REQUIRE(acl::x(acl::corner(
            acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, 13.0f)), 6)) == 6.0f);
  REQUIRE(acl::y(acl::corner(
            acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, 13.0f)), 6)) == 17.0f);
  REQUIRE(acl::z(acl::corner(
            acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, 13.0f)), 6)) == -10.0f);
  REQUIRE(acl::w(acl::corner(
            acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, 13.0f)), 6)) == 0.0f);

  REQUIRE(acl::x(acl::corner(
            acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, 13.0f)), 7)) == 6.0f);
  REQUIRE(acl::y(acl::corner(
            acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, 13.0f)), 7)) == 17.0f);
  REQUIRE(acl::z(acl::corner(
            acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, 13.0f)), 7)) == 16.0f);
  REQUIRE(acl::w(acl::corner(
            acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, 13.0f)), 7)) == 0.0f);
}

TEST_CASE("Validate append", "[append]")
{
  auto  aabb1 = acl::aabb(acl::vec3a(1.0f, 2.0f, 3.0f), acl::vec3a(5.0f, 15.0f, 13.0f));
  auto  aabb2 = acl::aabb(acl::vec3a(0.0f, 0.0f, -9.0f), acl::vec3a(1.0f, 5.0f, 10.0f));
  auto p     = acl::vec3a(2.0f, 11.0f, -32.0f);

  REQUIRE(acl::x(acl::center(acl::append(aabb1, p))) == 1.0f);
  REQUIRE(acl::z(acl::center(acl::append(aabb1, p))) == -8.0f);
  REQUIRE(acl::x(acl::size(acl::append(aabb1, p))) == 10.0f);
  REQUIRE(acl::z(acl::size(acl::append(aabb1, p))) == 48.0f);

  REQUIRE(acl::x(acl::center(acl::append(aabb1, aabb2))) == 1.0f);
  REQUIRE(acl::y(acl::center(acl::append(aabb1, aabb2))) == 2.0f);
  REQUIRE(acl::z(acl::center(acl::append(aabb1, aabb2))) == -1.5f);
  REQUIRE(acl::x(acl::size(acl::append(aabb1, aabb2))) == 10.0f);
  REQUIRE(acl::y(acl::size(acl::append(aabb1, aabb2))) == 30.0f);
  REQUIRE(acl::z(acl::size(acl::append(aabb1, aabb2))) == 35.0f);
}