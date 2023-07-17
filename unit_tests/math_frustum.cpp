#include <acl/math/vml.hpp>
#include <catch2/catch_all.hpp>

TEMPLATE_TEST_CASE("Validate frustum_t<TestType>::set", "[frustum_t<TestType>::set]", float, double)
{
  acl::mat4_t<TestType>    m            = acl::make_orthographic_projection<TestType>(100.0f, 90.0f, 1.0f, 1000.0f);
  acl::frustum_t<TestType> frustum_orig = acl::make_frustum(acl::transpose(m));
  acl::frustum_t<TestType> frustum_other;
  frustum_other                  = frustum_orig;
  acl::frustum_t<TestType> frust = std::move(frustum_other);

  CHECK(acl::dot(frust[acl::frustum_t<TestType>::k_near], acl::vec3a_t<TestType>(0.0f, 0.0f, 0.0f)) ==
        Catch::Approx(-1.0f));
  CHECK(acl::dot(frust[acl::frustum_t<TestType>::k_far], acl::vec3a_t<TestType>(0.0f, 0.0f, 0.0f)) ==
        Catch::Approx(1000.0f));
  CHECK(acl::dot(frust[acl::frustum_t<TestType>::k_left], acl::vec3a_t<TestType>(0.0f, 0.0f, 0.0f)) ==
        Catch::Approx(50.0f));
  CHECK(acl::dot(frust[acl::frustum_t<TestType>::k_right], acl::vec3a_t<TestType>(0.0f, 0.0f, 0.0f)) ==
        Catch::Approx(50.0f));
  CHECK(acl::dot(frust[acl::frustum_t<TestType>::k_top], acl::vec3a_t<TestType>(0.0f, 0.0f, 0.0f)) ==
        Catch::Approx(45.0f));
  CHECK(acl::dot(frust[acl::frustum_t<TestType>::k_bottom], acl::vec3a_t<TestType>(0.0f, 0.0f, 0.0f)) ==
        Catch::Approx(45.0f));
}
