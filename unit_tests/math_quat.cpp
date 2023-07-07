#include <catch2/catch_all.hpp>
#include <cmath>
#include <limits>
#include <acl/math/vml.hpp>

TEST_CASE("Validate quat::mul", "[quat::mul]")
{

  {
    acl::quat_t p = acl::quat::from_axis_angle(acl::vec3::set(0, 1.0f, 0), acl::to_radians(20.0f));
    acl::mat4_t m = acl::mat4::from_quat(p);
    acl::mat4_t t = acl::mat4::from_translation(acl::vec3a::set(10.1f, 42.f, 0));
    m             = acl::mat4::mul(m, t);

    CHECK(acl::quat::equals(p, acl::quat::from_mat4(m)));
    CHECK(acl::quat::equals(p, acl::quat::from_mat3(acl::mat4::as_mat3(m))));
    CHECK(acl::quat::equals(p, acl::quat::mul(p, acl::quat::identity())));
    CHECK(acl::quat::equals(p, acl::quat::mul(acl::quat::identity(), p)));
    CHECK(acl::quat::equals(acl::quat::identity(), acl::quat::mul(acl::quat::conjugate(p), p)));
    CHECK(acl::quat::equals(acl::quat::identity(), acl::quat::mul(p, acl::quat::conjugate(p))));
    CHECK(acl::quat::equals(acl::quat::identity(), acl::quat::mul(acl::quat::inverse(p), p)));
    CHECK(acl::quat::equals(acl::quat::identity(), acl::quat::mul(p, acl::quat::inverse(p))));
  }

  {
    acl::quat_t p = acl::quat::from_axis_angle(acl::vec3::set(0, 1.0f, 0), acl::to_radians(180.0f));
    acl::mat4_t m = acl::mat4::from_quat(p);
    acl::mat4_t t = acl::mat4::from_translation(acl::vec3a::set(10.1f, 42.f, 0));
    m             = acl::mat4::mul(m, t);

    CHECK(acl::quat::equals(p, acl::quat::from_mat4(m)));
    CHECK(acl::quat::equals(p, acl::quat::from_mat3(acl::mat4::as_mat3(m))));
    CHECK(acl::quat::equals(p, acl::quat::mul(p, acl::quat::identity())));
    CHECK(acl::quat::equals(p, acl::quat::mul(acl::quat::identity(), p)));
    CHECK(acl::quat::equals(acl::quat::identity(), acl::quat::mul(acl::quat::conjugate(p), p)));
    CHECK(acl::quat::equals(acl::quat::identity(), acl::quat::mul(p, acl::quat::conjugate(p))));
    CHECK(acl::quat::equals(acl::quat::identity(), acl::quat::mul(acl::quat::inverse(p), p)));
    CHECK(acl::quat::equals(acl::quat::identity(), acl::quat::mul(p, acl::quat::inverse(p))));
  }

  {
    acl::quat_t p = acl::quat::from_axis_angle(acl::vec3::set(1.0, 0.0f, 0), acl::to_radians(180.0f));
    acl::mat4_t m = acl::mat4::from_quat(p);
    acl::mat4_t t = acl::mat4::from_translation(acl::vec3a::set(10.1f, 42.f, 0));
    m             = acl::mat4::mul(m, t);

    CHECK(acl::quat::equals(p, acl::quat::from_mat4(m)));
    CHECK(acl::quat::equals(p, acl::quat::from_mat3(acl::mat4::as_mat3(m))));
    CHECK(acl::quat::equals(p, acl::quat::mul(p, acl::quat::identity())));
    CHECK(acl::quat::equals(p, acl::quat::mul(acl::quat::identity(), p)));
    CHECK(acl::quat::equals(acl::quat::identity(), acl::quat::mul(acl::quat::conjugate(p), p)));
    CHECK(acl::quat::equals(acl::quat::identity(), acl::quat::mul(p, acl::quat::conjugate(p))));
    CHECK(acl::quat::equals(acl::quat::identity(), acl::quat::mul(acl::quat::inverse(p), p)));
    CHECK(acl::quat::equals(acl::quat::identity(), acl::quat::mul(p, acl::quat::inverse(p))));
  }

  {
    acl::quat_t p = acl::quat::from_axis_angle(acl::vec3::set(0.0, 0.0f, 1.0f), acl::to_radians(180.0f));
    acl::mat4_t m = acl::mat4::from_quat(p);
    acl::mat4_t t = acl::mat4::from_translation(acl::vec3a::set(10.1f, 42.f, 0));
    m             = acl::mat4::mul(m, t);

    CHECK(acl::quat::equals(p, acl::quat::from_mat4(m)));
    CHECK(acl::quat::equals(p, acl::quat::from_mat3(acl::mat4::as_mat3(m))));
    CHECK(acl::quat::equals(p, acl::quat::mul(p, acl::quat::identity())));
    CHECK(acl::quat::equals(p, acl::quat::mul(acl::quat::identity(), p)));
    CHECK(acl::quat::equals(acl::quat::identity(), acl::quat::mul(acl::quat::conjugate(p), p)));
    CHECK(acl::quat::equals(acl::quat::identity(), acl::quat::mul(p, acl::quat::conjugate(p))));
    CHECK(acl::quat::equals(acl::quat::identity(), acl::quat::mul(acl::quat::inverse(p), p)));
    CHECK(acl::quat::equals(acl::quat::identity(), acl::quat::mul(p, acl::quat::inverse(p))));
  }
}

TEST_CASE("Validate quat::slerp", "[quat::slerp]")
{

  acl::quat_t p = acl::quat::from_axis_angle(acl::vec3::set(0, 1.0f, 0), acl::to_radians(20.0f));
  acl::quat_t q = acl::quat::from_axis_angle(acl::vec3::set(0, 1.0f, 0), acl::to_radians(120.0f));
  acl::quat_t r = acl::quat::from_axis_angle(acl::vec3::set(0, 1.0f, 0), acl::to_radians(70.0f));
  CHECK(acl::quat::equals(r, acl::quat::slerp(p, q, 0.5f)));
}
