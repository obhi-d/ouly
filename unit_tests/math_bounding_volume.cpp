#include <catch2/catch_all.hpp>
#include <acl/math/vml.hpp>

TEST_CASE("Validate bounds_info::update", "[bounds_info::update]")
{
  acl::bounds_info_t bounds = {{-2.0f, -2.0f, -2.0f}, {2.0f, 2.0f, 2.0f}, -1.0f};

  acl::bounds_info_t bounds1 = {{-2.0f, -2.0f, -2.0f}, {2.0f, 2.0f, 2.0f}, 3.4641f};
  acl::bounds_info_t bounds2 = {{2.0f, 2.0f, 2.0f}, {2.0f, 2.0f, 2.0f}, 3.4641f};

  acl::bounds_info::update(bounds, bounds1);
  acl::bounds_info::update(bounds, bounds2);

  REQUIRE(acl::vec3::x(bounds.center) == Catch::Approx(0.0f));
  REQUIRE(acl::vec3::y(bounds.center) == Catch::Approx(0.0f));
  REQUIRE(acl::vec3::z(bounds.center) == Catch::Approx(0.0f));

  REQUIRE(acl::vec3::x(bounds.half_extends) == Catch::Approx(4.0));
  REQUIRE(acl::vec3::y(bounds.half_extends) == Catch::Approx(4.0));
  REQUIRE(acl::vec3::z(bounds.half_extends) == Catch::Approx(4.0));

  REQUIRE(bounds.radius == Catch::Approx(6.9282));
}

TEST_CASE("Validate bounding_volume::center", "[bounding_volume::center]")
{
  acl::bounding_volume_t bounds1 = {
    acl::sphere::set(5.0f, 2.2f, 5.0f, 3.4641f),
    acl::vec3a::set(2.0f, 2.0f, 2.0f),
    acl::sphere::set(0.0f, 0.0f, 0.0f, 3.4641f),
    acl::vec3a::set(2.0f, 2.0f, 2.0f),
  };

  REQUIRE(acl::vec3a::x(acl::bounding_volume::center(bounds1)) == Catch::Approx(5.0f));
  REQUIRE(acl::vec3a::y(acl::bounding_volume::center(bounds1)) == Catch::Approx(2.2f));
  REQUIRE(acl::vec3a::z(acl::bounding_volume::center(bounds1)) == Catch::Approx(5.0f));
}

TEST_CASE("Validate bounding_volume::half_extends", "[bounding_volume::half_extends]")
{
  acl::bounding_volume_t bounds1 = {
    acl::sphere::set(5.0f, 2.2f, 5.0f, 3.4641f),
    acl::vec3a::set(2.0f, 2.0f, 2.0f),
    acl::sphere::set(0.0f, 0.0f, 0.0f, 3.4641f),
    acl::vec3a::set(2.0f, 2.0f, 2.0f),
  };

  REQUIRE(acl::vec3a::x(acl::bounding_volume::half_extends(bounds1)) == Catch::Approx(2.0f));
  REQUIRE(acl::vec3a::y(acl::bounding_volume::half_extends(bounds1)) == Catch::Approx(2.0f));
  REQUIRE(acl::vec3a::z(acl::bounding_volume::half_extends(bounds1)) == Catch::Approx(2.0f));
}

TEST_CASE("Validate bounding_volume::radius", "[bounding_volume::radius]")
{
  acl::bounding_volume_t bounds1 = {
    acl::sphere::set(5.0f, 2.2f, 5.0f, 3.4641f),
    acl::vec3a::set(2.0f, 2.0f, 2.0f),
    acl::sphere::set(0.0f, 0.0f, 0.0f, 3.4641f),
    acl::vec3a::set(2.0f, 2.0f, 2.0f),
  };

  REQUIRE(acl::bounding_volume::radius(bounds1) == Catch::Approx(3.4641f));
}

TEST_CASE("Validate bounding_volume::vradius", "[bounding_volume::vradius]")
{
  acl::bounding_volume_t bounds1 = {
    acl::sphere::set(5.0f, 2.2f, 5.0f, 3.4641f),
    acl::vec3a::set(2.0f, 2.0f, 2.0f),
    acl::sphere::set(0.0f, 0.0f, 0.0f, 3.4641f),
    acl::vec3a::set(2.0f, 2.0f, 2.0f),
  };

  REQUIRE(acl::vec3a::x(acl::bounding_volume::vradius(bounds1)) == Catch::Approx(3.4641f));
}

TEST_CASE("Validate bounding_volume::nullify", "[bounding_volume::nullify]")
{
  acl::bounding_volume_t bounds1 = {
    acl::sphere::set(5.0f, 2.2f, 5.0f, 3.4641f),
    acl::vec3a::set(2.0f, 2.0f, 2.0f),
    acl::sphere::set(0.0f, 0.0f, 0.0f, 3.4641f),
    acl::vec3a::set(2.0f, 2.0f, 2.0f),
  };
  acl::bounding_volume::nullify(bounds1);
  REQUIRE(acl::vec3a::x(acl::bounding_volume::center(bounds1)) == Catch::Approx(0.0f));
  REQUIRE(acl::vec3a::y(acl::bounding_volume::center(bounds1)) == Catch::Approx(0.0f));
  REQUIRE(acl::vec3a::z(acl::bounding_volume::center(bounds1)) == Catch::Approx(0.0f));
  REQUIRE(acl::bounding_volume::radius(bounds1) == Catch::Approx(0.0f));
  REQUIRE(acl::vec3a::x(acl::bounding_volume::half_extends(bounds1)) == Catch::Approx(0.0f));
  REQUIRE(acl::vec3a::y(acl::bounding_volume::half_extends(bounds1)) == Catch::Approx(0.0f));
  REQUIRE(acl::vec3a::z(acl::bounding_volume::half_extends(bounds1)) == Catch::Approx(0.0f));
}

TEST_CASE("Validate bounding_volume::from_box", "[bounding_volume::from_box]")
{
  acl::bounding_volume_t bounds1 =
    acl::bounding_volume::from_box(acl::vec3a::set(2.0f, 1.0f, 5.0f), acl::vec3a::set(4.0f, 1.0f, 6.0f));
  REQUIRE(acl::vec3a::x(acl::bounding_volume::center(bounds1)) == Catch::Approx(2.0f));
  REQUIRE(acl::vec3a::y(acl::bounding_volume::center(bounds1)) == Catch::Approx(1.0f));
  REQUIRE(acl::vec3a::z(acl::bounding_volume::center(bounds1)) == Catch::Approx(5.0f));
  REQUIRE(acl::bounding_volume::radius(bounds1) == Catch::Approx(7.28011f));
  REQUIRE(acl::vec3a::x(acl::bounding_volume::half_extends(bounds1)) == Catch::Approx(4.0f));
  REQUIRE(acl::vec3a::y(acl::bounding_volume::half_extends(bounds1)) == Catch::Approx(1.0f));
  REQUIRE(acl::vec3a::z(acl::bounding_volume::half_extends(bounds1)) == Catch::Approx(6.0f));
}

TEST_CASE("Validate bounding_volume::set", "[bounding_volume::set]")
{
  acl::bounding_volume_t bounds1 = {
    acl::sphere::set(5.0f, 2.2f, 5.0f, 3.4641f),
    acl::vec3a::set(2.0f, 2.0f, 2.0f),
    acl::sphere::set(0.0f, 0.0f, 0.0f, 3.4641f),
    acl::vec3a::set(2.0f, 2.0f, 2.0f),
  };
  bounds1 = acl::bounding_volume::set(acl::vec3a::set(2.0f, 1.0f, 5.0f), acl::vec3a::set(4.0f, 1.0f, 6.0f), 1100.0f);
  REQUIRE(acl::vec3a::x(acl::bounding_volume::center(bounds1)) == Catch::Approx(2.0f));
  REQUIRE(acl::vec3a::y(acl::bounding_volume::center(bounds1)) == Catch::Approx(1.0f));
  REQUIRE(acl::vec3a::z(acl::bounding_volume::center(bounds1)) == Catch::Approx(5.0f));
  REQUIRE(acl::bounding_volume::radius(bounds1) == Catch::Approx(1100.0f));
  REQUIRE(acl::vec3a::x(acl::bounding_volume::half_extends(bounds1)) == Catch::Approx(4.0f));
  REQUIRE(acl::vec3a::y(acl::bounding_volume::half_extends(bounds1)) == Catch::Approx(1.0f));
  REQUIRE(acl::vec3a::z(acl::bounding_volume::half_extends(bounds1)) == Catch::Approx(6.0f));

  bounds1 =
    acl::bounding_volume::set(acl::sphere::set(12.0f, 31.0f, 5.0f, 22.0f), acl::vec3a::set(14.0f, 15.0f, 61.0f));
  REQUIRE(acl::vec3a::x(acl::bounding_volume::center(bounds1)) == Catch::Approx(12.0f));
  REQUIRE(acl::vec3a::y(acl::bounding_volume::center(bounds1)) == Catch::Approx(31.0f));
  REQUIRE(acl::vec3a::z(acl::bounding_volume::center(bounds1)) == Catch::Approx(5.0f));
  REQUIRE(acl::bounding_volume::radius(bounds1) == Catch::Approx(22.0f));
  REQUIRE(acl::vec3a::x(acl::bounding_volume::half_extends(bounds1)) == Catch::Approx(14.0f));
  REQUIRE(acl::vec3a::y(acl::bounding_volume::half_extends(bounds1)) == Catch::Approx(15.0f));
  REQUIRE(acl::vec3a::z(acl::bounding_volume::half_extends(bounds1)) == Catch::Approx(61.0f));
}

TEST_CASE("Validate bounding_volume::update(matrix)", "[bounding_volume::update(matrix)]")
{
  acl::bounding_volume_t bounds1 = {
    acl::sphere::set(5.0f, 2.2f, 5.0f, 3.4641f),
    acl::vec3a::set(2.0f, 2.0f, 2.0f),
    acl::sphere::set(0.0f, 0.0f, 0.0f, 3.4641f),
    acl::vec3a::set(2.0f, 2.0f, 2.0f),
  };

  acl::mat4_t m = acl::mat4::from_scale_rotation_translation(
    2.0f, acl::quat::from_axis_angle(acl::vec3::set(0.0f, 1.0f, 0.0f), acl::to_radians(45.0f)),
    acl::vec3a::set(2.0f, 0.0f, 0.0f));

  acl::bounding_volume::update(bounds1, m);

  REQUIRE(acl::vec3a::x(acl::bounding_volume::center(bounds1)) == Catch::Approx(2.0f));
  REQUIRE(acl::vec3a::y(acl::bounding_volume::center(bounds1)) == Catch::Approx(0.0f));
  REQUIRE(acl::vec3a::z(acl::bounding_volume::center(bounds1)) == Catch::Approx(0.0f));
  REQUIRE(acl::bounding_volume::radius(bounds1) == Catch::Approx(6.9282f));
  REQUIRE(acl::vec3a::x(acl::bounding_volume::half_extends(bounds1)) == Catch::Approx(5.65685f));
  REQUIRE(acl::vec3a::y(acl::bounding_volume::half_extends(bounds1)) == Catch::Approx(4.0f));
  REQUIRE(acl::vec3a::z(acl::bounding_volume::half_extends(bounds1)) == Catch::Approx(5.65685));
}

TEST_CASE("Validate bounding_volume::update(srt)", "[bounding_volume::update(srt)]")
{
  acl::bounding_volume_t bounds1 = {
    acl::sphere::set(5.0f, 2.2f, 5.0f, 3.4641f),
    acl::vec3a::set(2.0f, 2.0f, 2.0f),
    acl::sphere::set(0.0f, 0.0f, 0.0f, 3.4641f),
    acl::vec3a::set(2.0f, 2.0f, 2.0f),
  };

  acl::bounding_volume::update(bounds1, 2.0f,
                               acl::quat::from_axis_angle(acl::vec3::set(0.0f, 1.0f, 0.0f), acl::to_radians(45.0f)),
                               acl::vec3a::set(2.0f, 0.0f, 0.0f));

  REQUIRE(acl::vec3a::x(acl::bounding_volume::center(bounds1)) == Catch::Approx(2.0f));
  REQUIRE(acl::vec3a::y(acl::bounding_volume::center(bounds1)) == Catch::Approx(0.0f));
  REQUIRE(acl::vec3a::z(acl::bounding_volume::center(bounds1)) == Catch::Approx(0.0f));
  REQUIRE(acl::bounding_volume::radius(bounds1) == Catch::Approx(6.9282f));
  REQUIRE(acl::vec3a::x(acl::bounding_volume::half_extends(bounds1)) == Catch::Approx(5.65685f));
  REQUIRE(acl::vec3a::y(acl::bounding_volume::half_extends(bounds1)) == Catch::Approx(4.0f));
  REQUIRE(acl::vec3a::z(acl::bounding_volume::half_extends(bounds1)) == Catch::Approx(5.65685));
}

TEST_CASE("Validate bounding_volume::update(transform)", "[bounding_volume::update(transform)]")
{
  acl::bounding_volume_t bounds1 = {
    acl::sphere::set(5.0f, 2.2f, 5.0f, 3.4641f),
    acl::vec3a::set(2.0f, 2.0f, 2.0f),
    acl::sphere::set(0.0f, 0.0f, 0.0f, 3.4641f),
    acl::vec3a::set(2.0f, 2.0f, 2.0f),
  };

  acl::transform_t tf;
  acl::transform::set_rotation(tf,
                               acl::quat::from_axis_angle(acl::vec3::set(0.0f, 1.0f, 0.0f), acl::to_radians(45.0f)));
  acl::transform::set_scale(tf, 2.0f);
  acl::transform::set_translation(tf, acl::vec3a::set(2.0f, 0.0f, 0.0f));

  acl::bounding_volume::update(bounds1, tf);

  REQUIRE(acl::vec3a::x(acl::bounding_volume::center(bounds1)) == Catch::Approx(2.0f));
  REQUIRE(acl::vec3a::y(acl::bounding_volume::center(bounds1)) == Catch::Approx(0.0f));
  REQUIRE(acl::vec3a::z(acl::bounding_volume::center(bounds1)) == Catch::Approx(0.0f));
  REQUIRE(acl::bounding_volume::radius(bounds1) == Catch::Approx(6.9282f));
  REQUIRE(acl::vec3a::x(acl::bounding_volume::half_extends(bounds1)) == Catch::Approx(5.65685f));
  REQUIRE(acl::vec3a::y(acl::bounding_volume::half_extends(bounds1)) == Catch::Approx(4.0f));
  REQUIRE(acl::vec3a::z(acl::bounding_volume::half_extends(bounds1)) == Catch::Approx(5.65685));
}

TEST_CASE("Validate bounding_volume::update(bounding_volume)", "[bounding_volume::update(bounding_volume)]")
{
  acl::bounding_volume_t bounds1 = {
    acl::sphere::set(5.0f, 4.0f, 5.0f, 3.4641f),
    acl::vec3a::set(2.0f, 2.0f, 2.0f),
    acl::sphere::set(0.0f, 0.0f, 0.0f, 3.4641f),
    acl::vec3a::set(2.0f, 2.0f, 2.0f),
  };
  acl::bounding_volume_t bounds2 = {
    acl::sphere::set(-5.0f, -4.0f, 5.0f, 13.4641f),
    acl::vec3a::set(10.0f, 10.0f, 10.0f),
    acl::sphere::set(0.0f, 0.0f, 0.0f, 13.4641f),
    acl::vec3a::set(2.0f, 2.0f, 2.0f),
  };

  acl::bounding_volume::update(bounds1, bounds2);

  REQUIRE(acl::vec3a::x(acl::bounding_volume::center(bounds1)) == Catch::Approx(-4.0f));
  REQUIRE(acl::vec3a::y(acl::bounding_volume::center(bounds1)) == Catch::Approx(-4.0f));
  REQUIRE(acl::vec3a::z(acl::bounding_volume::center(bounds1)) == Catch::Approx(5.0f));
  REQUIRE(acl::bounding_volume::radius(bounds1) == Catch::Approx(14.86722f));
  REQUIRE(acl::vec3a::x(acl::bounding_volume::half_extends(bounds1)) == Catch::Approx(11.0f));
  REQUIRE(acl::vec3a::y(acl::bounding_volume::half_extends(bounds1)) == Catch::Approx(10.0f));
  REQUIRE(acl::vec3a::z(acl::bounding_volume::half_extends(bounds1)) == Catch::Approx(10.0f));
}

TEST_CASE("Validate bounding_volume::update(points)", "[bounding_volume::update(points)]")
{
  acl::bounding_volume_t bounds1 = {
    acl::sphere::set(5.0f, 4.0f, 5.0f, 3.4641f),
    acl::vec3a::set(2.0f, 2.0f, 2.0f),
    acl::sphere::set(0.0f, 0.0f, 0.0f, 3.4641f),
    acl::vec3a::set(2.0f, 2.0f, 2.0f),
  };
  acl::vec3a_t points[3] = {acl::vec3a::set(-5.0f, -4.0f, 5.0f), acl::vec3a::set(-15.0f, -14.0f, -5.0f),
                            acl::vec3a::set(5.0f, 6.0f, 15.0f)};

  acl::bounding_volume::update(bounds1, points, 3);

  REQUIRE(acl::vec3a::x(acl::bounding_volume::center(bounds1)) == Catch::Approx(-4.0f));
  REQUIRE(acl::vec3a::y(acl::bounding_volume::center(bounds1)) == Catch::Approx(-4.0f));
  REQUIRE(acl::vec3a::z(acl::bounding_volume::center(bounds1)) == Catch::Approx(5.0f));
  REQUIRE(acl::bounding_volume::radius(bounds1) == Catch::Approx(17.91647f));
  REQUIRE(acl::vec3a::x(acl::bounding_volume::half_extends(bounds1)) == Catch::Approx(11.0f));
  REQUIRE(acl::vec3a::y(acl::bounding_volume::half_extends(bounds1)) == Catch::Approx(10.0f));
  REQUIRE(acl::vec3a::z(acl::bounding_volume::half_extends(bounds1)) == Catch::Approx(10.0f));
}
