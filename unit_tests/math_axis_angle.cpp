#include <acl/math/vml.hpp>
#include <catch2/catch_all.hpp>

TEST_CASE("Validate axis_angle::set", "[axis_angle::set]")
{
  auto aa = acl::make_axis_angle(acl::vec3a::set(1.0f, 1.0f, 1.0f), acl::to_radians(10.0f));

  CHECK(acl::x(acl::axis(aa)) == Catch::Approx(0.57735f));
  CHECK(acl::y(acl::axis(aa)) == Catch::Approx(0.57735f));
  CHECK(acl::z(acl::axis(aa)) == Catch::Approx(0.57735f));
  CHECK(acl::equals(acl::angle(aa), 0.17453f));
  CHECK(acl::equals(acl::x(acl::vangle(aa)), 0.17453f));
}
