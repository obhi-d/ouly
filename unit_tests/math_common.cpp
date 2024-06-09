
#include <acl/math/vml.hpp>
#include <catch2/catch_all.hpp>

TEMPLATE_TEST_CASE("Validate rect", "[rect]", float, double)
{
  acl::rect r  = acl::rect(100.0f, 0.0f, 200.0f, 400.0f);
  auto      m2 = acl::center(r);
  CHECK(acl::equals(acl::vec2{150, 200}, m2));
  m2 = acl::half_size(r);
  CHECK(acl::equals(acl::vec2{50, 200}, m2));
  m2 = acl::size(r);
  CHECK(acl::equals(acl::vec2{100, 400}, m2));
}

TEMPLATE_TEST_CASE("Validate irect", "[irect]", float, double)
{
  acl::irect r  = acl::irect(100, 0, 200, 400);
  auto       m2 = acl::center(r);
  CHECK(acl::equals(acl::ivec2{150, 200}, m2));
  m2 = acl::half_size(r);
  CHECK(acl::equals(acl::ivec2{50, 200}, m2));
  m2 = acl::size(r);
  CHECK(acl::equals(acl::ivec2{100, 400}, m2));
  CHECK(acl::width(r) == 100);
  CHECK(acl::height(r) == 400);
  CHECK(acl::left(r) == 100);
  CHECK(acl::top(r) == 0);
  CHECK(acl::right(r) == 200);
  CHECK(acl::bottom(r) == 400);
  CHECK(acl::log2_next(1) == 0);
  CHECK(acl::log2_next(251) == 8);
  CHECK(acl::log2_next_positive(251) == 8);
  auto color      = acl::rgba8(133, 21, 43, 244);
  auto linear     = acl::linear_to_gamma(color);
  auto back_color = acl::gamma_to_linear(linear);
  CHECK(acl::rgba8(133, 21, 43, 244) == color);
  CHECK(acl::rgba8(131, 20, 42, 244) == back_color);
}
