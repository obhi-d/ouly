#include <acl/math/vml.hpp>
#include <catch2/catch_all.hpp>

TEST_CASE("Validate axis_angle::set", "[axis_angle::set]")
{
  acl::axis_angle_t aa = acl::axis_angle::set(acl::vec3a::set(1.0f, 1.0f, 1.0f), acl::to_radians(10.0f));

  CHECK(acl::axis_angle::x(acl::axis_angle::axis(aa)) == Catch::Approx(0.57735f));
  CHECK(acl::axis_angle::y(acl::axis_angle::axis(aa)) == Catch::Approx(0.57735f));
  CHECK(acl::axis_angle::z(acl::axis_angle::axis(aa)) == Catch::Approx(0.57735f));
  CHECK(acl::real::equals(acl::axis_angle::angle(aa), 0.17453f));
  CHECK(acl::real::equals(acl::axis_angle::x(acl::axis_angle::vangle(aa)), 0.17453f));
}
