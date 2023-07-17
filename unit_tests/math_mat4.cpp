#include <acl/math/vml.hpp>
#include <catch2/catch_all.hpp>
#include <cstring>

TEMPLATE_TEST_CASE("Mat4: Validate mul", "[mul]", float, double)
{
  acl::mat4_t<TestType> m1 = {
    5.0f, 7.0f, 9.0f, 10.0f, 2.0f, 3.0f, 3.0f, 8.0f, 8.0f, 10.0f, 2.0f, 3.0f, 3.0f, 3.0f, 4.0f, 8.0f,
  };

  acl::mat4_t<TestType> m2 = {
    3.0f, 10.0f, 12.0f, 18.0f, 12.0f, 1.0f, 4.0f, 9.0f, 9.0f, 10.0f, 12.0f, 2.0f, 3.0f, 12.0f, 4.0f, 10.0f,
  };

  acl::mat4_t<TestType> m3 = {
    210.0f, 267.0f, 236.0f, 271.0f, 93.0f,  149.0f, 104.0f, 149.0f,
    171.0f, 146.0f, 172.0f, 268.0f, 105.0f, 169.0f, 128.0f, 169.0f,
  };

  acl::mat4_t<TestType> m2_times_3 = {
    9.0f, 30.0f, 36.0f, 54.0f, 36.0f, 3.0f, 12.0f, 27.0f, 27.0f, 30.0f, 36.0f, 6.0f, 9.0f, 36.0f, 12.0f, 30.0f,
  };

  acl::mat4_t<TestType> identity;

  acl::mat4_t<TestType> im1  = identity * m1;
  acl::mat4_t<TestType> m1i  = m1 * identity;
  acl::mat4_t<TestType> m1m2 = m1 * m2;

  CHECK(acl::equals<TestType>(im1, m1i));
  CHECK(std::memcmp(&im1, &m1i, sizeof(im1)) == 0);
  CHECK(acl::equals<TestType>(m1m2, m3));

  CHECK(acl::equals<TestType>(3.0f * m2, m2_times_3));
  CHECK(acl::equals<TestType>(m1[0] * m2, m1m2[0]));
}

TEMPLATE_TEST_CASE("Mat4: Validate transform_assume_ortho", "[transform_assume_ortho]", float, double)
{
  acl::mat4_t<TestType> m = {
    0.0f, 0.80f, 0.60f, 0.0f, -0.80f, -0.36f, 0.48f, 0.0f, -0.60f, 0.48f, -0.64f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
  };

  acl::vec3a_t<TestType> const points[4] = {
    { 3.000f, 10.000f, 12.000f},
    {12.000f,  1.000f,  4.000f},
    { 9.000f, 10.000f, 12.000f},
    { 3.000f, 12.000f,  4.000f},
  };

  acl::vec3a_t<TestType> const expected[4] = {
    { -15.2f,  4.56f, -1.08f},
    {  -3.2f, 11.16f,  5.12f},
    { -15.2f,  9.36f,  2.52f},
    {-12.00f,   0.0f,   5.0f},
  };

  acl::vec3a_t<TestType> output[4];

  for (int i = 0; i < 4; ++i)
  {
    output[i] = points[i] * m;
    CHECK(acl::equals<TestType>(output[i], expected[i]));
    output[i] = points[i];
  }

  CHECK(acl::equals<TestType>(acl::vec3a_t<TestType>(3.000f, 10.000f, 12.000f) * m,
                              acl::vec3a_t<TestType>(-15.2000008f, 4.55999947f, -1.08f)));
}

TEMPLATE_TEST_CASE("Mat4: Validate transform_and_project", "[transform_and_project]", float, double)
{
  acl::mat4_t<TestType> m = {
    5.0f, 7.0f, 9.0f, 10.0f, 2.0f, 3.0f, 3.0f, 8.0f, 8.0f, 10.0f, 2.0f, 3.0f, 3.0f, 3.0f, 4.0f, 8.0f,
  };

  acl::vec4_t<TestType> const points[4] = {
    { 3.000f, 10.000f, 12.000f},
    {12.000f,  1.000f,  4.000f},
    { 9.000f, 10.000f, 12.000f},
    { 3.000f, 12.000f,  4.000f},
  };

  acl::vec4_t<TestType> const expected[4] = {
    {0.87012987013f, 1.12987012987f, 0.55194805194f},
    {0.65540540540f, 0.87837837837f, 0.83108108108f},
    {0.76635514018f, 1.00934579439f, 0.64953271028f},
    {0.50684931506f, 0.68493150684f, 0.51369863013f},
  };

  acl::vec4_t<TestType> output[4];

  for (int i = 0; i < 4; ++i)
  {
    output[i] = points[i] * m;
    output[i] /= acl::vec4_t<TestType>(output[i].w);
    CHECK(acl::equals<TestType>(output[i], expected[i]));
    output[i] = points[i];
  }

  auto v = acl::vec4_t<TestType>(3.000f, 10.000f, 12.000f);
  v      = v * m;
  v /= acl::vec4_t<TestType>(v.w);
  CHECK(acl::equals<TestType>(v, acl::vec4_t<TestType>(0.87012987013f, 1.12987012987f, 0.55194805194f, 1.0f)));
}

TEMPLATE_TEST_CASE("Mat4: Validate transform_aabb", "[transform_aabb]", float, double)
{
  acl::aabb_t<TestType> aabb =
    acl::make_aabb_from_center_extends(acl::vec3a_t<TestType>(), acl::vec3a_t<TestType>(4, 2, 2));
  acl::mat4_t<TestType> scale  = acl::make_mat4_from_scale(acl::vec3a_t<TestType>(2, 2, 2));
  acl::mat4_t<TestType> rotate = acl::make_mat4_form_quaternion(
    acl::make_quaternion(acl::make_axis_angle(acl::vec3a_t<TestType>(0, 0, 1), acl::to_radians<TestType>(90.0f))));
  acl::mat4_t<TestType> translate = acl::make_mat4_from_translation(acl::vec3a_t<TestType>(10, 0, 0));
  acl::mat4_t<TestType> combined  = scale * rotate * translate;
  acl::aabb_t<TestType> expected =
    acl::make_aabb_from_center_extends(acl::vec3a_t<TestType>(10, 0, 0), acl::vec3a_t<TestType>(4, 8, 4));

  acl::aabb_t<TestType> result = aabb * combined;

  CHECK(acl::equals<TestType>(expected, result));
}

TEMPLATE_TEST_CASE("Mat4: Validate from_perspective_projection", "[from_perspective_projection]", float, double)
{
  acl::mat4_t<TestType> proj    = acl::make_perspective_projection<TestType>(acl::k_pi_by_2, 1.2f, 1.0f, 100.0f);
  float                 y_scale = 1.0f / std::tan(static_cast<float>(3.14159265358979323846 / 4.0));
  float                 x_scale = y_scale / 1.2f;
  CHECK(acl::get(proj, 0, 0) == Catch::Approx(x_scale));
  CHECK(acl::get(proj, 0, 1) == Catch::Approx(0.0f));
  CHECK(acl::get(proj, 0, 2) == Catch::Approx(0.0f));
  CHECK(acl::get(proj, 0, 3) == Catch::Approx(0.0f));
  CHECK(acl::get(proj, 1, 0) == Catch::Approx(0.0f));
  CHECK(acl::get(proj, 1, 1) == Catch::Approx(y_scale));
  CHECK(acl::get(proj, 1, 2) == Catch::Approx(0.0f));
  CHECK(acl::get(proj, 1, 3) == Catch::Approx(0.0f));
  CHECK(acl::get(proj, 2, 0) == Catch::Approx(0.0f));
  CHECK(acl::get(proj, 2, 1) == Catch::Approx(0.0f));
  CHECK(acl::get(proj, 2, 2) == Catch::Approx(100.0f / 99.0f));
  CHECK(acl::get(proj, 2, 3) == Catch::Approx(1.0f));
  CHECK(acl::get(proj, 3, 0) == Catch::Approx(0.0f));
  CHECK(acl::get(proj, 3, 1) == Catch::Approx(0.0f));
  CHECK(acl::get(proj, 3, 2) == Catch::Approx(-1.0101f));
  CHECK(acl::get(proj, 3, 3) == Catch::Approx(0.0f));
}

TEMPLATE_TEST_CASE("Mat4: Validate inverse", "[inverse]", float, double)
{
  acl::mat4_t<TestType> m   = {3.6f, 6.3f, 4.4f, 6.7f, 1.2f, 5.7f, 6.5f, 2.2f,
                               7.8f, 5.5f, 3.6f, 7.7f, 3.3f, 5.3f, 5.6f, 7.7f};
  acl::mat4_t<TestType> inv = {-0.232841581f, 0.0817205757f, 0.255250692f,   -0.0759970918f,
                               0.561829031f,  0.0166856032f, -0.0919162259f, -0.401715338f,
                               -0.467710704f, 0.185540006f,  0.0651223063f,  0.288835317f,
                               0.0532288961f, -0.181446120f, -0.0934877992f, 0.228883758f};

  CHECK(acl::equals<TestType>(acl::inverse(m), inv));

  acl::mat4_t<TestType> o = {
    0.0f, 0.80f, 0.60f, 0.0f, -0.80f, -0.36f, 0.48f, 0.0f, -0.60f, 0.48f, -0.64f, 0.0f, 12.0f, 20.0f, 3.0f, 1.0f,
  };

  acl::mat4_t<TestType> oi = {0.0f, -0.8f, -0.6f,  0.0f, 0.8f,   -0.36f,      0.48f,  0.0f,
                              0.6f, 0.48f, -0.64f, 0.0f, -17.8f, 15.3600016f, -0.48f, 1.0f};

  CHECK(acl::equals<TestType>(acl::inverse_assume_ortho(o), oi));
}
