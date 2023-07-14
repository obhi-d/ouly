#include <acl/math/vml.hpp>
#include <catch2/catch_all.hpp>
#include <cstring>

TEST_CASE("Validate intersect::bounding_volume_frustum_coherent", "[intersect::bounding_volume_frustum_coherent]")
{
  // define a custom prism
  acl::mat4                 m = acl::make_mat4_from_orthographic_projection(-50.0f, 50.0f, -45.0f, 45.0f, 1.0f, 1000.0f);
  acl::frustum              frustum_orig = acl::make_frustum(acl::transpose(m));
  auto                        planes       = acl::get_planes(frustum_orig);
  std::array<acl::plane_t, 8> custom_planes;
  std::memcpy(custom_planes.data(), planes.data(), planes.size() * sizeof(acl::plane_t));
  custom_planes[6] = acl::plane(acl::get_plane(frustum_orig, acl::frustum::k_far), 900.0f);

  custom_planes[7] = acl::plane(acl::frustum::get_plane(frustum_orig, acl::frustum::k_near), -10.0f);

  acl::frustum_t custom = acl::frustum::from_planes(nullptr, static_cast<std::uint32_t>(custom_planes.size()));
  acl::frustum_t unused = acl::frustum::from_planes(custom_planes.data(), 6);

  acl::frustum_t unused_copy(unused);
  CHECK(acl::frustum::count(unused_copy) == 6);

  acl::mat4_t tm = acl::mat4::transpose(m);
  acl::mat4::transpose_in_place(m);
  CHECK(acl::mat4::equals(m, tm));

  acl::frustum::set(unused, m);
  std::uint32_t idx = 0;
  for (auto& p : custom_planes)
    acl::frustum::set_plane(custom, idx++, p);

  acl::frustum_t copy(custom);
  copy = custom;
  acl::frustum_t inter(std::move(copy));
  custom = std::move(inter);

  acl::frustum::coherency state = acl::frustum::default_coherency(6);

  acl::bounding_volume_t vol  = acl::bounding_volume::from_box(acl::vec3a::set(5, 5, 5), acl::vec3a::set(2, 2, 2));
  acl::bounding_volume_t vol2 = acl::bounding_volume::from_box(acl::vec3a::set(25, 1225, 25), acl::vec3a::set(2, 2, 2));

  CHECK(acl::bounding_volume_frustum_coherent(vol, frustum_orig, state) == acl::result_t::k_inside);

  state = acl::frustum::default_coherency(8);

  CHECK(acl::bounding_volume_frustum_coherent(vol2, custom, state) == acl::result_t::k_outside);
#ifndef NDEBUG
  CHECK(acl::bounding_volume_frustum_coherent(vol2, custom, state) == acl::result_t::k_outside);
  CHECK(state.iterations == 0);
#endif

  vol = acl::bounding_volume::from_box(acl::vec3a::set(5, 5, 5), acl::vec3a::set(20, 20, 20));

  CHECK(acl::bounding_volume_frustum_coherent(vol, custom, state) == acl::result_t::k_intersecting);
}

TEST_CASE("Validate intersect::bounding_volume_frustum", "[intersect::bounding_volume_frustum]")
{
  // define a custom prism
  acl::mat4_t                 m = acl::mat4::from_orthographic_projection(-50.0f, 50.0f, -45.0f, 45.0f, 1.0f, 1000.0f);
  acl::frustum_t              frustum_orig = acl::frustum::from_mat4_transpose(acl::mat4::transpose(m));
  auto                        planes       = acl::frustum::get_planes(frustum_orig);
  std::array<acl::plane_t, 8> custom_planes;
  std::memcpy(custom_planes.data(), planes.first, planes.second * sizeof(acl::plane_t));
  custom_planes[6] = acl::plane(acl::frustum::get_plane(frustum_orig, acl::frustum::k_far), 900.0f);
  custom_planes[7] = acl::plane(acl::frustum::get_plane(frustum_orig, acl::frustum::k_near), -10.0f);

  acl::frustum_t custom =
    acl::frustum::from_planes(custom_planes.data(), static_cast<std::uint32_t>(custom_planes.size()));

  acl::bounding_volume_t vol = acl::bounding_volume::from_box(acl::vec3a::set(5, 5, 5), acl::vec3a::set(2, 2, 2));

  CHECK(acl::bounding_volume_frustum(vol, frustum_orig) == acl::result_t::k_inside);
  CHECK(acl::bounding_volume_frustum(vol, custom) == acl::result_t::k_outside);

  vol = acl::bounding_volume::from_box(acl::vec3a::set(5, 5, 5), acl::vec3a::set(20, 20, 20));

  CHECK(acl::bounding_volume_frustum(vol, custom) == acl::result_t::k_intersecting);
}

TEST_CASE("Validate intersect::bounding_volumes", "[intersect::bounding_volumes]")
{
  // define a custom prism
  acl::bounding_volume_t vol1 = acl::bounding_volume::from_box(acl::vec3a::set(5, 5, 5), acl::vec3a::set(12, 12, 12));
  acl::bounding_volume_t vol2 =
    acl::bounding_volume::from_box(acl::vec3a::set(15, 15, 15), acl::vec3a::set(12, 12, 12));
  acl::bounding_volume_t vol3 = acl::bounding_volume::from_box(acl::vec3a::set(19, 19, 19), acl::vec3a::set(1, 1, 1));

  CHECK(acl::bounding_volumes(vol1, vol2) == acl::result_t::k_intersecting);
  CHECK(acl::bounding_volumes(vol1, vol3) == acl::result_t::k_outside);
  CHECK(acl::bounding_volumes(vol2, vol3) == acl::result_t::k_intersecting);
}

TEST_CASE("Validate intersect::bounding_sphere_frustum", "[intersect::bounding_sphere_frustum]")
{
  // define a custom prism
  acl::mat4_t                 m = acl::mat4::from_orthographic_projection(-50.0f, 50.0f, -45.0f, 45.0f, 1.0f, 1000.0f);
  acl::frustum_t              frustum_orig = acl::frustum::from_mat4_transpose(acl::mat4::transpose(m));
  auto                        planes       = acl::frustum::get_planes(frustum_orig);
  std::array<acl::plane_t, 8> custom_planes;
  std::memcpy(custom_planes.data(), planes.first, planes.second * sizeof(acl::plane_t));
  custom_planes[6] = acl::plane(acl::frustum::get_plane(frustum_orig, acl::frustum::k_far), 900.0f);
  custom_planes[7] = acl::plane(acl::frustum::get_plane(frustum_orig, acl::frustum::k_near), -10.0f);

  acl::frustum_t custom =
    acl::frustum::from_planes(custom_planes.data(), static_cast<std::uint32_t>(custom_planes.size()));

  acl::sphere_t vol = acl::sphere::set(acl::vec3a::set(5, 5, 5), 2);

  CHECK(acl::bounding_sphere_frustum(vol, frustum_orig) == acl::result_t::k_inside);
  CHECK(acl::bounding_sphere_frustum(vol, custom) == acl::result_t::k_outside);

  vol = acl::sphere::set(acl::vec3a::set(5, 5, 5), 20);

  CHECK(acl::bounding_sphere_frustum(vol, custom) == acl::result_t::k_intersecting);
}
