#include <acl/math/vml.hpp>
#include <catch2/catch_all.hpp>
#include <cmath>
#include <limits>

TEMPLATE_TEST_CASE("Validate combine", "[combine]", float, double)
{
  acl::transform_t<TestType> t = acl::transform_t<TestType>();
  acl::quat_t<TestType>      rot =
    acl::make_quaternion(acl::vec3a_t<TestType>(0, 1.0f, 0), acl::to_radians<TestType>(20.0f));

  acl::set_translation(t, acl::vec3a_t<TestType>(10.0f));
  acl::set_scale(t, 2.5f);
  acl::set_rotation(t, rot);

  acl::mat4_t<TestType> scale     = acl::make_mat4_from_scale(acl::vec3a_t<TestType>(2.5f));
  acl::mat4_t<TestType> rotate    = acl::make_mat4_form_quaternion(rot);
  acl::mat4_t<TestType> translate = acl::make_mat4_from_translation(acl::vec3a_t<TestType>(10.0f));
  acl::mat4_t<TestType> expected  = scale * rotate * translate;
  acl::mat4_t<TestType> result    = acl::make_mat4(t);

  CHECK(acl::equals<TestType>(result, expected));

  acl::transform_t<TestType> id;

  t = id * t;

  result = acl::make_mat4(t);

  CHECK(acl::equals<TestType>(result, expected));

  t = t * id;

  CHECK(acl::equals<TestType>(result, expected));

  acl::transform_t<TestType> t2;

  acl::quat_t<TestType> rot2 =
    acl::make_quaternion(acl::vec3a_t<TestType>(1.0f, 0.0f, 0), acl::to_radians<TestType>(120.0f));

  acl::set_translation(t2, acl::vec3a_t<TestType>(10.0f));
  acl::set_scale(t2, 0.5f);
  acl::set_rotation(t2, rot2);

  acl::transform_t<TestType> combined = t2 * t;

  acl::mat4_t<TestType> t2m = acl::make_mat4(t2);
  auto                  tm  = acl::make_mat4(t);
  auto                  exp = tm * t2m;
  auto                  res = acl::make_mat4(combined);

  CHECK(acl::equals<TestType>(res, exp));

  auto ct = acl::make_transform(tm);

  CHECK(acl::equals<TestType>(ct, t));

  acl::vec3a_t<TestType> point = acl::vec3a_t<TestType>(15.0f, 442.04f, 23.0f);
  CHECK(acl::equals<TestType>(point * res, point * combined));
}