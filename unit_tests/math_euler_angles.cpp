#include <catch2/catch_all.hpp>
#include <acl/math/vml.hpp>

TEST_CASE("Validate euler_angles::canonize", "[euler_angles::canonize]")
{
  acl::euler_angles angles = acl::make_euler_angles(acl::to_radians(100), acl::to_radians(720), acl::to_radians(410));
  angles                     = acl::canonize(angles);
  CHECK(acl::to_degrees(acl::x(angles)) == Catch::Approx(80.0f));
  CHECK(acl::to_degrees(acl::y(angles)) == Catch::Approx(-180.0f));
  CHECK(acl::to_degrees(acl::z(angles)) == Catch::Approx(-130.0f));

  angles = acl::euler_angles(acl::to_radians(-100), acl::to_radians(720), acl::to_radians(410));
  angles = acl::canonize(angles);
  CHECK(acl::to_degrees(acl::x(angles)) == Catch::Approx(10.0f));
  CHECK(acl::to_degrees(acl::y(angles)) == Catch::Approx(-180.0f));
  CHECK(acl::to_degrees(acl::z(angles)) == Catch::Approx(-130.0f));

  angles = acl::euler_angles(acl::k_pi_by_2 - 1e-3f, acl::to_radians(720), acl::to_radians(410));
  angles = acl::canonize(angles);
  CHECK(acl::to_degrees(acl::x(angles)) == Catch::Approx(89.9427f));
  CHECK(acl::to_degrees(acl::z(angles)) == Catch::Approx(49.99998f));
}

TEST_CASE("Validate euler_angles::from_quat", "[euler_angles::from_quat]")
{
  acl::euler_angles angles =
    acl::make_euler_angles_from_quat_conjugate(acl::make_quaternion(acl::vec3(0, 1, 0), acl::to_radians(65.0f)));
  angles = acl::canonize(angles);
  CHECK(acl::to_degrees(acl::x(angles)) == Catch::Approx(0.0f));
  CHECK(acl::to_degrees(acl::y(angles)) == Catch::Approx(65.0f));
  CHECK(acl::to_degrees(acl::z(angles)) == Catch::Approx(0.0f));
}

TEST_CASE("Validate euler_angles::from_quat_conjugate", "[euler_angles::from_quat_conjugate]")
{
  acl::euler_angles angles = acl::make_euler_angles_from_quat_conjugate(
    acl::conjugate(acl::make_quaternion(acl::vec3(0, 1, 0), acl::to_radians(65.0f))));
  angles = acl::canonize(angles);
  CHECK(acl::to_degrees(acl::x(angles)) == Catch::Approx(0.0f));
  CHECK(acl::to_degrees(acl::y(angles)) == Catch::Approx(65.0f));
  CHECK(acl::to_degrees(acl::z(angles)) == Catch::Approx(0.0f));
}

TEST_CASE("Validate euler_angles::from_mat4", "[euler_angles::from_mat4]")
{
  acl::mat4 mat = acl::make_mat4_form_quaternion(acl::make_quaternion(acl::vec3(0, 1, 0), acl::to_radians(65.0f)));
  acl::euler_angles angles = acl::make_euler_angles(mat);
  angles                     = acl::canonize(angles);
  CHECK(acl::to_degrees(acl::x(angles)) == Catch::Approx(0.0f));
  CHECK(acl::to_degrees(acl::y(angles)) == Catch::Approx(65.0f));
  CHECK(acl::to_degrees(acl::z(angles)) == Catch::Approx(0.0f));
}

TEST_CASE("Validate euler_angles::from_mat3", "[euler_angles::from_mat3]")
{
  acl::mat3 mat = acl::make_mat3(acl::make_quaternion(acl::vec3(0, 1, 0), acl::to_radians(65.0f)));
  acl::euler_angles angles = acl::make_euler_angles(mat);
  angles                     = acl::canonize(angles);
  CHECK(acl::to_degrees(acl::x(angles)) == Catch::Approx(0.0f));
  CHECK(acl::to_degrees(acl::y(angles)) == Catch::Approx(65.0f));
  CHECK(acl::to_degrees(acl::z(angles)) == Catch::Approx(0.0f));
}
