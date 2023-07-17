#include <acl/math/vml.hpp>
#include <catch2/catch_all.hpp>

TEMPLATE_TEST_CASE("Validate axis_angle::set", "[axis_angle::set]", float, double)
{
  auto aa =
    acl::make_axis_angle(acl::normalize(acl::vec3a_t<TestType>(static_cast<TestType>(1.0f), static_cast<TestType>(1.0f),
                                                               static_cast<TestType>(1.0f))),
                         acl::to_radians<TestType>(static_cast<TestType>(10.0f)));

  CHECK(acl::get_x(acl::axis(aa)) == Catch::Approx(0.57735f));
  CHECK(acl::get_y(acl::axis(aa)) == Catch::Approx(0.57735f));
  CHECK(acl::get_z(acl::axis(aa)) == Catch::Approx(0.57735f));
  CHECK(acl::equals<TestType>(acl::angle(aa), 0.17453f));
  CHECK(acl::equals<TestType>(acl::get_x(acl::vangle(aa)), 0.17453f));
}
