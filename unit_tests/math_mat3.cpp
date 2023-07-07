#include <acl/math/vml.hpp>
#include <catch2/catch_all.hpp>

TEST_CASE("Validate mat3::transpose", "[mat3::transpose]")
{
  acl::mat3_t m =
    acl::mat3::from_quat(acl::quat::from_axis_angle(acl::vec3::set(0, 1.0f, 0.0f), acl::to_radians(10.0f)));
  acl::mat3_t t = acl::mat3::transpose(m);
  for (std::uint32_t row = 0; row < 3; ++row)
    for (std::uint32_t col = 0; col < 3; ++col)
      CHECK(acl::mat3::get(m, row, col) == acl::mat3::get(t, col, row));
}

TEST_CASE("Validate mat3::mul", "[mat3::mul]")
{
  acl::mat4_t m2 = {3.0f, 10.0f, 12.0f, 18.0f, 12.0f, 1.0f, 4.0f, 9.0f, 9.0f, 10.0f, 12.0f, 2.0f, 0, 0, 0, 0};

  acl::mat4_t m2_times_3 = {9.0f,  30.0f, 36.0f, 54.0f, 36.0f, 3.0f, 12.0f, 27.0f,
                            27.0f, 30.0f, 36.0f, 6.0f,  0,     0,    0,     0};

  acl::mat3_t& expected = acl::mat4::as_mat3(m2_times_3);

  CHECK(acl::mat3::equals(acl::mat3::mul(3.0f, acl::mat4::as_mat3(m2)), expected));
  CHECK(acl::mat3::equals(acl::mat3::mul(acl::mat4::as_mat3(m2), 3.0f), expected));
}
