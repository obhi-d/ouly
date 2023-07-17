#include <acl/math/vml.hpp>
#include <catch2/catch_all.hpp>

TEMPLATE_TEST_CASE("Validate mat3_t<TestType>::transpose", "[mat3_t<TestType>::transpose]", float, double)
{
  acl::mat3_t<TestType> m =
    acl::make_mat3(acl::make_quaternion(acl::vec3a_t<TestType>(0, 1.0f, 0.0f), acl::to_radians<TestType>(10.0f)));
  acl::mat3_t<TestType> t = acl::transpose(m);
  for (std::uint32_t row = 0; row < 3; ++row)
    for (std::uint32_t col = 0; col < 3; ++col)
      CHECK(m[row][col] == t[col][row]);
}

TEMPLATE_TEST_CASE("Validate mat3_t<TestType>::mul", "[mat3_t<TestType>::mul]", float, double)
{
  acl::mat4_t<TestType> m2 = {3.0f, 10.0f, 12.0f, 18.0f, 12.0f, 1.0f, 4.0f, 9.0f, 9.0f, 10.0f, 12.0f, 2.0f, 0, 0, 0, 0};
  acl::mat4_t<TestType> m2_times_3 = {9.0f,  30.0f, 36.0f, 54.0f, 36.0f, 3.0f, 12.0f, 27.0f,
                                      27.0f, 30.0f, 36.0f, 6.0f,  0,     0,    0,     0};

  acl::mat3_t<TestType>& expected = acl::as_mat3(m2_times_3);

  CHECK(acl::equals<TestType>(3.0f * acl::as_mat3(m2), expected));
  CHECK(acl::equals<TestType>(acl::as_mat3(m2) * 3.0f, expected));
}
