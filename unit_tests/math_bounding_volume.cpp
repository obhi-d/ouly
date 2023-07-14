#include <catch2/catch_all.hpp>
#include <acl/math/vml.hpp>

TEST_CASE("Validate bounds_info::update", "[bounds_info::update]")
{
  acl::bounds_info bounds = {{-2.0f, -2.0f, -2.0f}, {2.0f, 2.0f, 2.0f}, -1.0f};

  acl::bounds_info bounds1 = {{-2.0f, -2.0f, -2.0f}, {2.0f, 2.0f, 2.0f}, 3.4641f};
  acl::bounds_info bounds2 = {{2.0f, 2.0f, 2.0f}, {2.0f, 2.0f, 2.0f}, 3.4641f};

  acl::append(bounds, bounds1);
  acl::append(bounds, bounds2);

  REQUIRE(acl::x(bounds.center) == Catch::Approx(0.0f));
  REQUIRE(acl::y(bounds.center) == Catch::Approx(0.0f));
  REQUIRE(acl::z(bounds.center) == Catch::Approx(0.0f));

  REQUIRE(acl::x(bounds.half_extends) == Catch::Approx(4.0));
  REQUIRE(acl::y(bounds.half_extends) == Catch::Approx(4.0));
  REQUIRE(acl::z(bounds.half_extends) == Catch::Approx(4.0));

  REQUIRE(bounds.radius == Catch::Approx(6.9282));
}

TEST_CASE("Validate bounding_volume::center", "[bounding_volume::center]")
{
  acl::bounding_volume bounds = {
    acl::sphere(5.0f, 2.2f, 5.0f, 3.4641f),
    acl::vec3a(2.0f, 2.0f, 2.0f) };

  REQUIRE(acl::x(acl::center(bounds)) == Catch::Approx(5.0f));
  REQUIRE(acl::y(acl::center(bounds)) == Catch::Approx(2.2f));
  REQUIRE(acl::z(acl::center(bounds)) == Catch::Approx(5.0f));
}

TEST_CASE("Validate bounding_volume::half_extends", "[bounding_volume::half_extends]")
{
  acl::bounding_volume bounds1 = {
    acl::sphere(5.0f, 2.2f, 5.0f, 3.4641f),
    acl::vec3a(2.0f, 2.0f, 2.0f)
  };

  REQUIRE(acl::vec3a(acl::half_extends(bounds1)) == Catch::Approx(2.0f));
  REQUIRE(acl::vec3a(acl::half_extends(bounds1)) == Catch::Approx(2.0f));
  REQUIRE(acl::vec3a(acl::half_extends(bounds1)) == Catch::Approx(2.0f));
}

TEST_CASE("Validate bounding_volume::radius", "[bounding_volume::radius]")
{
  acl::bounding_volume bounds1 = {
    acl::sphere(5.0f, 2.2f, 5.0f, 3.4641f),
    acl::vec3a(2.0f, 2.0f, 2.0f)
  };

  REQUIRE(acl::radius(bounds1) == Catch::Approx(3.4641f));
}

TEST_CASE("Validate bounding_volume::vradius", "[bounding_volume::vradius]")
{
  acl::bounding_volume bounds1 = {
    acl::sphere(5.0f, 2.2f, 5.0f, 3.4641f),
    acl::vec3a(2.0f, 2.0f, 2.0f),
    acl::sphere(0.0f, 0.0f, 0.0f, 3.4641f),
    acl::vec3a(2.0f, 2.0f, 2.0f),
  };

  REQUIRE(acl::x(acl::vradius(bounds1)) == Catch::Approx(3.4641f));
}

TEST_CASE("Validate bounding_volume::nullify", "[bounding_volume::nullify]")
{
  acl::bounding_volume bounds1 = {
    acl::sphere(5.0f, 2.2f, 5.0f, 3.4641f),
    acl::vec3a(2.0f, 2.0f, 2.0f)
  };
  acl::nullify(bounds1);
  REQUIRE(acl::x(acl::center(bounds1)) == Catch::Approx(0.0f));
  REQUIRE(acl::y(acl::center(bounds1)) == Catch::Approx(0.0f));
  REQUIRE(acl::z(acl::center(bounds1)) == Catch::Approx(0.0f));
  REQUIRE(acl::radius(bounds1) == Catch::Approx(0.0f));
  REQUIRE(acl::x(acl::half_extends(bounds1)) == Catch::Approx(0.0f));
  REQUIRE(acl::y(acl::half_extends(bounds1)) == Catch::Approx(0.0f));
  REQUIRE(acl::z(acl::half_extends(bounds1)) == Catch::Approx(0.0f));
}

TEST_CASE("Validate bounding_volume::from_box", "[bounding_volume::from_box]")
{
  acl::bounding_volume bounds1 = acl::make_bounding_volume(acl::vec3a(2.0f, 1.0f, 5.0f), acl::vec3a(4.0f, 1.0f, 6.0f));
  REQUIRE(acl::x(acl::center(bounds1)) == Catch::Approx(2.0f));
  REQUIRE(acl::y(acl::center(bounds1)) == Catch::Approx(1.0f));
  REQUIRE(acl::z(acl::center(bounds1)) == Catch::Approx(5.0f));
  REQUIRE(acl::radius(bounds1) == Catch::Approx(7.28011f));
  REQUIRE(acl::x(acl::half_extends(bounds1)) == Catch::Approx(4.0f));
  REQUIRE(acl::y(acl::half_extends(bounds1)) == Catch::Approx(1.0f));
  REQUIRE(acl::z(acl::half_extends(bounds1)) == Catch::Approx(6.0f));
}

TEST_CASE("Validate bounding_volume", "[bounding_volume]")
{
  acl::bounding_volume bounds1 = {
    acl::sphere(5.0f, 2.2f, 5.0f, 3.4641f),
    acl::vec3a(2.0f, 2.0f, 2.0f)
  };

  bounds1 = acl::make_bounding_volume(acl::vec3a(2.0f, 1.0f, 5.0f), acl::vec3a(4.0f, 1.0f, 6.0f), 1100.0f);
  REQUIRE(acl::x(acl::center(bounds1)) == Catch::Approx(2.0f));
  REQUIRE(acl::y(acl::center(bounds1)) == Catch::Approx(1.0f));
  REQUIRE(acl::z(acl::center(bounds1)) == Catch::Approx(5.0f));
  REQUIRE(acl::radius(bounds1) == Catch::Approx(1100.0f));
  REQUIRE(acl::x(acl::half_extends(bounds1)) == Catch::Approx(4.0f));
  REQUIRE(acl::y(acl::half_extends(bounds1)) == Catch::Approx(1.0f));
  REQUIRE(acl::z(acl::half_extends(bounds1)) == Catch::Approx(6.0f));

  bounds1 =
    acl::make_bounding_volume(acl::sphere(12.0f, 31.0f, 5.0f, 22.0f), acl::vec3a(14.0f, 15.0f, 61.0f));
  REQUIRE(acl::x(acl::center(bounds1)) == Catch::Approx(12.0f));
  REQUIRE(acl::y(acl::center(bounds1)) == Catch::Approx(31.0f));
  REQUIRE(acl::z(acl::center(bounds1)) == Catch::Approx(5.0f));
  REQUIRE(acl::radius(bounds1) == Catch::Approx(22.0f));
  REQUIRE(acl::x(acl::half_extends(bounds1)) == Catch::Approx(14.0f));
  REQUIRE(acl::y(acl::half_extends(bounds1)) == Catch::Approx(15.0f));
  REQUIRE(acl::z(acl::half_extends(bounds1)) == Catch::Approx(61.0f));
}

TEST_CASE("Validate bounding_volume::update(matrix)", "[bounding_volume::update(matrix)]")
{
  acl::bounding_volume bounds = {
    acl::sphere(5.0f, 2.2f, 5.0f, 3.4641f),
    acl::vec3a(2.0f, 2.0f, 2.0f) };
  acl::bounding_volume bounds_orig = {
    acl::sphere(0.0f, 0.0f, 0.0f, 3.4641f),
    acl::vec3a(2.0f, 2.0f, 2.0f),
  };

  acl::mat4 m = acl::make_mat4(2.0f, acl::make_quaternion(acl::vec3(0.0f, 1.0f, 0.0f), acl::to_radians(45.0f)), acl::vec3a(2.0f, 0.0f, 0.0f));

  bounds = (bounds_orig * m);

  REQUIRE(acl::x(acl::center(bounds)) == Catch::Approx(2.0f));
  REQUIRE(acl::y(acl::center(bounds)) == Catch::Approx(0.0f));
  REQUIRE(acl::z(acl::center(bounds)) == Catch::Approx(0.0f));
  REQUIRE(acl::radius(bounds) == Catch::Approx(6.9282f));
  REQUIRE(acl::x(acl::half_extends(bounds)) == Catch::Approx(5.65685f));
  REQUIRE(acl::y(acl::half_extends(bounds)) == Catch::Approx(4.0f));
  REQUIRE(acl::z(acl::half_extends(bounds)) == Catch::Approx(5.65685));
}

TEST_CASE("Validate bounding_volume::update(srt)", "[bounding_volume::update(srt)]")
{
  acl::bounding_volume bounds1 = {
    acl::sphere(5.0f, 2.2f, 5.0f, 3.4641f),
    acl::vec3a(2.0f, 2.0f, 2.0f) };
  acl::bounding_volume bounds_orig = {
    acl::sphere(0.0f, 0.0f, 0.0f, 3.4641f),
    acl::vec3a(2.0f, 2.0f, 2.0f)
  };

  bounds1 = acl::transform(bounds_orig, 2.0f,
                               acl::make_quaternion(acl::vec3(0.0f, 1.0f, 0.0f), acl::to_radians(45.0f)),
                               acl::vec3a(2.0f, 0.0f, 0.0f));

  REQUIRE(acl::x(acl::center(bounds1)) == Catch::Approx(2.0f));
  REQUIRE(acl::y(acl::center(bounds1)) == Catch::Approx(0.0f));
  REQUIRE(acl::z(acl::center(bounds1)) == Catch::Approx(0.0f));
  REQUIRE(acl::radius(bounds1) == Catch::Approx(6.9282f));
  REQUIRE(acl::x(acl::half_extends(bounds1)) == Catch::Approx(5.65685f));
  REQUIRE(acl::y(acl::half_extends(bounds1)) == Catch::Approx(4.0f));
  REQUIRE(acl::z(acl::half_extends(bounds1)) == Catch::Approx(5.65685));
}

TEST_CASE("Validate bounding_volume::update(transform)", "[bounding_volume::update(transform)]")
{
  acl::bounding_volume bounds1 = {
    acl::sphere(5.0f, 2.2f, 5.0f, 3.4641f),
    acl::vec3a(2.0f, 2.0f, 2.0f)};
  acl::bounding_volume bounds_orig = {
    acl::sphere(0.0f, 0.0f, 0.0f, 3.4641f),
    acl::vec3a(2.0f, 2.0f, 2.0f)
  };

  acl::transform tf;
  tf.rotation = acl::make_quaternion(acl::vec3(0.0f, 1.0f, 0.0f), acl::to_radians(45.0f));
  tf.translation_and_scale = acl::vec4(2.0f, 0.0f, 0.0f, 2.0f);

  bounds1 = (bounds_orig * tf);

  REQUIRE(acl::x(acl::center(bounds1)) == Catch::Approx(2.0f));
  REQUIRE(acl::y(acl::center(bounds1)) == Catch::Approx(0.0f));
  REQUIRE(acl::z(acl::center(bounds1)) == Catch::Approx(0.0f));
  REQUIRE(acl::radius(bounds1) == Catch::Approx(6.9282f));
  REQUIRE(acl::x(acl::half_extends(bounds1)) == Catch::Approx(5.65685f));
  REQUIRE(acl::y(acl::half_extends(bounds1)) == Catch::Approx(4.0f));
  REQUIRE(acl::z(acl::half_extends(bounds1)) == Catch::Approx(5.65685));
}

TEST_CASE("Validate bounding_volume::update(bounding_volume)", "[bounding_volume::update(bounding_volume)]")
{
  acl::bounding_volume bounds1 = {
    acl::sphere(5.0f, 4.0f, 5.0f, 3.4641f),
    acl::vec3a(2.0f, 2.0f, 2.0f)
  };
  
  acl::bounding_volume_t bounds2 = {
    acl::sphere(-5.0f, -4.0f, 5.0f, 13.4641f),
    acl::vec3a(10.0f, 10.0f, 10.0f)
  };
  
  acl::append(bounds1, bounds2);

  REQUIRE(acl::x(acl::center(bounds1)) == Catch::Approx(-4.0f));
  REQUIRE(acl::y(acl::center(bounds1)) == Catch::Approx(-4.0f));
  REQUIRE(acl::z(acl::center(bounds1)) == Catch::Approx(5.0f));
  REQUIRE(acl::radius(bounds1) == Catch::Approx(14.86722f));
  REQUIRE(acl::x(acl:::half_extends(bounds1)) == Catch::Approx(11.0f));
  REQUIRE(acl::y(acl:::half_extends(bounds1)) == Catch::Approx(10.0f));
  REQUIRE(acl::z(acl:::half_extends(bounds1)) == Catch::Approx(10.0f));
}

TEST_CASE("Validate bounding_volume::update(points)", "[bounding_volume::update(points)]")
{
  acl::bounding_volume bounds1 = {
    acl::sphere(5.0f, 4.0f, 5.0f, 3.4641f),
    acl::vec3a(2.0f, 2.0f, 2.0f)
  };

  acl::vec3a_t points[3] = {acl::vec3a(-5.0f, -4.0f, 5.0f), acl::vec3a(-15.0f, -14.0f, -5.0f),
                            acl::vec3a(5.0f, 6.0f, 15.0f)};

  acl::append(bounds1, points, 3);

  REQUIRE(acl::x(acl::center(bounds1)) == Catch::Approx(-4.0f));
  REQUIRE(acl::y(acl::center(bounds1)) == Catch::Approx(-4.0f));
  REQUIRE(acl::z(acl::center(bounds1)) == Catch::Approx(5.0f));
  REQUIRE(acl::radius(bounds1) == Catch::Approx(17.91647f));
  REQUIRE(acl::x(acl::half_extends(bounds1)) == Catch::Approx(11.0f));
  REQUIRE(acl::y(acl::half_extends(bounds1)) == Catch::Approx(10.0f));
  REQUIRE(acl::z(acl::half_extends(bounds1)) == Catch::Approx(10.0f));
}
