#include <catch2/catch_all.hpp>
#include <acl/math/vml.hpp>

TEST_CASE("Validate euler_angles::canonize", "[euler_angles::canonize]")
{
  acl::euler_angles_t angles = acl::euler_angles::set(acl::to_radians(100), acl::to_radians(720), acl::to_radians(410));
  angles                     = acl::euler_angles::canonize(angles);
  CHECK(acl::to_degrees(acl::vec3::x(angles)) == Catch::Approx(80.0f));
  CHECK(acl::to_degrees(acl::vec3::y(angles)) == Catch::Approx(-180.0f));
  CHECK(acl::to_degrees(acl::vec3::z(angles)) == Catch::Approx(-130.0f));

  angles = acl::euler_angles::set(acl::to_radians(-100), acl::to_radians(720), acl::to_radians(410));
  angles = acl::euler_angles::canonize(angles);
  CHECK(acl::to_degrees(acl::vec3::x(angles)) == Catch::Approx(10.0f));
  CHECK(acl::to_degrees(acl::vec3::y(angles)) == Catch::Approx(-180.0f));
  CHECK(acl::to_degrees(acl::vec3::z(angles)) == Catch::Approx(-130.0f));

  angles = acl::euler_angles::set(acl::k_pi_by_2 - 1e-3f, acl::to_radians(720), acl::to_radians(410));
  angles = acl::euler_angles::canonize(angles);
  CHECK(acl::to_degrees(acl::vec3::x(angles)) == Catch::Approx(89.9427f));
  CHECK(acl::to_degrees(acl::vec3::z(angles)) == Catch::Approx(49.99998f));
}

TEST_CASE("Validate euler_angles::from_quat", "[euler_angles::from_quat]")
{
  acl::euler_angles_t angles =
    acl::euler_angles::from_quat(acl::quat::from_axis_angle(acl::vec3::set(0, 1, 0), acl::to_radians(65.0f)));
  angles = acl::euler_angles::canonize(angles);
  CHECK(acl::to_degrees(acl::vec3::x(angles)) == Catch::Approx(0.0f));
  CHECK(acl::to_degrees(acl::vec3::y(angles)) == Catch::Approx(65.0f));
  CHECK(acl::to_degrees(acl::vec3::z(angles)) == Catch::Approx(0.0f));
}

TEST_CASE("Validate euler_angles::from_quat_conjugate", "[euler_angles::from_quat_conjugate]")
{
  acl::euler_angles_t angles = acl::euler_angles::from_quat_conjugate(
    acl::quat::conjugate(acl::quat::from_axis_angle(acl::vec3::set(0, 1, 0), acl::to_radians(65.0f))));
  angles = acl::euler_angles::canonize(angles);
  CHECK(acl::to_degrees(acl::vec3::x(angles)) == Catch::Approx(0.0f));
  CHECK(acl::to_degrees(acl::vec3::y(angles)) == Catch::Approx(65.0f));
  CHECK(acl::to_degrees(acl::vec3::z(angles)) == Catch::Approx(0.0f));
}

TEST_CASE("Validate euler_angles::from_mat4", "[euler_angles::from_mat4]")
{
  acl::mat4_t mat =
    acl::mat4::from_rotation(acl::quat::from_axis_angle(acl::vec3::set(0, 1, 0), acl::to_radians(65.0f)));
  acl::euler_angles_t angles = acl::euler_angles::from_mat4(mat);
  angles                     = acl::euler_angles::canonize(angles);
  CHECK(acl::to_degrees(acl::vec3::x(angles)) == Catch::Approx(0.0f));
  CHECK(acl::to_degrees(acl::vec3::y(angles)) == Catch::Approx(65.0f));
  CHECK(acl::to_degrees(acl::vec3::z(angles)) == Catch::Approx(0.0f));
}

TEST_CASE("Validate euler_angles::from_mat3", "[euler_angles::from_mat3]")
{
  acl::mat3_t mat =
    acl::mat3::from_rotation(acl::quat::from_axis_angle(acl::vec3::set(0, 1, 0), acl::to_radians(65.0f)));
  acl::euler_angles_t angles = acl::euler_angles::from_mat3(mat);
  angles                     = acl::euler_angles::canonize(angles);
  CHECK(acl::to_degrees(acl::vec3::x(angles)) == Catch::Approx(0.0f));
  CHECK(acl::to_degrees(acl::vec3::y(angles)) == Catch::Approx(65.0f));
  CHECK(acl::to_degrees(acl::vec3::z(angles)) == Catch::Approx(0.0f));
}
