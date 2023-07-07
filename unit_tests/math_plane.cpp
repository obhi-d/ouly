#include <catch2/catch_all.hpp>
#include <acl/math/vml.hpp>

TEST_CASE("Validate plane::abs_normal", "[plane::abs_normal]")
{
  acl::plane_t p = acl::plane::set(-1.0f, 1.0f, -1.0f, 10.0f);
  acl::vec3a_t n = acl::plane::abs_normal(p);
  CHECK(acl::plane::x(acl::plane::vdot(p, acl::vec3a::set(1.0f, 1.0f, 1.0f))) == Catch::Approx(9.0));
  CHECK(acl::plane::x(n) == Catch::Approx(1.0));
  CHECK(acl::plane::y(n) == Catch::Approx(1.0));
  CHECK(acl::plane::z(n) == Catch::Approx(1.0));
}

TEST_CASE("Validate plane::dot_with_normal", "[plane::dot_with_normal]")
{
  acl::plane_t p = acl::plane::set(-1.0f, 1.0f, -1.0f, 10.0f);
  acl::vec3a_t n = acl::vec3a::set(-2.0f, 3.0f, -5.0f);
  CHECK(acl::plane::dot_with_normal(p, n) == Catch::Approx(10.0f));
  CHECK(acl::vec4::w(acl::plane::get_normal(p)) == Catch::Approx(0.0f));
  CHECK(acl::vec4::x(acl::plane::get_normal(p)) == Catch::Approx(-1.0f));
}
