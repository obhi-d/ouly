#include <acl/math/vml.hpp>
#include <catch2/catch_all.hpp>

TEMPLATE_TEST_CASE("Validate euler_angles_t<TestType>::canonize", "[euler_angles_t<TestType>::canonize]", float, double)
{
  acl::euler_angles_t<TestType> angles = acl::euler_angles_t<TestType>(
    acl::to_radians<TestType>(100.0f), acl::to_radians<TestType>(720.0f), acl::to_radians<TestType>(410.0f));
  angles = acl::canonize(angles);
  CHECK(acl::to_degrees(acl::get_x(angles)) == Catch::Approx(80.0f));
  CHECK(acl::to_degrees(acl::get_y(angles)) == Catch::Approx(-180.0f));
  CHECK(acl::to_degrees(acl::get_z(angles)) == Catch::Approx(-130.0f));

  angles = acl::euler_angles_t<TestType>(acl::to_radians<TestType>(-100.0f), acl::to_radians<TestType>(720.0f),
                                         acl::to_radians<TestType>(410.0f));
  angles = acl::canonize(angles);
  CHECK(acl::to_degrees(acl::get_x(angles)) == Catch::Approx(10.0f));
  CHECK(acl::to_degrees(acl::get_y(angles)) == Catch::Approx(-180.0f));
  CHECK(acl::to_degrees(acl::get_z(angles)) == Catch::Approx(-130.0f));

  angles = acl::euler_angles_t<TestType>(acl::k_pi_by_2 - 1e-3f, acl::to_radians<TestType>(720.0f),
                                         acl::to_radians<TestType>(410.0f));
  angles = acl::canonize(angles);
  CHECK(acl::to_degrees(acl::get_x(angles)) == Catch::Approx(89.9427f));
  CHECK(acl::to_degrees(acl::get_z(angles)) == Catch::Approx(49.99998f));
}

TEMPLATE_TEST_CASE("Validate euler_angles_t<TestType>::from_quat", "[euler_angles_t<TestType>::from_quat]", float,
                   double)
{
  acl::euler_angles_t<TestType> angles = acl::make_euler_angles_from_quat_conjugate(
    acl::make_quaternion(acl::vec3a_t<TestType>(0, 1, 0), acl::to_radians<TestType>(65.0f)));
  angles = acl::canonize(angles);
  CHECK(acl::to_degrees(acl::get_x(angles)) == Catch::Approx(0.0f));
  CHECK(acl::to_degrees(acl::get_y(angles)) == Catch::Approx(65.0f));
  CHECK(acl::to_degrees(acl::get_z(angles)) == Catch::Approx(0.0f));
}

TEMPLATE_TEST_CASE("Validate euler_angles_t<TestType>::from_quat_conjugate",
                   "[euler_angles_t<TestType>::from_quat_conjugate]", float, double)
{
  acl::euler_angles_t<TestType> angles = acl::make_euler_angles_from_quat_conjugate(
    acl::conjugate(acl::make_quaternion(acl::vec3a_t<TestType>(0.0f, 1.0f, 0.0f), acl::to_radians<TestType>(65.0f))));
  angles = acl::canonize(angles);
  CHECK(acl::to_degrees(acl::get_x(angles)) == Catch::Approx(0.0f));
  CHECK(acl::to_degrees(acl::get_y(angles)) == Catch::Approx(65.0f));
  CHECK(acl::to_degrees(acl::get_z(angles)) == Catch::Approx(0.0f));
}

TEMPLATE_TEST_CASE("Validate euler_angles_t<TestType>::from_mat4", "[euler_angles_t<TestType>::from_mat4]", float,
                   double)
{
  acl::mat4_t<TestType> mat = acl::make_mat4_form_quaternion(
    acl::make_quaternion(acl::vec3a_t<TestType>(0, 1, 0), acl::to_radians<TestType>(65.0f)));
  acl::euler_angles_t<TestType> angles = acl::make_euler_angles(mat);
  angles                               = acl::canonize(angles);
  CHECK(acl::to_degrees(acl::get_x(angles)) == Catch::Approx(0.0f));
  CHECK(acl::to_degrees(acl::get_y(angles)) == Catch::Approx(65.0f));
  CHECK(acl::to_degrees(acl::get_z(angles)) == Catch::Approx(0.0f));
}

TEMPLATE_TEST_CASE("Validate euler_angles_t<TestType>::from_mat3", "[euler_angles_t<TestType>::from_mat3]", float,
                   double)
{
  acl::mat3_t<TestType> mat =
    acl::make_mat3(acl::make_quaternion(acl::vec3a_t<TestType>(0, 1, 0), acl::to_radians<TestType>(65.0f)));
  acl::euler_angles_t<TestType> angles = acl::make_euler_angles(mat);
  angles                               = acl::canonize(angles);
  CHECK(acl::to_degrees(acl::get_x(angles)) == Catch::Approx(0.0f));
  CHECK(acl::to_degrees(acl::get_y(angles)) == Catch::Approx(65.0f));
  CHECK(acl::to_degrees(acl::get_z(angles)) == Catch::Approx(0.0f));
}
