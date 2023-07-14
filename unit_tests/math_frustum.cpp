#include <acl/math/vml.hpp>
#include <catch2/catch_all.hpp>

TEST_CASE("Validate frustum::set", "[frustum::set]")
{
  acl::mat4    m            = acl::make_orthographic_projection(100.0f, 90.0f, 1.0f, 1000.0f);
  acl::frustum frustum_orig = acl::make_frustum(acl::transpose(m));
  acl::frustum frustum_other;
  frustum_other          = frustum_orig;
  acl::frustum frustum = std::move(frustum_other);

  CHECK(acl::dot(acl::get_plane(frustum, acl::frustum::plane_type::k_near),
                        acl::vec3a::set(0.0f, 0.0f, 0.0f)) == Catch::Approx(-1.0f));
  CHECK(acl::dot(acl::get_plane(frustum, acl::frustum::plane_type::k_far),
                        acl::vec3a::set(0.0f, 0.0f, 0.0f)) == Catch::Approx(1000.0f));
  CHECK(acl::dot(acl::get_plane(frustum, acl::frustum::plane_type::k_left),
                        acl::vec3a::set(0.0f, 0.0f, 0.0f)) == Catch::Approx(50.0f));
  CHECK(acl::dot(acl::get_plane(frustum, acl::frustum::plane_type::k_right),
                        acl::vec3a::set(0.0f, 0.0f, 0.0f)) == Catch::Approx(50.0f));
  CHECK(acl::dot(acl::get_plane(frustum, acl::frustum::plane_type::k_top),
                        acl::vec3a::set(0.0f, 0.0f, 0.0f)) == Catch::Approx(45.0f));
  CHECK(acl::dot(acl::get_plane(frustum, acl::frustum::plane_type::k_bottom),
                        acl::vec3a::set(0.0f, 0.0f, 0.0f)) == Catch::Approx(45.0f));
}
