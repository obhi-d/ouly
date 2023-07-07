
#include <catch2/catch_all.hpp>
#include <acl/math/vml.hpp>

TEST_CASE("Validate rect", "[rect]")
{
  acl::rect_t r  = acl::rect::set(100.0, 0, 200.0, 400.0);
  auto        m2 = acl::rect::center(r);
  CHECK(acl::vec2::equals(acl::vec2_t{150, 200}, m2));
  m2 = acl::rect::half_size(r);
  CHECK(acl::vec2::equals(acl::vec2_t{50, 200}, m2));
  m2 = acl::rect::size(r);
  CHECK(acl::vec2::equals(acl::vec2_t{100, 400}, m2));
}

TEST_CASE("Validate irect", "[irect]")
{
  acl::irect_t r  = acl::irect::set(100, 0, 200, 400);
  auto         m2 = acl::irect::center(r);
  CHECK(acl::ivec2::equals(acl::ivec2_t{150, 200}, m2));
  m2 = acl::irect::half_size(r);
  CHECK(acl::ivec2::equals(acl::ivec2_t{50, 200}, m2));
  m2 = acl::irect::size(r);
  CHECK(acl::ivec2::equals(acl::ivec2_t{100, 400}, m2));
  CHECK(acl::irect::width(r) == 100);
  CHECK(acl::irect::height(r) == 400);
  CHECK(acl::irect::left(r) == 100);
  CHECK(acl::irect::top(r) == 0);
  CHECK(acl::irect::right(r) == 200);
  CHECK(acl::irect::bottom(r) == 400);
}
