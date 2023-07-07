#include <catch2/catch_all.hpp>
#include <acl/math/vml.hpp>

TEST_CASE("Validate aabb::is_valid", "[aabb::is_valid]")
{
  REQUIRE(acl::rect::get(acl::rect::set(12.0f, 4.0f, 4.0f, 1.0f), 0, 0) == 12.0f);

  REQUIRE(acl::aabb::is_valid(acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, 13.0f))) ==
          true);
  REQUIRE(acl::aabb::is_valid(
            acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(-5.0f, 15.0f, 13.0f))) == false);
  REQUIRE(acl::aabb::is_valid(
            acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, -13.0f))) == false);
  REQUIRE(acl::aabb::is_valid(
            acl::aabb::set_min_max(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(0.1f, 5.0f, 13.0f))) == false);
  REQUIRE(acl::aabb::is_valid(
            acl::aabb::set_min_max(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(-1.0f, 5.0f, 13.0f))) == false);
  REQUIRE(acl::aabb::is_valid(
            acl::aabb::set_min_max(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(1.0f, -5.0f, 13.0f))) == false);
  REQUIRE(acl::aabb::is_valid(
            acl::aabb::set_min_max(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(1.0f, 1.0f, 13.0f))) == false);
  REQUIRE(acl::aabb::is_valid(
            acl::aabb::set_min_max(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(1.0f, 2.0f, 1.0f))) == false);
  REQUIRE(acl::aabb::is_valid(
            acl::aabb::set_min_max(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(1.0f, 2.0f, -8.0f))) == false);
}

TEST_CASE("Validate aabb::center", "[aabb::center]")
{
  REQUIRE(acl::vec3a::x(acl::aabb::center(
            acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, 13.0f)))) == 1.0f);
  REQUIRE(acl::vec3a::y(acl::aabb::center(
            acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, 13.0f)))) == 2.0f);
  REQUIRE(acl::vec3a::z(acl::aabb::center(
            acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, 13.0f)))) == 3.0f);
  REQUIRE(acl::vec3a::w(acl::aabb::center(
            acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, 13.0f)))) == 0.0f);
}

TEST_CASE("Validate aabb::size", "[aabb::size]")
{
  REQUIRE(acl::vec3a::x(acl::aabb::size(
            acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, 13.0f)))) == 10.0f);
  REQUIRE(acl::vec3a::y(acl::aabb::size(
            acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, 13.0f)))) == 30.0f);
  REQUIRE(acl::vec3a::z(acl::aabb::size(
            acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, 13.0f)))) == 26.0f);
  REQUIRE(acl::vec3a::w(acl::aabb::size(
            acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, 13.0f)))) == 0.0f);
}

TEST_CASE("Validate aabb::half_size", "[aabb::half_size]")
{
  REQUIRE(acl::vec3a::x(acl::aabb::half_size(
            acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, 13.0f)))) == 5.0f);
  REQUIRE(acl::vec3a::y(acl::aabb::half_size(
            acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, 13.0f)))) == 15.0f);
  REQUIRE(acl::vec3a::z(acl::aabb::half_size(
            acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, 13.0f)))) == 13.0f);
  REQUIRE(acl::vec3a::w(acl::aabb::half_size(
            acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, 13.0f)))) == 0.0f);
}

TEST_CASE("Validate aabb::corner", "[aabb::corner]")
{
  REQUIRE(acl::vec3a::x(acl::aabb::corner(
            acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, 13.0f)), 0)) == -4.0f);
  REQUIRE(acl::vec3a::y(acl::aabb::corner(
            acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, 13.0f)), 0)) == -13.0f);
  REQUIRE(acl::vec3a::z(acl::aabb::corner(
            acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, 13.0f)), 0)) == -10.0f);
  REQUIRE(acl::vec3a::w(acl::aabb::corner(
            acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, 13.0f)), 0)) == 0.0f);

  REQUIRE(acl::vec3a::x(acl::aabb::corner(
            acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, 13.0f)), 1)) == -4.0f);
  REQUIRE(acl::vec3a::y(acl::aabb::corner(
            acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, 13.0f)), 1)) == -13.0f);
  REQUIRE(acl::vec3a::z(acl::aabb::corner(
            acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, 13.0f)), 1)) == 16.0f);
  REQUIRE(acl::vec3a::w(acl::aabb::corner(
            acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, 13.0f)), 1)) == 0.0f);

  REQUIRE(acl::vec3a::x(acl::aabb::corner(
            acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, 13.0f)), 2)) == -4.0f);
  REQUIRE(acl::vec3a::y(acl::aabb::corner(
            acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, 13.0f)), 2)) == 17.0f);
  REQUIRE(acl::vec3a::z(acl::aabb::corner(
            acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, 13.0f)), 2)) == -10.0f);
  REQUIRE(acl::vec3a::w(acl::aabb::corner(
            acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, 13.0f)), 2)) == 0.0f);

  REQUIRE(acl::vec3a::x(acl::aabb::corner(
            acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, 13.0f)), 3)) == -4.0f);
  REQUIRE(acl::vec3a::y(acl::aabb::corner(
            acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, 13.0f)), 3)) == 17.0f);
  REQUIRE(acl::vec3a::z(acl::aabb::corner(
            acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, 13.0f)), 3)) == 16.0f);
  REQUIRE(acl::vec3a::w(acl::aabb::corner(
            acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, 13.0f)), 3)) == 0.0f);

  REQUIRE(acl::vec3a::x(acl::aabb::corner(
            acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, 13.0f)), 4)) == 6.0f);
  REQUIRE(acl::vec3a::y(acl::aabb::corner(
            acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, 13.0f)), 4)) == -13.0f);
  REQUIRE(acl::vec3a::z(acl::aabb::corner(
            acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, 13.0f)), 4)) == -10.0f);
  REQUIRE(acl::vec3a::w(acl::aabb::corner(
            acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, 13.0f)), 4)) == 0.0f);

  REQUIRE(acl::vec3a::x(acl::aabb::corner(
            acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, 13.0f)), 5)) == 6.0f);
  REQUIRE(acl::vec3a::y(acl::aabb::corner(
            acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, 13.0f)), 5)) == -13.0f);
  REQUIRE(acl::vec3a::z(acl::aabb::corner(
            acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, 13.0f)), 5)) == 16.0f);
  REQUIRE(acl::vec3a::w(acl::aabb::corner(
            acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, 13.0f)), 5)) == 0.0f);

  REQUIRE(acl::vec3a::x(acl::aabb::corner(
            acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, 13.0f)), 6)) == 6.0f);
  REQUIRE(acl::vec3a::y(acl::aabb::corner(
            acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, 13.0f)), 6)) == 17.0f);
  REQUIRE(acl::vec3a::z(acl::aabb::corner(
            acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, 13.0f)), 6)) == -10.0f);
  REQUIRE(acl::vec3a::w(acl::aabb::corner(
            acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, 13.0f)), 6)) == 0.0f);

  REQUIRE(acl::vec3a::x(acl::aabb::corner(
            acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, 13.0f)), 7)) == 6.0f);
  REQUIRE(acl::vec3a::y(acl::aabb::corner(
            acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, 13.0f)), 7)) == 17.0f);
  REQUIRE(acl::vec3a::z(acl::aabb::corner(
            acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, 13.0f)), 7)) == 16.0f);
  REQUIRE(acl::vec3a::w(acl::aabb::corner(
            acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, 13.0f)), 7)) == 0.0f);
}

TEST_CASE("Validate aabb::append", "[aabb::append]")
{
  acl::aabb_t  aabb1 = acl::aabb::set(acl::vec3a::set(1.0f, 2.0f, 3.0f), acl::vec3a::set(5.0f, 15.0f, 13.0f));
  acl::aabb_t  aabb2 = acl::aabb::set(acl::vec3a::set(0.0f, 0.0f, -9.0f), acl::vec3a::set(1.0f, 5.0f, 10.0f));
  acl::vec3a_t p     = acl::vec3a::set(2.0f, 11.0f, -32.0f);

  REQUIRE(acl::vec3a::x(acl::aabb::center(acl::aabb::append(aabb1, p))) == 1.0f);
  REQUIRE(acl::vec3a::z(acl::aabb::center(acl::aabb::append(aabb1, p))) == -8.0f);
  REQUIRE(acl::vec3a::x(acl::aabb::size(acl::aabb::append(aabb1, p))) == 10.0f);
  REQUIRE(acl::vec3a::z(acl::aabb::size(acl::aabb::append(aabb1, p))) == 48.0f);

  REQUIRE(acl::vec3a::x(acl::aabb::center(acl::aabb::append(aabb1, aabb2))) == 1.0f);
  REQUIRE(acl::vec3a::y(acl::aabb::center(acl::aabb::append(aabb1, aabb2))) == 2.0f);
  REQUIRE(acl::vec3a::z(acl::aabb::center(acl::aabb::append(aabb1, aabb2))) == -1.5f);
  REQUIRE(acl::vec3a::x(acl::aabb::size(acl::aabb::append(aabb1, aabb2))) == 10.0f);
  REQUIRE(acl::vec3a::y(acl::aabb::size(acl::aabb::append(aabb1, aabb2))) == 30.0f);
  REQUIRE(acl::vec3a::z(acl::aabb::size(acl::aabb::append(aabb1, aabb2))) == 35.0f);
}