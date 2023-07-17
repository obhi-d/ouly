#include <acl/math/vml.hpp>
#include <catch2/catch_all.hpp>

TEMPLATE_TEST_CASE("BV: Validate bounds_info_t<TestType>::update", "[bounds_info_t<TestType>::update]", float, double)
{
  acl::bounds_info_t<TestType> bounds = {
    {-2.0f, -2.0f, -2.0f},
    { 2.0f,  2.0f,  2.0f},
    3.4641f
  };

  acl::bounds_info_t<TestType> bounds1 = {
    {-2.0f, -2.0f, -2.0f},
    { 2.0f,  2.0f,  2.0f},
    3.4641f
  };
  acl::bounds_info_t<TestType> bounds2 = {
    {2.0f, 2.0f, 2.0f},
    {2.0f, 2.0f, 2.0f},
    3.4641f
  };

  bounds = (bounds + bounds1);
  bounds += bounds2;

  REQUIRE(acl::get_x(bounds.center) == Catch::Approx(0.0f));
  REQUIRE(acl::get_y(bounds.center) == Catch::Approx(0.0f));
  REQUIRE(acl::get_z(bounds.center) == Catch::Approx(0.0f));

  REQUIRE(acl::get_x(bounds.half_extends) == Catch::Approx(4.0));
  REQUIRE(acl::get_y(bounds.half_extends) == Catch::Approx(4.0));
  REQUIRE(acl::get_z(bounds.half_extends) == Catch::Approx(4.0));

  REQUIRE(bounds.radius == Catch::Approx(6.9282));
}

TEMPLATE_TEST_CASE("BV: Validate bounding_volume_t<TestType>::center", "[bounding_volume_t<TestType>::center]", float,
                   double)
{
  acl::bounding_volume_t<TestType> bounds = {acl::sphere_t<TestType>(5.0f, 2.2f, 5.0f, 3.4641f),
                                             acl::vec3a_t<TestType>(2.0f, 2.0f, 2.0f)};

  REQUIRE(acl::get_x(acl::center(bounds)) == Catch::Approx(5.0f));
  REQUIRE(acl::get_y(acl::center(bounds)) == Catch::Approx(2.2f));
  REQUIRE(acl::get_z(acl::center(bounds)) == Catch::Approx(5.0f));
}

TEMPLATE_TEST_CASE("BV: Validate bounding_volume_t<TestType>::half_extends",
                   "[bounding_volume_t<TestType>::half_extends]", float, double)
{
  acl::bounding_volume_t<TestType> bounds1 = {acl::sphere_t<TestType>(5.0f, 2.2f, 5.0f, 3.4641f),
                                              acl::vec3a_t<TestType>(2.0f, 2.0f, 2.0f)};

  REQUIRE(acl::get_x(acl::half_extends(bounds1)) == Catch::Approx(2.0f));
  REQUIRE(acl::get_y(acl::half_extends(bounds1)) == Catch::Approx(2.0f));
  REQUIRE(acl::get_z(acl::half_extends(bounds1)) == Catch::Approx(2.0f));
}

TEMPLATE_TEST_CASE("BV: Validate bounding_volume_t<TestType>::radius", "[bounding_volume_t<TestType>::radius]", float,
                   double)
{
  acl::bounding_volume_t<TestType> bounds1 = {acl::sphere_t<TestType>(5.0f, 2.2f, 5.0f, 3.4641f),
                                              acl::vec3a_t<TestType>(2.0f, 2.0f, 2.0f)};

  REQUIRE(acl::radius(bounds1) == Catch::Approx(3.4641f));
}

TEMPLATE_TEST_CASE("BV: Validate bounding_volume_t<TestType>::vradius", "[bounding_volume_t<TestType>::vradius]", float,
                   double)
{
  acl::bounding_volume_t<TestType> bounds1 = {acl::sphere_t<TestType>(5.0f, 2.2f, 5.0f, 3.4641f),
                                              acl::vec3a_t<TestType>(2.0f, 2.0f, 2.0f)};

  REQUIRE(acl::get_x(acl::vradius(bounds1)) == Catch::Approx(3.4641f));
}

TEMPLATE_TEST_CASE("BV: Validate bounding_volume_t<TestType>::nullify", "[bounding_volume_t<TestType>::nullify]", float,
                   double)
{
  acl::bounding_volume_t<TestType> bounds1 = {acl::sphere_t<TestType>(5.0f, 2.2f, 5.0f, 3.4641f),
                                              acl::vec3a_t<TestType>(2.0f, 2.0f, 2.0f)};
  acl::nullify(bounds1);
  REQUIRE(acl::get_x(acl::center(bounds1)) == Catch::Approx(0.0f));
  REQUIRE(acl::get_y(acl::center(bounds1)) == Catch::Approx(0.0f));
  REQUIRE(acl::get_z(acl::center(bounds1)) == Catch::Approx(0.0f));
  REQUIRE(acl::radius(bounds1) == Catch::Approx(0.0f));
  REQUIRE(acl::get_x(acl::half_extends(bounds1)) == Catch::Approx(0.0f));
  REQUIRE(acl::get_y(acl::half_extends(bounds1)) == Catch::Approx(0.0f));
  REQUIRE(acl::get_z(acl::half_extends(bounds1)) == Catch::Approx(0.0f));
}

TEMPLATE_TEST_CASE("BV: Validate bounding_volume_t<TestType>::from_box", "[bounding_volume_t<TestType>::from_box]",
                   float, double)
{
  acl::bounding_volume_t<TestType> bounds1 =
    acl::make_bounding_volume(acl::vec3a_t<TestType>(2.0f, 1.0f, 5.0f), acl::vec3a_t<TestType>(4.0f, 1.0f, 6.0f));
  REQUIRE(acl::get_x(acl::center(bounds1)) == Catch::Approx(2.0f));
  REQUIRE(acl::get_y(acl::center(bounds1)) == Catch::Approx(1.0f));
  REQUIRE(acl::get_z(acl::center(bounds1)) == Catch::Approx(5.0f));
  REQUIRE(acl::radius(bounds1) == Catch::Approx(7.28011f));
  REQUIRE(acl::get_x(acl::half_extends(bounds1)) == Catch::Approx(4.0f));
  REQUIRE(acl::get_y(acl::half_extends(bounds1)) == Catch::Approx(1.0f));
  REQUIRE(acl::get_z(acl::half_extends(bounds1)) == Catch::Approx(6.0f));
}

TEMPLATE_TEST_CASE("BV: Validate bounding_volume_t<TestType>", "[bounding_volume_t<TestType>]", float, double)
{
  acl::bounding_volume_t<TestType> bounds1 = {acl::sphere_t<TestType>(5.0f, 2.2f, 5.0f, 3.4641f),
                                              acl::vec3a_t<TestType>(2.0f, 2.0f, 2.0f)};

  bounds1 = acl::make_bounding_volume(acl::vec3a_t<TestType>(2.0f, 1.0f, 5.0f),
                                      acl::vec3a_t<TestType>(4.0f, 1.0f, 6.0f), 1100.0f);
  REQUIRE(acl::get_x(acl::center(bounds1)) == Catch::Approx(2.0f));
  REQUIRE(acl::get_y(acl::center(bounds1)) == Catch::Approx(1.0f));
  REQUIRE(acl::get_z(acl::center(bounds1)) == Catch::Approx(5.0f));
  REQUIRE(acl::radius(bounds1) == Catch::Approx(1100.0f));
  REQUIRE(acl::get_x(acl::half_extends(bounds1)) == Catch::Approx(4.0f));
  REQUIRE(acl::get_y(acl::half_extends(bounds1)) == Catch::Approx(1.0f));
  REQUIRE(acl::get_z(acl::half_extends(bounds1)) == Catch::Approx(6.0f));

  bounds1 = acl::make_bounding_volume(acl::sphere_t<TestType>(12.0f, 31.0f, 5.0f, 22.0f),
                                      acl::vec3a_t<TestType>(14.0f, 15.0f, 61.0f));
  REQUIRE(acl::get_x(acl::center(bounds1)) == Catch::Approx(12.0f));
  REQUIRE(acl::get_y(acl::center(bounds1)) == Catch::Approx(31.0f));
  REQUIRE(acl::get_z(acl::center(bounds1)) == Catch::Approx(5.0f));
  REQUIRE(acl::radius(bounds1) == Catch::Approx(22.0f));
  REQUIRE(acl::get_x(acl::half_extends(bounds1)) == Catch::Approx(14.0f));
  REQUIRE(acl::get_y(acl::half_extends(bounds1)) == Catch::Approx(15.0f));
  REQUIRE(acl::get_z(acl::half_extends(bounds1)) == Catch::Approx(61.0f));
}

TEMPLATE_TEST_CASE("BV: Validate bounding_volume_t<TestType>::update(matrix)",
                   "[bounding_volume_t<TestType>::update(matrix)]", float, double)
{
  acl::bounding_volume_t<TestType> bounds      = {acl::sphere_t<TestType>(5.0f, 2.2f, 5.0f, 3.4641f),
                                                  acl::vec3a_t<TestType>(2.0f, 2.0f, 2.0f)};
  acl::bounding_volume_t<TestType> bounds_orig = {
    acl::sphere_t<TestType>(0.0f, 0.0f, 0.0f, 3.4641f),
    acl::vec3a_t<TestType>(2.0f, 2.0f, 2.0f),
  };

  acl::mat4_t<TestType> m =
    acl::make_mat4(static_cast<TestType>(2.0f),
                   acl::make_quaternion(acl::vec3a_t<TestType>(0.0f, 1.0f, 0.0f), acl::to_radians<TestType>(45.0f)),
                   acl::vec3a_t<TestType>(2.0f, 0.0f, 0.0f));

  REQUIRE(acl::test_orthogonal(m));

  bounds = (bounds_orig * m);

  REQUIRE(acl::get_x(acl::center(bounds)) == Catch::Approx(2.0f));
  REQUIRE(acl::get_y(acl::center(bounds)) == Catch::Approx(0.0f));
  REQUIRE(acl::get_z(acl::center(bounds)) == Catch::Approx(0.0f));
  REQUIRE(acl::radius(bounds) == Catch::Approx(6.9282f));
  REQUIRE(acl::get_x(acl::half_extends(bounds)) == Catch::Approx(5.65685f));
  REQUIRE(acl::get_y(acl::half_extends(bounds)) == Catch::Approx(4.0f));
  REQUIRE(acl::get_z(acl::half_extends(bounds)) == Catch::Approx(5.65685));
}

TEMPLATE_TEST_CASE("BV: Validate bounding_volume_t<TestType>::update(srt)",
                   "[bounding_volume_t<TestType>::update(srt)]", float, double)
{
  acl::bounding_volume_t<TestType> bounds1     = {acl::sphere_t<TestType>(5.0f, 2.2f, 5.0f, 3.4641f),
                                                  acl::vec3a_t<TestType>(2.0f, 2.0f, 2.0f)};
  acl::bounding_volume_t<TestType> bounds_orig = {acl::sphere_t<TestType>(0.0f, 0.0f, 0.0f, 3.4641f),
                                                  acl::vec3a_t<TestType>(2.0f, 2.0f, 2.0f)};

  bounds1 = acl::make_bounding_volume(
    bounds_orig, static_cast<TestType>(2.0f),
    acl::make_quaternion(acl::vec3a_t<TestType>(0.0f, 1.0f, 0.0f), acl::to_radians<TestType>(45.0f)),
    acl::vec3a_t<TestType>(2.0f, 0.0f, 0.0f));

  REQUIRE(acl::get_x(acl::center(bounds1)) == Catch::Approx(2.0f));
  REQUIRE(acl::get_y(acl::center(bounds1)) == Catch::Approx(0.0f));
  REQUIRE(acl::get_z(acl::center(bounds1)) == Catch::Approx(0.0f));
  REQUIRE(acl::radius(bounds1) == Catch::Approx(6.9282f));
  REQUIRE(acl::get_x(acl::half_extends(bounds1)) == Catch::Approx(5.65685f));
  REQUIRE(acl::get_y(acl::half_extends(bounds1)) == Catch::Approx(4.0f));
  REQUIRE(acl::get_z(acl::half_extends(bounds1)) == Catch::Approx(5.65685));
}

TEMPLATE_TEST_CASE("BV: Validate bounding_volume_t<TestType>::update(transform)",
                   "[bounding_volume_t<TestType>::update(transform)]", float, double)
{
  acl::bounding_volume_t<TestType> bounds1     = {acl::sphere_t<TestType>(5.0f, 2.2f, 5.0f, 3.4641f),
                                                  acl::vec3a_t<TestType>(2.0f, 2.0f, 2.0f)};
  acl::bounding_volume_t<TestType> bounds_orig = {acl::sphere_t<TestType>(0.0f, 0.0f, 0.0f, 3.4641f),
                                                  acl::vec3a_t<TestType>(2.0f, 2.0f, 2.0f)};

  acl::transform_t<TestType> tf;
  tf.rotation = acl::make_quaternion(acl::vec3a_t<TestType>(0.0f, 1.0f, 0.0f), acl::to_radians<TestType>(45.0f));
  tf.translation_and_scale = acl::vec4_t<TestType>(2.0f, 0.0f, 0.0f, 2.0f);

  bounds1 = bounds_orig * tf;

  REQUIRE(acl::get_x(acl::center(bounds1)) == Catch::Approx(2.0f));
  REQUIRE(acl::get_y(acl::center(bounds1)) == Catch::Approx(0.0f));
  REQUIRE(acl::get_z(acl::center(bounds1)) == Catch::Approx(0.0f));
  REQUIRE(acl::radius(bounds1) == Catch::Approx(6.9282f));
  REQUIRE(acl::get_x(acl::half_extends(bounds1)) == Catch::Approx(5.65685f));
  REQUIRE(acl::get_y(acl::half_extends(bounds1)) == Catch::Approx(4.0f));
  REQUIRE(acl::get_z(acl::half_extends(bounds1)) == Catch::Approx(5.65685));
}

TEMPLATE_TEST_CASE("BV: Validate bounding_volume_t<TestType>::update(bounding_volume_t<TestType>)",
                   "[bounding_volume_t<TestType>::update(bounding_volume_t<TestType>)]", float, double)
{
  acl::bounding_volume_t<TestType> bounds1 = {acl::sphere_t<TestType>(5.0f, 4.0f, 5.0f, 3.4641f),
                                              acl::vec3a_t<TestType>(2.0f, 2.0f, 2.0f)};

  acl::bounding_volume_t<TestType> bounds2 = {acl::sphere_t<TestType>(-5.0f, -4.0f, 5.0f, 13.4641f),
                                              acl::vec3a_t<TestType>(10.0f, 10.0f, 10.0f)};

  bounds1 = (bounds1 + bounds2);

  REQUIRE(acl::get_x(acl::center(bounds1)) == Catch::Approx(-4.0f));
  REQUIRE(acl::get_y(acl::center(bounds1)) == Catch::Approx(-4.0f));
  REQUIRE(acl::get_z(acl::center(bounds1)) == Catch::Approx(5.0f));
  REQUIRE(acl::radius(bounds1) == Catch::Approx(14.86722f));
  REQUIRE(acl::get_x(acl::half_extends(bounds1)) == Catch::Approx(11.0f));
  REQUIRE(acl::get_y(acl::half_extends(bounds1)) == Catch::Approx(10.0f));
  REQUIRE(acl::get_z(acl::half_extends(bounds1)) == Catch::Approx(10.0f));
}

TEMPLATE_TEST_CASE("BV: Validate bounding_volume_t<TestType>::update(points)",
                   "[bounding_volume_t<TestType>::update(points)]", float, double)
{
  acl::bounding_volume_t<TestType> bounds1 = {acl::sphere_t<TestType>(5.0f, 4.0f, 5.0f, 3.4641f),
                                              acl::vec3a_t<TestType>(2.0f, 2.0f, 2.0f)};

  acl::vec3a_t<TestType> points[3] = {acl::vec3a_t<TestType>(-5.0f, -4.0f, 5.0f),
                                      acl::vec3a_t<TestType>(-15.0f, -14.0f, -5.0f),
                                      acl::vec3a_t<TestType>(5.0f, 6.0f, 15.0f)};

  bounds1 = bounds1 + acl::make_bounding_volume(points, 3);

  REQUIRE(acl::get_x(acl::center(bounds1)) == Catch::Approx(-4.0f));
  REQUIRE(acl::get_y(acl::center(bounds1)) == Catch::Approx(-4.0f));
  REQUIRE(acl::get_z(acl::center(bounds1)) == Catch::Approx(5.0f));
  REQUIRE(acl::radius(bounds1) == Catch::Approx(16.7954292f));
  REQUIRE(acl::get_x(acl::half_extends(bounds1)) == Catch::Approx(11.0f));
  REQUIRE(acl::get_y(acl::half_extends(bounds1)) == Catch::Approx(10.0f));
  REQUIRE(acl::get_z(acl::half_extends(bounds1)) == Catch::Approx(10.0f));
}
