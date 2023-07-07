#include <catch2/catch_all.hpp>
#include <cstring>
#include <acl/math/vml.hpp>

TEST_CASE("Validate mat4::mul", "[mat4::mul]")
{
  acl::mat4_t m1 = {
    5.0f, 7.0f, 9.0f, 10.0f, 2.0f, 3.0f, 3.0f, 8.0f, 8.0f, 10.0f, 2.0f, 3.0f, 3.0f, 3.0f, 4.0f, 8.0f,
  };

  acl::mat4_t m2 = {
    3.0f, 10.0f, 12.0f, 18.0f, 12.0f, 1.0f, 4.0f, 9.0f, 9.0f, 10.0f, 12.0f, 2.0f, 3.0f, 12.0f, 4.0f, 10.0f,
  };

  acl::mat4_t m3 = {
    210.0f, 267.0f, 236.0f, 271.0f, 93.0f,  149.0f, 104.0f, 149.0f,
    171.0f, 146.0f, 172.0f, 268.0f, 105.0f, 169.0f, 128.0f, 169.0f,
  };

  acl::mat4_t m2_times_3 = {
    9.0f, 30.0f, 36.0f, 54.0f, 36.0f, 3.0f, 12.0f, 27.0f, 27.0f, 30.0f, 36.0f, 6.0f, 9.0f, 36.0f, 12.0f, 30.0f,
  };

  acl::mat4_t identity = acl::mat4::identity();

  acl::mat4_t im1  = acl::mat4::mul(identity, m1);
  acl::mat4_t m1i  = acl::mat4::mul(m1, identity);
  acl::mat4_t m1m2 = acl::mat4::mul(m1, m2);

  CHECK(acl::mat4::equals(im1, m1i));
  CHECK(std::memcmp(&im1, &m1i, sizeof(im1)) == 0);
  CHECK(acl::mat4::equals(m1m2, m3));

  CHECK(acl::mat4::equals(acl::mat4::mul(3.0f, m2), m2_times_3));
  CHECK(acl::vec4::equals(acl::mat4::mul(acl::mat4::row(m1, 0), m2), acl::mat4::row(m1m2, 0)));
}

TEST_CASE("Validate mat4::transform_assume_ortho", "[mat4::transform_assume_ortho]")
{
  acl::mat4_t m = {
    0.0f, 0.80f, 0.60f, 0.0f, -0.80f, -0.36f, 0.48f, 0.0f, -0.60f, 0.48f, -0.64f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
  };

  acl::vec3_t const points[4] = {
    {3.000f, 10.000f, 12.000f},
    {12.000f, 1.000f, 4.000f},
    {9.000f, 10.000f, 12.000f},
    {3.000f, 12.000f, 4.000f},
  };

  acl::vec3_t const expected[4] = {
    {-15.2f, 4.56f, -1.08f},
    {-3.2f, 11.16f, 5.12f},
    {-15.2f, 9.36f, 2.52f},
    {-12.00f, 0.0f, 5.0f},
  };

  acl::vec3_t output[4];

  acl::mat4::transform_assume_ortho(m, points, sizeof(acl::vec3_t), 4, output, sizeof(acl::vec3_t));

  for (int i = 0; i < 4; ++i)
  {
    CHECK(acl::vec3::equals(output[i], expected[i]));
    output[i] = points[i];
  }

  acl::mat4::transform_assume_ortho(m, output, sizeof(acl::vec3_t), 4);

  for (int i = 0; i < 4; ++i)
  {
    CHECK(acl::vec3::equals(output[i], expected[i]));
  }

  CHECK(acl::vec3a::equals(
    acl::vec3a::from_vec4(acl::mat4::transform_assume_ortho(m, acl::vec3a::set(3.000f, 10.000f, 12.000f))),
    acl::vec3a::set(-15.2000008f, 4.55999947f, -1.08f)));
}

TEST_CASE("Validate mat4::transform_and_project", "[mat4::transform_and_project]")
{
  acl::mat4_t m = {
    5.0f, 7.0f, 9.0f, 10.0f, 2.0f, 3.0f, 3.0f, 8.0f, 8.0f, 10.0f, 2.0f, 3.0f, 3.0f, 3.0f, 4.0f, 8.0f,
  };

  acl::vec3_t const points[4] = {
    {3.000f, 10.000f, 12.000f},
    {12.000f, 1.000f, 4.000f},
    {9.000f, 10.000f, 12.000f},
    {3.000f, 12.000f, 4.000f},
  };

  acl::vec3_t const expected[4] = {
    {0.87012987013f, 1.12987012987f, 0.55194805194f},
    {0.65540540540f, 0.87837837837f, 0.83108108108f},
    {0.76635514018f, 1.00934579439f, 0.64953271028f},
    {0.50684931506f, 0.68493150684f, 0.51369863013f},
  };

  acl::vec3_t output[4];

  acl::mat4::transform_and_project(m, points, sizeof(acl::vec3_t), 4, output, sizeof(acl::vec3_t));

  for (int i = 0; i < 4; ++i)
  {
    CHECK(acl::vec3::equals(output[i], expected[i]));
    output[i] = points[i];
  }

  CHECK(acl::vec3a::equals(acl::mat4::transform_and_project(m, acl::vec3a::set(3.000, 10.000, 12.000)),
                           acl::vec4::set(0.87012987013f, 1.12987012987f, 0.55194805194f, 1.0f)));
}

TEST_CASE("Validate mat4::transform_aabb", "[mat4::transform_aabb]")
{
  acl::aabb_t aabb      = acl::aabb::set(acl::vec3a::zero(), acl::vec3a::set(4, 2, 2));
  acl::mat4_t scale     = acl::mat4::from_scale(acl::vec3a::set(2, 2, 2));
  acl::mat4_t rotate    = acl::mat4::from_rotation(acl::quat::from_axis_angle(
       acl::axis_angle::set_assume_normalized(acl::vec3a::set(0, 0, 1), acl::to_radians(90.0))));
  acl::mat4_t translate = acl::mat4::from_translation(acl::vec3a::set(10, 0, 0));
  acl::mat4_t combined  = acl::mat4::mul(scale, acl::mat4::mul(rotate, translate));
  acl::aabb_t expected  = acl::aabb::set(acl::vec3a::set(10, 0, 0), acl::vec3a::set(4, 8, 4));

  acl::aabb_t result = acl::mat4::transform_aabb(combined, aabb);

  CHECK(acl::aabb::equals(expected, result));
}

TEST_CASE("Validate mat4::from_perspective_projection", "[mat4::from_perspective_projection]")
{
  acl::mat4_t proj    = acl::mat4::from_perspective_projection(acl::k_pi_by_2, 1.2f, 1.0f, 100.0f);
  float       y_scale = 1.0f / std::tan(static_cast<float>(3.14159265358979323846 / 4.0));
  float       x_scale = y_scale / 1.2f;
  CHECK(acl::mat4::get(proj, 0, 0) == Catch::Approx(x_scale));
  CHECK(acl::mat4::get(proj, 0, 1) == Catch::Approx(0.0f));
  CHECK(acl::mat4::get(proj, 0, 2) == Catch::Approx(0.0f));
  CHECK(acl::mat4::get(proj, 0, 3) == Catch::Approx(0.0f));
  CHECK(acl::mat4::get(proj, 1, 0) == Catch::Approx(0.0f));
  CHECK(acl::mat4::get(proj, 1, 1) == Catch::Approx(y_scale));
  CHECK(acl::mat4::get(proj, 1, 2) == Catch::Approx(0.0f));
  CHECK(acl::mat4::get(proj, 1, 3) == Catch::Approx(0.0f));
  CHECK(acl::mat4::get(proj, 2, 0) == Catch::Approx(0.0f));
  CHECK(acl::mat4::get(proj, 2, 1) == Catch::Approx(0.0f));
  CHECK(acl::mat4::get(proj, 2, 2) == Catch::Approx(100.0f / 99.0f));
  CHECK(acl::mat4::get(proj, 2, 3) == Catch::Approx(1.0f));
  CHECK(acl::mat4::get(proj, 3, 0) == Catch::Approx(0.0f));
  CHECK(acl::mat4::get(proj, 3, 1) == Catch::Approx(0.0f));
  CHECK(acl::mat4::get(proj, 3, 2) == Catch::Approx(-1.0101f));
  CHECK(acl::mat4::get(proj, 3, 3) == Catch::Approx(0.0f));
}

TEST_CASE("Validate mat4::inverse", "[mat4::inverse]")
{
  acl::mat4_t m   = {3.6, 6.3, 4.4, 6.7, 1.2, 5.7, 6.5, 2.2, 7.8, 5.5, 3.6, 7.7, 3.3, 5.3, 5.6, 7.7};
  acl::mat4_t inv = {-0.232841581f,  0.0817205757f, 0.255250692f,   -0.0759970918f, 0.561829031f,  0.0166856032f,
                     -0.0919162259f, -0.401715338f, -0.467710704f,  0.185540006f,   0.0651223063f, 0.288835317f,
                     0.0532288961f,  -0.181446120f, -0.0934877992f, 0.228883758f};

  CHECK(acl::mat4::equals(acl::mat4::inverse(m), inv));

  acl::mat4_t o = {
    0.0f, 0.80f, 0.60f, 0.0f, -0.80f, -0.36f, 0.48f, 0.0f, -0.60f, 0.48f, -0.64f, 0.0f, 12.0f, 20.0f, 3.0f, 1.0f,
  };

  acl::mat4_t oi = {0, -0.8f, -0.6f, 0, 0.8f, -0.36, 0.48f, 0, 0.6f, 0.48f, -0.64f, 0, -17.8f, 15.3600016f, -0.48f, 1};

  CHECK(acl::mat4::equals(acl::mat4::inverse_assume_ortho(o), oi));
}