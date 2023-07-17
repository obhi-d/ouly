#include <acl/math/vml.hpp>
#include <catch2/catch_all.hpp>
#include <cmath>
#include <limits>

TEMPLATE_TEST_CASE("Validate cross", "[cross]", float, double)
{
  acl::vec3_t<TestType> v1       = acl::vec3_t<TestType>(4, 6, 1);
  acl::vec3_t<TestType> v2       = acl::vec3_t<TestType>(3, 8, 5);
  acl::vec3_t<TestType> expected = acl::vec3_t<TestType>(22, -17, 14);
  CHECK(acl::equals(expected, acl::cross(v1, v2)));
}

TEMPLATE_TEST_CASE("Validate compare", "[compare]", float, double)
{
  acl::vec3a_t<TestType> p    = acl::vec3a_t<TestType>(-441.3f, 23.0f, -1.0f);
  auto                   copy = p;
  acl::vec3a_t<TestType> q    = acl::vec3a_t<TestType>(441.3f, 5.0f, 51.0f);
  acl::vec3a_t<TestType> r    = acl::vec3a_t<TestType>(445.3f, 15.0f, 151.0f);
  CHECK(p == copy);
  CHECK(acl::greater_any(p, q));
  CHECK(acl::greater_all(p, q) == false);
  CHECK(acl::lesser_any(p, q));
  CHECK(acl::lesser_all(p, q) == false);
  CHECK(acl::greater_any(q, r) == false);
  CHECK(acl::greater_all(q, r) == false);
  CHECK(acl::lesser_any(q, r));
  CHECK(acl::lesser_all(q, r));
  CHECK(acl::greater_any(r, q));
  CHECK(acl::greater_all(r, q));
  CHECK(acl::lesser_any(r, q) == false);
  CHECK(acl::lesser_all(r, q) == false);
}

TEMPLATE_TEST_CASE("Validate vec4_t<TestType>::mul", "[vec4_t<TestType>::mul]", float, double)
{
  acl::mat4_t<TestType> m = {
    0.0f, 0.80f, 0.60f, 0.0f, -0.80f, -0.36f, 0.48f, 0.0f, -0.60f, 0.48f, -0.64f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
  };
  acl::vec4_t<TestType> v        = acl::vec4_t<TestType>(3.000f, 10.000f, 12.000f, 1.0f);
  acl::vec4_t<TestType> expected = acl::vec4_t<TestType>(-15.2f, 4.56f, -1.08f, 1.0f);
  CHECK(acl::equals<TestType>(expected, (v * m)));
}

TEMPLATE_TEST_CASE("Validate mul", "[mul]", float, double)
{
  acl::mat4_t<TestType> m = {
    0.0f, 0.80f, 0.60f, 0.0f, -0.80f, -0.36f, 0.48f, 0.0f, -0.60f, 0.48f, -0.64f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
  };
  acl::vec3a_t<TestType> v        = acl::vec3a_t<TestType>(3.000f, 10.000f, 12.000f);
  acl::vec3a_t<TestType> expected = acl::vec3a_t<TestType>(-15.2f, 4.56f, -1.08f);
  CHECK(acl::equals<TestType>(expected, (v * m)));
}

TEST_CASE("Validate ivec4", "[ivec4]")
{
  acl::ivec4 m  = {1, 4, 5, 3};
  auto       c  = m;
  auto       m2 = (m + m);
  CHECK(m == c);
  CHECK(acl::equals(acl::ivec4{2, 8, 10, 6}, m2));
}
