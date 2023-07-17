#include <acl/math/vml.hpp>
#include <catch2/catch_all.hpp>
#include <cstring>

TEMPLATE_TEST_CASE("Validate intersect::bounding_volume_frustum_coherent",
                   "[intersect::bounding_volume_frustum_coherent]", float, double)
{
  // define a custom prism
  acl::mat4_t<TestType>    m = acl::make_orthographic_projection<TestType>(-50.0f, 50.0f, -45.0f, 45.0f, 1.0f, 1000.0f);
  acl::frustum_t<TestType> frustum_orig = acl::make_frustum(acl::transpose(m));
  auto                     planes       = acl::get_planes(frustum_orig);
  std::array<acl::plane_t<TestType>, 8> custom_planes;
  std::memcpy(custom_planes.data(), planes.data(), planes.size() * sizeof(acl::plane_t<TestType>));

  custom_planes[6] = acl::plane_t<TestType>(frustum_orig[acl::frustum_t<TestType>::k_far], 900.0f);
  custom_planes[7] = acl::plane_t<TestType>(frustum_orig[acl::frustum_t<TestType>::k_near], -10.0f);

  acl::frustum_t<TestType> custom =
    acl::make_frustum((acl::plane_t<TestType>*)nullptr, static_cast<std::uint32_t>(custom_planes.size()));
  acl::frustum_t<TestType> unused = acl::make_frustum(custom_planes.data(), 6);

  acl::frustum_t<TestType> unused_copy(unused);
  CHECK(unused_copy.size() == 6);

  acl::mat4_t<TestType> tm = acl::transpose(m);
  m                        = acl::transpose(m);
  CHECK(acl::equals<TestType>(m, tm));

  unused            = acl::make_frustum(m);
  std::uint32_t idx = 0;
  for (auto& p : custom_planes)
    custom[idx++] = p;

  acl::frustum_t copy(custom);
  copy = custom;
  acl::frustum_t inter(std::move(copy));
  custom = std::move(inter);

  acl::coherency state = acl::default_coherency(6);

  acl::bounding_volume_t<TestType> vol =
    acl::make_bounding_volume(acl::vec3a_t<TestType>(5, 5, 5), acl::vec3a_t<TestType>(2, 2, 2));
  acl::bounding_volume_t<TestType> vol2 =
    acl::make_bounding_volume(acl::vec3a_t<TestType>(25, 1225, 25), acl::vec3a_t<TestType>(2, 2, 2));

  CHECK(acl::test_intersection(vol, frustum_orig, state) == acl::result_t::k_inside);

  state = acl::default_coherency(8);

  CHECK(acl::test_intersection(vol2, custom, state) == acl::result_t::k_outside);
#ifndef NDEBUG
  CHECK(acl::test_intersection(vol2, custom, state) == acl::result_t::k_outside);
  CHECK(state.iterations == 0);
#endif

  vol = acl::make_bounding_volume(acl::vec3a_t<TestType>(5, 5, 5), acl::vec3a_t<TestType>(20, 20, 20));

  CHECK(acl::test_intersection(vol, custom, state) == acl::result_t::k_intersecting);
}

TEMPLATE_TEST_CASE("Validate intersect::bounding_volume_frustum", "[intersect::bounding_volume_frustum]", float, double)
{
  // define a custom prism

  acl::mat4_t<TestType>    m = acl::make_orthographic_projection<TestType>(-50.0f, 50.0f, -45.0f, 45.0f, 1.0f, 1000.0f);
  acl::frustum_t<TestType> frustum_orig = acl::make_frustum(acl::transpose(m));
  auto                     planes       = acl::get_planes(frustum_orig);
  std::array<acl::plane_t<TestType>, 8> custom_planes;
  std::memcpy(custom_planes.data(), planes.data(), planes.size() * sizeof(acl::plane_t<TestType>));

  custom_planes[6] = acl::plane_t<TestType>(frustum_orig[acl::frustum_t<TestType>::k_far], 900.0f);
  custom_planes[7] = acl::plane_t<TestType>(frustum_orig[acl::frustum_t<TestType>::k_near], -10.0f);

  acl::frustum_t<TestType> custom =
    acl::make_frustum((acl::plane_t<TestType>*)nullptr, static_cast<std::uint32_t>(custom_planes.size()));
  acl::frustum_t<TestType> unused = acl::make_frustum(custom_planes.data(), 6);

  acl::bounding_volume_t<TestType> vol =
    acl::make_bounding_volume(acl::vec3a_t<TestType>(5, 5, 5), acl::vec3a_t<TestType>(2, 2, 2));

  CHECK(acl::test_intersection(vol, frustum_orig) == acl::result_t::k_inside);
  CHECK(acl::test_intersection(vol, custom) == acl::result_t::k_outside);

  vol = acl::make_bounding_volume(acl::vec3a_t<TestType>(5, 5, 5), acl::vec3a_t<TestType>(20, 20, 20));

  CHECK(acl::test_intersection(vol, custom) == acl::result_t::k_intersecting);
}

TEMPLATE_TEST_CASE("Validate intersect::bounding_volumes", "[intersect::bounding_volumes]", float, double)
{
  // define a custom prism
  acl::bounding_volume_t<TestType> vol1 =
    acl::make_bounding_volume(acl::vec3a_t<TestType>(5, 5, 5), acl::vec3a_t<TestType>(12, 12, 12));
  acl::bounding_volume_t<TestType> vol2 =
    acl::make_bounding_volume(acl::vec3a_t<TestType>(15, 15, 15), acl::vec3a_t<TestType>(12, 12, 12));
  acl::bounding_volume_t<TestType> vol3 =
    acl::make_bounding_volume(acl::vec3a_t<TestType>(19, 19, 19), acl::vec3a_t<TestType>(1, 1, 1));

  CHECK(acl::test_intersection(vol1, vol2) == acl::result_t::k_intersecting);
  CHECK(acl::test_intersection(vol1, vol3) == acl::result_t::k_outside);
  CHECK(acl::test_intersection(vol2, vol3) == acl::result_t::k_intersecting);
}

TEMPLATE_TEST_CASE("Validate intersect::bounding_sphere_frustum", "[intersect::bounding_sphere_frustum]", float, double)
{
  // define a custom prism
  auto m            = acl::make_orthographic_projection<TestType>(-50.0f, 50.0f, -45.0f, 45.0f, 1.0f, 1000.0f);
  auto frustum_orig = acl::make_frustum<TestType>(acl::transpose(m));
  auto planes       = acl::get_planes(frustum_orig);

  std::array<acl::plane_t<TestType>, 8> custom_planes;
  std::memcpy(custom_planes.data(), planes.data(), planes.size() * sizeof(acl::plane_t<TestType>));
  custom_planes[6] =
    acl::plane_t<TestType>(frustum_orig[acl::frustum_t<TestType>::k_far], static_cast<TestType>(900.0f));
  custom_planes[7] =
    acl::plane_t<TestType>(frustum_orig[acl::frustum_t<TestType>::k_near], static_cast<TestType>(-10.0f));

  acl::frustum_t<TestType> custom =
    acl::make_frustum(custom_planes.data(), static_cast<std::uint32_t>(custom_planes.size()));
  acl::sphere_t<TestType> vol = acl::make_sphere(acl::vec3a_t<TestType>(5, 5, 5), 2);

  CHECK(acl::test_intersection(vol, frustum_orig) == acl::result_t::k_inside);
  CHECK(acl::test_intersection(vol, custom) == acl::result_t::k_outside);

  vol = acl::make_sphere(acl::vec3a_t<TestType>(5, 5, 5), 20);

  CHECK(acl::test_intersection(vol, custom) == acl::result_t::k_intersecting);
}
