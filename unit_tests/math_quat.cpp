#include <acl/math/vml.hpp>
#include <catch2/catch_all.hpp>
#include <cmath>
#include <limits>

TEMPLATE_TEST_CASE("Validate mul", "[mul]", float, double)
{

  {
    acl::quat_t<TestType> p =
      acl::make_quaternion(acl::vec3a_t<TestType>(0, 1.0f, 0), acl::to_radians<TestType>(20.0f));
    acl::mat4_t<TestType> m = acl::make_mat4_form_quaternion(p);
    acl::mat4_t<TestType> t = acl::make_mat4_from_translation(acl::make_vec3a<TestType>(10.1f, 42.f, 0.f));
    m                       = m * t;

    CHECK(acl::equals<TestType>(p, acl::make_quaternion(m)));
    CHECK(acl::equals<TestType>(p, acl::make_quaternion(acl::as_mat3(m))));
    CHECK(acl::equals<TestType>(p, p * acl::quat_t<TestType>()));
    CHECK(acl::equals<TestType>(p, acl::quat_t<TestType>() * p));
    CHECK(acl::equals<TestType>(acl::quat_t<TestType>(), acl::conjugate(p) * p));
    CHECK(acl::equals<TestType>(acl::quat_t<TestType>(), p * acl::conjugate(p)));
    CHECK(acl::equals<TestType>(acl::quat_t<TestType>(), acl::inverse(p) * p));
    CHECK(acl::equals<TestType>(acl::quat_t<TestType>(), p * acl::inverse(p)));
  }

  {
    acl::quat_t<TestType> p =
      acl::make_quaternion(acl::vec3a_t<TestType>(0, 1.0f, 0), acl::to_radians<TestType>(180.0f));
    acl::mat4_t<TestType> m = acl::make_mat4_form_quaternion(p);
    acl::mat4_t<TestType> t = acl::make_mat4_from_translation(acl::make_vec3a<TestType>(10.1f, 42.f, 0.0f));
    m                       = m * t;

    CHECK(acl::equals<TestType>(p, acl::make_quaternion(m)));
    CHECK(acl::equals<TestType>(p, acl::make_quaternion(acl::as_mat3(m))));
    CHECK(acl::equals<TestType>(p, p * acl::quat_t<TestType>()));
    CHECK(acl::equals<TestType>(p, acl::quat_t<TestType>() * p));
    CHECK(acl::equals<TestType>(acl::quat_t<TestType>(), (acl::conjugate(p) * p)));
    CHECK(acl::equals<TestType>(acl::quat_t<TestType>(), (p * acl::conjugate(p))));
    CHECK(acl::equals<TestType>(acl::quat_t<TestType>(), (acl::inverse(p) * p)));
    CHECK(acl::equals<TestType>(acl::quat_t<TestType>(), (p * acl::inverse(p))));
  }

  {
    acl::quat_t<TestType> p =
      acl::make_quaternion(acl::vec3a_t<TestType>(1.0, 0.0f, 0), acl::to_radians<TestType>(180.0f));
    acl::mat4_t<TestType> m = acl::make_mat4_form_quaternion(p);
    acl::mat4_t<TestType> t = acl::make_mat4_from_translation(acl::vec3a_t<TestType>(10.1f, 42.f, 0));
    m                       = m * t;

    CHECK(acl::equals<TestType>(p, acl::make_quaternion(m)));
    CHECK(acl::equals<TestType>(p, acl::make_quaternion(acl::as_mat3(m))));
    CHECK(acl::equals<TestType>(p, p * acl::quat_t<TestType>()));
    CHECK(acl::equals<TestType>(p, acl::quat_t<TestType>() * p));
    CHECK(acl::equals<TestType>(acl::quat_t<TestType>(), (acl::conjugate(p) * p)));
    CHECK(acl::equals<TestType>(acl::quat_t<TestType>(), (p * acl::conjugate(p))));
    CHECK(acl::equals<TestType>(acl::quat_t<TestType>(), (acl::inverse(p) * p)));
    CHECK(acl::equals<TestType>(acl::quat_t<TestType>(), (p * acl::inverse(p))));
  }

  {
    acl::quat_t<TestType> p =
      acl::make_quaternion(acl::vec3a_t<TestType>(0.0, 0.0f, 1.0f), acl::to_radians<TestType>(180.0f));
    acl::mat4_t<TestType> m = acl::make_mat4_form_quaternion(p);
    acl::mat4_t<TestType> t = acl::make_mat4_from_translation(acl::vec3a_t<TestType>(10.1f, 42.f, 0));
    m                       = (m * t);

    CHECK(acl::equals<TestType>(p, acl::make_quaternion(m)));
    CHECK(acl::equals<TestType>(p, acl::make_quaternion(acl::as_mat3(m))));
    CHECK(acl::equals<TestType>(p, (p * acl::quat_t<TestType>())));
    CHECK(acl::equals<TestType>(p, (acl::quat_t<TestType>() * p)));
    CHECK(acl::equals<TestType>(acl::quat_t<TestType>(), (acl::conjugate(p) * p)));
    CHECK(acl::equals<TestType>(acl::quat_t<TestType>(), (p * acl::conjugate(p))));
    CHECK(acl::equals<TestType>(acl::quat_t<TestType>(), (acl::inverse(p) * p)));
    CHECK(acl::equals<TestType>(acl::quat_t<TestType>(), (p * acl::inverse(p))));
  }
}

TEMPLATE_TEST_CASE("Validate slerp", "[slerp]", float, double)
{
  acl::quat_t<TestType> p = acl::make_quaternion(acl::vec3a_t<TestType>(0, 1.0f, 0), acl::to_radians<TestType>(20.0f));
  acl::quat_t<TestType> q = acl::make_quaternion(acl::vec3a_t<TestType>(0, 1.0f, 0), acl::to_radians<TestType>(120.0f));
  acl::quat_t<TestType> r = acl::make_quaternion(acl::vec3a_t<TestType>(0, 1.0f, 0), acl::to_radians<TestType>(70.0f));
  CHECK(acl::equals<TestType>(r, acl::slerp(p, q, 0.5f)));
}
