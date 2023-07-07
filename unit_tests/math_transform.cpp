#include <catch2/catch_all.hpp>
#include <cmath>
#include <limits>
#include <acl/math/vml.hpp>

TEST_CASE("Validate transform::combine", "[transform::combine]")
{
  acl::transform_t t   = acl::transform::identity();
  acl::quat_t      rot = acl::quat::from_axis_angle(acl::vec3::set(0, 1.0f, 0), acl::to_radians(20.0f));

  acl::transform::set_translation(t, acl::vec3a::set(10.0f));
  acl::transform::set_scale(t, 2.5f);
  acl::transform::set_rotation(t, rot);

  acl::mat4_t scale     = acl::mat4::from_scale(acl::vec3a::set(2.5f));
  acl::mat4_t rotate    = acl::mat4::from_rotation(rot);
  acl::mat4_t translate = acl::mat4::from_translation(acl::vec3a::set(10.0f));
  acl::mat4_t expected  = acl::mat4::mul(scale, acl::mat4::mul(rotate, translate));

  acl::mat4_t result;

  acl::transform::matrix(t, result);

  CHECK(acl::mat4::equals(result, expected));

  acl::transform_t id;
  acl::transform::identity(id);

  t = acl::transform::combine(id, t);

  acl::transform::matrix(t, result);

  CHECK(acl::mat4::equals(result, expected));

  t = acl::transform::combine(t, id);

  CHECK(acl::mat4::equals(result, expected));

  acl::transform_t t2 = acl::transform::identity();

  acl::quat_t rot2 = acl::quat::from_axis_angle(acl::vec3::set(1.0f, 0.0f, 0), acl::to_radians(120.0f));

  acl::transform::set_translation(t2, acl::vec3a::set(10.0f));
  acl::transform::set_scale(t2, 0.5f);
  acl::transform::set_rotation(t2, rot2);

  acl::transform_t combined = acl::transform::combine(t2, t);

  acl::mat4_t tm, t2m, exp, res;
  acl::transform::matrix(t2, t2m);
  acl::transform::matrix(t, tm);
  exp = acl::mat4::mul(tm, t2m);

  acl::transform::matrix(combined, res);

  CHECK(acl::mat4::equals(res, exp));

  acl::vec3a_t point = acl::vec3a::set(15.0f, 442.04f, 23.0f);
  CHECK(acl::vec3a::equals(acl::vec3a::mul(point, res), acl::transform::mul(point, combined)));
}