#include <catch2/catch_all.hpp>
#include <cmath>
#include <limits>
#include <acl/math/vml.hpp>

TEST_CASE("Validate vec3::cross", "[vec3::cross]")
{
  acl::vec3_t v1       = acl::vec3::set(4, 6, 1);
  acl::vec3_t v2       = acl::vec3::set(3, 8, 5);
  acl::vec3_t expected = acl::vec3::set(22, -17, 14);
  CHECK(acl::vec3::equals(expected, acl::vec3::cross(v1, v2)));
}

TEST_CASE("Validate vec3a::compare", "[vec3a::compare]")
{
  acl::vec3a_t p    = acl::vec3a::set(-441.3f, 23.0f, -1.0f);
  auto         copy = p;
  acl::vec3a_t q    = acl::vec3a::set(441.3f, 5.0f, 51.0f);
  acl::vec3a_t r    = acl::vec3a::set(445.3f, 15.0f, 151.0f);
  CHECK(p == copy);
  CHECK(acl::vec3a::greater_any(p, q));
  CHECK(acl::vec3a::greater_all(p, q) == false);
  CHECK(acl::vec3a::lesser_any(p, q));
  CHECK(acl::vec3a::lesser_all(p, q) == false);
  CHECK(acl::vec3a::greater_any(q, r) == false);
  CHECK(acl::vec3a::greater_all(q, r) == false);
  CHECK(acl::vec3a::lesser_any(q, r));
  CHECK(acl::vec3a::lesser_all(q, r));
  CHECK(acl::vec3a::greater_any(r, q));
  CHECK(acl::vec3a::greater_all(r, q));
  CHECK(acl::vec3a::lesser_any(r, q) == false);
  CHECK(acl::vec3a::lesser_all(r, q) == false);
}

TEST_CASE("Validate vec4::mul", "[vec4::mul]")
{
  acl::mat4_t m = {
    0.0f, 0.80f, 0.60f, 0.0f, -0.80f, -0.36f, 0.48f, 0.0f, -0.60f, 0.48f, -0.64f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
  };
  acl::vec4_t v        = acl::vec4::set(3.000f, 10.000f, 12.000f, 1.0f);
  acl::vec4_t expected = acl::vec4::set(-15.2f, 4.56f, -1.08f, 1.0f);
  CHECK(acl::vec4::equals(expected, acl::vec4::mul(v, m)));
}

TEST_CASE("Validate vec3a::mul", "[vec3a::mul]")
{
  acl::mat4_t m = {
    0.0f, 0.80f, 0.60f, 0.0f, -0.80f, -0.36f, 0.48f, 0.0f, -0.60f, 0.48f, -0.64f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
  };
  acl::vec3a_t v        = acl::vec3a::set(3.000f, 10.000f, 12.000f);
  acl::vec3a_t expected = acl::vec3a::set(-15.2f, 4.56f, -1.08f);
  CHECK(acl::vec3a::equals(expected, acl::vec3a::mul(v, m)));
}

TEST_CASE("Validate ivec4", "[ivec4]")
{
  acl::ivec4_t m  = {1, 4, 5, 3};
  auto         c  = m;
  auto         m2 = acl::ivec4::add(m, m);
  CHECK(m == c);
  CHECK(acl::ivec4::equals(acl::ivec4_t{2, 8, 10, 6}, m2));
}
