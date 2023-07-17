#include <acl/math/vml.hpp>
#include <catch2/catch_all.hpp>

TEMPLATE_TEST_CASE("Validate abs_normal", "[abs_normal]", float, double)
{
  acl::plane_t<TestType> p = acl::plane_t<TestType>(-1.0f, 1.0f, -1.0f, 10.0f);
  acl::vec3a_t<TestType> n = acl::abs_normal(p);
  CHECK(acl::get_x(acl::vdot(p, acl::vec3a_t<TestType>(1.0f, 1.0f, 1.0f))) == Catch::Approx(9.0));
  CHECK(acl::get_x(n) == Catch::Approx(1.0));
  CHECK(acl::get_y(n) == Catch::Approx(1.0));
  CHECK(acl::get_z(n) == Catch::Approx(1.0));
}

TEMPLATE_TEST_CASE("Validate dot_with_normal", "[dot_with_normal]", float, double)
{
  acl::plane_t<TestType> p = acl::plane_t<TestType>(-1.0f, 1.0f, -1.0f, 10.0f);
  acl::vec3a_t<TestType> n = acl::vec3a_t<TestType>(-2.0f, 3.0f, -5.0f);
  CHECK(acl::dot_with_normal(p, n) == Catch::Approx(10.0f));
  CHECK(acl::get_w(acl::get_normal(p)) == Catch::Approx(0.0f));
  CHECK(acl::get_x(acl::get_normal(p)) == Catch::Approx(-1.0f));
}
