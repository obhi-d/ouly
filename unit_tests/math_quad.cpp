#include <catch2/catch_all.hpp>
#include <cmath>
#include <limits>
#include <acl/math/vml.hpp>

TEST_CASE("Validate quad::isnan", "[quad::isnan]")
{
  float       val[4] = {std::numeric_limits<float>::quiet_NaN(), 1.0f, -1.0f, 10.0f};
  acl::quad_t p      = acl::quad::set_unaligned(val);
  CHECK(acl::quad::isnan(p) == true);
  auto res = acl::quad::isnanv(p);
  CHECK(acl::quad::x(res) != 0.0f);
}

TEST_CASE("Validate quad::isinf", "[quad::isinf]")
{
  acl::quad_t p = acl::quad::set(std::numeric_limits<float>::infinity(), 1.0f, -1.0f, 10.0f);
  CHECK(acl::quad::isinf(p) == true);
  CHECK(acl::quad::x(acl::quad::isinfv(p)) != 0.0f);
  p = acl::quad::set(1.0f, -std::numeric_limits<float>::infinity(), -1.0f, 10.0f);
  CHECK(acl::quad::isinf(p) == true);
  CHECK(acl::quad::x(acl::quad::isinfv(p)) == 0.0f);
}

TEST_CASE("Validate quad::isgreater_x", "[quad::isgreater_x]")
{
  acl::quad_t p = acl::quad::set(-441.3f, 1.0f, -1.0f, 10.0f);
  acl::quad_t q = acl::quad::set(reinterpret_cast<float const*>(&p));
  CHECK(acl::quad::isgreater_x(p, q) == false);
  q = acl::quad::set(441.3f, 1.0f, -1.0f, 10.0f);
  CHECK(acl::quad::isgreater_x(p, q) == false);
  q = acl::quad::set(-1441.3f, 1.0f, -1.0f, 10.0f);
  CHECK(acl::quad::isgreater_x(p, q) == true);
}

TEST_CASE("Validate quad::set", "[quad::set]")
{
  acl::quad_t p = acl::quad::set_x(41.3f);
  CHECK(acl::quad::get(p, 0) == Catch::Approx(41.3f));
  CHECK(acl::quad::get(p, 1) == Catch::Approx(0.0f));
  p = acl::quad::set_x(p, 141.3f);
  CHECK(acl::quad::get(p, 0) == Catch::Approx(141.3f));
  CHECK(acl::quad::get(p, 1) == Catch::Approx(0.0f));
  p = acl::quad::set_y(p, 41.3f);
  CHECK(acl::quad::get(p, 0) == Catch::Approx(141.3f));
  CHECK(acl::quad::get(p, 1) == Catch::Approx(41.3f));
  p = acl::quad::set_z(p, 41.3f);
  CHECK(acl::quad::get(p, 0) == Catch::Approx(141.3f));
  CHECK(acl::quad::get(p, 1) == Catch::Approx(41.3f));
  CHECK(acl::quad::get(p, 2) == Catch::Approx(41.3f));
  p = acl::quad::set_w(p, 41.3f);
  CHECK(acl::quad::get(p, 0) == Catch::Approx(141.3f));
  CHECK(acl::quad::get(p, 1) == Catch::Approx(41.3f));
  CHECK(acl::quad::get(p, 2) == Catch::Approx(41.3f));
  CHECK(acl::quad::get(p, 3) == Catch::Approx(41.3f));
  acl::quad_t q = acl::quad::set_x(31.3f);
  acl::quad_t r = acl::quad::set_x(p, q);
  CHECK(acl::quad::get(r, 0) == Catch::Approx(31.3f));
  r = acl::quad::set_y(r, q);
  CHECK(acl::quad::get(r, 0) == Catch::Approx(31.3f));
  CHECK(acl::quad::get(r, 1) == Catch::Approx(31.3f));
  r = acl::quad::set_z(r, q);
  CHECK(acl::quad::get(r, 0) == Catch::Approx(31.3f));
  CHECK(acl::quad::get(r, 1) == Catch::Approx(31.3f));
  CHECK(acl::quad::get(r, 2) == Catch::Approx(31.3f));
  r = acl::quad::set_w(r, q);
  CHECK(acl::quad::get(r, 0) == Catch::Approx(31.3f));
  CHECK(acl::quad::get(r, 1) == Catch::Approx(31.3f));
  CHECK(acl::quad::get(r, 2) == Catch::Approx(31.3f));
  CHECK(acl::quad::get(r, 3) == Catch::Approx(31.3f));
}

TEST_CASE("Validate quad::compare", "[quad::compare]")
{
  acl::quad_t p = acl::quad::set(-441.3f, 23.0f, -1.0f, 10.0f);
  acl::quad_t q = acl::quad::set(441.3f, 5.0f, 51.0f, 10.0f);
  acl::quad_t r = acl::quad::set(445.3f, 15.0f, 151.0f, 110.0f);
  CHECK(acl::quad::greater_any(p, q));
  CHECK(acl::quad::greater_all(p, q) == false);
  CHECK(acl::quad::lesser_any(p, q));
  CHECK(acl::quad::lesser_all(p, q) == false);
  CHECK(acl::quad::greater_any(q, r) == false);
  CHECK(acl::quad::greater_all(q, r) == false);
  CHECK(acl::quad::lesser_any(q, r));
  CHECK(acl::quad::lesser_all(q, r));
  CHECK(acl::quad::greater_any(r, q));
  CHECK(acl::quad::greater_all(r, q));
  CHECK(acl::quad::lesser_any(r, q) == false);
  CHECK(acl::quad::lesser_all(r, q) == false);
}

TEST_CASE("Validate quad::arithmetic", "[quad::arithmetic]")
{
  acl::quad_t p = acl::quad::set(10.0f, 23.0f, -1.0f, 10.0f);
  acl::quad_t q = acl::quad::set(441.3f, 5.0f, 51.0f, 10.0f);
  acl::quad_t r = acl::quad::mul_x(p, q);
  CHECK(acl::quad::x(r) == Catch::Approx(4413.0f));
  r = acl::quad::recip_sqrt_x(r);
  CHECK(acl::real::equals(acl::quad::x(r), (1 / acl::sqrt(4413.0f))));
  p = acl::quad::set(110.0f, 223.0f, 11.0f, 10.0f);
  q = acl::quad::set(10.0f);
  r = acl::quad::div(p, q);
  CHECK(acl::quad::x(r) == Catch::Approx(11.0f));
  CHECK(acl::quad::y(r) == Catch::Approx(22.3f));
  CHECK(acl::quad::z(r) == Catch::Approx(1.1f));
  CHECK(acl::quad::w(r) == Catch::Approx(1.0f));
  r = acl::quad::madd(p, q, q);
  CHECK(acl::quad::x(r) == Catch::Approx(1110.0f));
  CHECK(acl::quad::y(r) == Catch::Approx(2240.0f));
  CHECK(acl::quad::z(r) == Catch::Approx(120.0f));
  CHECK(acl::quad::w(r) == Catch::Approx(110.0f));
  r = acl::quad::vhadd(p);
  CHECK(acl::real::equals(acl::quad::x(r), acl::quad::hadd(p)));
  q = acl::quad::set(441.3f, 5.0f, 51.0f, 10.0f);
  r = acl::quad::recip_sqrt(q);
  CHECK(acl::real::equals(acl::quad::x(r), (1 / acl::sqrt(441.3f))));
  CHECK(acl::real::equals(acl::quad::y(r), (1 / acl::sqrt(5.0f))));
  CHECK(acl::real::equals(acl::quad::z(r), (1 / acl::sqrt(51.0f))));
  CHECK(acl::real::equals(acl::quad::w(r), (1 / acl::sqrt(10.0f))));
  std::uint32_t select_mask[4] = {0xffffffff, 0, 0xffffffff, 0};
  q                            = acl::quad::set(441.3f, 5.0f, 51.0f, 10.0f);
  p                            = acl::quad::set(10.0f, 23.0f, -1.0f, 20.0f);
  r = acl::quad::select(p, q, acl::quad::set_unaligned(reinterpret_cast<float const*>(select_mask)));
  CHECK(acl::quad::x(r) == Catch::Approx(441.3f));
  CHECK(acl::quad::y(r) == Catch::Approx(23.0f));
  CHECK(acl::quad::z(r) == Catch::Approx(51.0f));
  CHECK(acl::quad::w(r) == Catch::Approx(20.0f));
  q = acl::quad::set(1.3f, 1.2f, 1.6f, 1.8f);
  r = acl::quad::normalize(q);
  CHECK(acl::quad::x(r) == Catch::Approx(0.43503f));
  CHECK(acl::quad::y(r) == Catch::Approx(0.40156f));
  CHECK(acl::quad::z(r) == Catch::Approx(0.53542f));
  CHECK(acl::quad::w(r) == Catch::Approx(0.60234f));
  q = acl::quad::set(10.0f, 12.0f, 5.0f, 8.0f);
  p = acl::quad::set(20.0f, 20.0f, 15.0f, 20.0f);
  r = acl::quad::lerp(p, q, 0.5f);
  CHECK(acl::quad::x(r) == Catch::Approx(15.0f));
  CHECK(acl::quad::y(r) == Catch::Approx(16.0f));
  CHECK(acl::quad::z(r) == Catch::Approx(10.f));
  CHECK(acl::quad::w(r) == Catch::Approx(14.f));
  CHECK(acl::quad::distance(p, q) == Catch::Approx(20.19901));
  CHECK(acl::quad::sqdistance(p, q) == Catch::Approx(408.0f));
  q = acl::quad::set(10.0f, 12.0f, 5.0f, 8.0f);
  r = acl::quad::set_000w(q, 0);
  CHECK(acl::quad::x(r) == Catch::Approx(0.0f));
  CHECK(acl::quad::y(r) == Catch::Approx(0.0f));
  CHECK(acl::quad::z(r) == Catch::Approx(0.0f));
  CHECK(acl::quad::w(r) == Catch::Approx(10.0f));
  r = acl::quad::set_000w(q, 1);
  CHECK(acl::quad::x(r) == Catch::Approx(0.0f));
  CHECK(acl::quad::y(r) == Catch::Approx(0.0f));
  CHECK(acl::quad::z(r) == Catch::Approx(0.0f));
  CHECK(acl::quad::w(r) == Catch::Approx(12.0f));
  r = acl::quad::set_000w(q, 2);
  CHECK(acl::quad::x(r) == Catch::Approx(0.0f));
  CHECK(acl::quad::y(r) == Catch::Approx(0.0f));
  CHECK(acl::quad::z(r) == Catch::Approx(0.0f));
  CHECK(acl::quad::w(r) == Catch::Approx(5.0f));
  r = acl::quad::set_000w(q, 3);
  CHECK(acl::quad::x(r) == Catch::Approx(0.0f));
  CHECK(acl::quad::y(r) == Catch::Approx(0.0f));
  CHECK(acl::quad::z(r) == Catch::Approx(0.0f));
  CHECK(acl::quad::w(r) == Catch::Approx(8.0f));
  q = acl::quad::set(10.0f, 12.0f, 5.0f, 8.0f);
  r = acl::quad::set_111w(q, 0);
  CHECK(acl::quad::x(r) == Catch::Approx(1.0f));
  CHECK(acl::quad::y(r) == Catch::Approx(1.0f));
  CHECK(acl::quad::z(r) == Catch::Approx(1.0f));
  CHECK(acl::quad::w(r) == Catch::Approx(10.0f));
  r = acl::quad::set_111w(q, 1);
  CHECK(acl::quad::x(r) == Catch::Approx(1.0f));
  CHECK(acl::quad::y(r) == Catch::Approx(1.0f));
  CHECK(acl::quad::z(r) == Catch::Approx(1.0f));
  CHECK(acl::quad::w(r) == Catch::Approx(12.0f));
  r = acl::quad::set_111w(q, 2);
  CHECK(acl::quad::x(r) == Catch::Approx(1.0f));
  CHECK(acl::quad::y(r) == Catch::Approx(1.0f));
  CHECK(acl::quad::z(r) == Catch::Approx(1.0f));
  CHECK(acl::quad::w(r) == Catch::Approx(5.0f));
  r = acl::quad::set_111w(q, 3);
  CHECK(acl::quad::x(r) == Catch::Approx(1.0f));
  CHECK(acl::quad::y(r) == Catch::Approx(1.0f));
  CHECK(acl::quad::z(r) == Catch::Approx(1.0f));
  CHECK(acl::quad::w(r) == Catch::Approx(8.0f));
}
