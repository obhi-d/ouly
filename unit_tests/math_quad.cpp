#include <acl/math/vml.hpp>
#include <catch2/catch_all.hpp>
#include <cmath>
#include <limits>

TEMPLATE_TEST_CASE("Vec4: Validate isnan", "[isnan]", float, double)
{
  TestType              val[4] = {std::numeric_limits<TestType>::quiet_NaN(), 1.0f, -1.0f, 10.0f};
  acl::vec4_t<TestType> p      = acl::vec4_t<TestType>(val);
  CHECK(acl::isnan(p) == true);
  auto res = acl::isnanv(p);
  CHECK(acl::get_x(res) != 0.0f);
}

TEMPLATE_TEST_CASE("Vec4: Validate isinf", "[isinf]", float, double)
{
  acl::vec4_t<TestType> p = {std::numeric_limits<float>::infinity(), 1.0f, -1.0f, 10.0f};
  CHECK(acl::isinf(p) == true);
  CHECK(acl::get_x(acl::isinfv(p)) != 0.0f);
  p = {1.0f, -std::numeric_limits<float>::infinity(), -1.0f, 10.0f};
  CHECK(acl::isinf(p) == true);
  CHECK(acl::get_x(acl::isinfv(p)) == 0.0f);
}

TEMPLATE_TEST_CASE("Vec4: Validate isgreater_x", "[isgreater_x]", float, double)
{
  acl::vec4_t<TestType> p = acl::vec4_t<TestType>(-441.3f, 1.0f, -1.0f, 10.0f);
  acl::vec4_t<TestType> q = acl::vec4_t<TestType>(reinterpret_cast<TestType const*>(&p));
  CHECK(acl::vml::isgreater_x(p.v, q.v) == false);
  q = acl::vec4_t<TestType>(441.3f, 1.0f, -1.0f, 10.0f);
  CHECK(acl::vml::isgreater_x(p.v, q.v) == false);
  q = acl::vec4_t<TestType>(-1441.3f, 1.0f, -1.0f, 10.0f);
  CHECK(acl::vml::isgreater_x(p.v, q.v) == true);
}

TEMPLATE_TEST_CASE("Vec4: Validate set", "[set]", float, double)
{
  acl::vec4_t<TestType> p = acl::vec4_t<TestType>(41.3f, 0, 0, 0);
  CHECK(p[0] == Catch::Approx(41.3f));
  CHECK(p[1] == Catch::Approx(0.0f));
  p = acl::set_x(p, 141.3f);
  CHECK(p[0] == Catch::Approx(141.3f));
  CHECK(p[1] == Catch::Approx(0.0f));
  p = acl::set_y(p, 41.3f);
  CHECK(p[0] == Catch::Approx(141.3f));
  CHECK(p[1] == Catch::Approx(41.3f));
  p = acl::set_z(p, 41.3f);
  CHECK(p[0] == Catch::Approx(141.3f));
  CHECK(p[1] == Catch::Approx(41.3f));
  CHECK(p[2] == Catch::Approx(41.3f));
  p = acl::set_w(p, 41.3f);
  CHECK(p[0] == Catch::Approx(141.3f));
  CHECK(p[1] == Catch::Approx(41.3f));
  CHECK(p[2] == Catch::Approx(41.3f));
  CHECK(p[3] == Catch::Approx(41.3f));
  acl::vec4_t<TestType> q = acl::vml::set_x<TestType>(31.3f);
  acl::vec4_t<TestType> r = acl::vml::set_x(p.v, q.v);
  CHECK(r[0] == Catch::Approx(31.3f));
  r = acl::vml::set_y(r.v, q.v);
  CHECK(r[0] == Catch::Approx(31.3f));
  CHECK(r[1] == Catch::Approx(31.3f));
  r = acl::vml::set_z(r.v, q.v);
  CHECK(r[0] == Catch::Approx(31.3f));
  CHECK(r[1] == Catch::Approx(31.3f));
  CHECK(r[2] == Catch::Approx(31.3f));
  r = acl::vml::set_w(r.v, q.v);
  CHECK(r[0] == Catch::Approx(31.3f));
  CHECK(r[1] == Catch::Approx(31.3f));
  CHECK(r[2] == Catch::Approx(31.3f));
  CHECK(r[3] == Catch::Approx(31.3f));
}

TEMPLATE_TEST_CASE("Vec4: Validate compare", "[compare]", float, double)
{
  acl::vec4_t<TestType> p = acl::vec4_t<TestType>(-441.3f, 23.0f, -1.0f, 10.0f);
  acl::vec4_t<TestType> q = acl::vec4_t<TestType>(441.3f, 5.0f, 51.0f, 10.0f);
  acl::vec4_t<TestType> r = acl::vec4_t<TestType>(445.3f, 15.0f, 151.0f, 110.0f);
  CHECK(acl::greater_any(p, q));
  CHECK(acl::greater_all(p, q) == false);
  CHECK(acl::lesser_any(p, q));
  CHECK(acl::lesser_all(p, q) == false);
  CHECK(acl::greater_any(q, r) == false);
  CHECK(acl::greater_all(q, r) == false);
  CHECK(acl::lesser_any(q, r));
  CHECK(acl::lesser_all(q, r));
  CHECK(acl::greater_any(r, q));
  CHECK(acl::greater_all(r, q));
  CHECK(acl::lesser_any(r, q) == false);
  CHECK(acl::lesser_all(r, q) == false);
}

TEMPLATE_TEST_CASE("Vec4: Validate arithmetic", "[arithmetic]", float, double)
{
  acl::vec4_t<TestType> p = acl::vec4_t<TestType>(10.0f, 23.0f, -1.0f, 10.0f);
  acl::vec4_t<TestType> q = acl::vec4_t<TestType>(441.3f, 5.0f, 51.0f, 10.0f);
  acl::vec4_t<TestType> r = acl::vml::mul_x(p.v, q.v);
  CHECK(acl::get_x(r) == Catch::Approx(4413.0f));
  r = acl::vml::recip_sqrt_x(r.v);
  CHECK(acl::equals<TestType>(acl::get_x(r), (1 / std::sqrt(4413.0f))));
  p = acl::vec4_t<TestType>(110.0f, 223.0f, 11.0f, 10.0f);
  q = acl::vec4_t<TestType>(10.0f);
  r = p / q;
  CHECK(acl::get_x(r) == Catch::Approx(11.0f));
  CHECK(acl::get_y(r) == Catch::Approx(22.3f));
  CHECK(acl::get_z(r) == Catch::Approx(1.1f));
  CHECK(acl::get_w(r) == Catch::Approx(1.0f));
  r = acl::madd(p, q, q);
  CHECK(acl::get_x(r) == Catch::Approx(1110.0f));
  CHECK(acl::get_y(r) == Catch::Approx(2240.0f));
  CHECK(acl::get_z(r) == Catch::Approx(120.0f));
  CHECK(acl::get_w(r) == Catch::Approx(110.0f));
  r = acl::vml::vhadd(p.v);
  CHECK(acl::equals<TestType>(acl::get_x(r), acl::vml::hadd(p.v)));
  q = acl::vec4_t<TestType>(441.3f, 5.0f, 51.0f, 10.0f);
  r = acl::vml::recip_sqrt(q.v);
  CHECK(acl::equals<TestType>(acl::get_x(r), (1 / std::sqrt(441.3f))));
  CHECK(acl::equals<TestType>(acl::get_y(r), (1 / std::sqrt(5.0f))));
  CHECK(acl::equals<TestType>(acl::get_z(r), (1 / std::sqrt(51.0f))));
  CHECK(acl::equals<TestType>(acl::get_w(r), (1 / std::sqrt(10.0f))));
  using int_type                    = std::conditional_t<sizeof(TestType) == 4, uint32_t, uint64_t>;
  constexpr int_type mask           = std::numeric_limits<int_type>::max();
  constexpr int_type zmask          = 0;
  TestType           select_mask[4] = {acl::uint_to_float(mask), acl::uint_to_float(zmask), acl::uint_to_float(mask),
                                       acl::uint_to_float(zmask)};
  q                                 = acl::vec4_t<TestType>(441.3f, 5.0f, 51.0f, 10.0f);
  p                                 = acl::vec4_t<TestType>(10.0f, 23.0f, -1.0f, 20.0f);
  r                                 = acl::vml::select(p.v, q.v, acl::vml::set_unaligned(select_mask));
  CHECK(acl::get_x(r) == Catch::Approx(441.3f));
  CHECK(acl::get_y(r) == Catch::Approx(23.0f));
  CHECK(acl::get_z(r) == Catch::Approx(51.0f));
  CHECK(acl::get_w(r) == Catch::Approx(20.0f));
  q = acl::vec4_t<TestType>(1.3f, 1.2f, 1.6f, 1.8f);
  r = acl::normalize(q);
  CHECK(acl::get_x(r) == Catch::Approx(0.43503f));
  CHECK(acl::get_y(r) == Catch::Approx(0.40156f));
  CHECK(acl::get_z(r) == Catch::Approx(0.53542f));
  CHECK(acl::get_w(r) == Catch::Approx(0.60234f));
  q = acl::vec4_t<TestType>(10.0f, 12.0f, 5.0f, 8.0f);
  p = acl::vec4_t<TestType>(20.0f, 20.0f, 15.0f, 20.0f);
  r = acl::lerp(p, q, 0.5f);
  CHECK(acl::get_x(r) == Catch::Approx(15.0f));
  CHECK(acl::get_y(r) == Catch::Approx(16.0f));
  CHECK(acl::get_z(r) == Catch::Approx(10.f));
  CHECK(acl::get_w(r) == Catch::Approx(14.f));
  CHECK(acl::distance(p, q) == Catch::Approx(20.19901));
  CHECK(acl::sqdistance(p, q) == Catch::Approx(408.0f));
  q = acl::vec4_t<TestType>(10.0f, 12.0f, 5.0f, 8.0f);
  r = acl::vml::set_000w(q.v, 0);
  CHECK(acl::get_x(r) == Catch::Approx(0.0f));
  CHECK(acl::get_y(r) == Catch::Approx(0.0f));
  CHECK(acl::get_z(r) == Catch::Approx(0.0f));
  CHECK(acl::get_w(r) == Catch::Approx(10.0f));
  r = acl::vml::set_000w(q.v, 1);
  CHECK(acl::get_x(r) == Catch::Approx(0.0f));
  CHECK(acl::get_y(r) == Catch::Approx(0.0f));
  CHECK(acl::get_z(r) == Catch::Approx(0.0f));
  CHECK(acl::get_w(r) == Catch::Approx(12.0f));
  r = acl::vml::set_000w(q.v, 2);
  CHECK(acl::get_x(r) == Catch::Approx(0.0f));
  CHECK(acl::get_y(r) == Catch::Approx(0.0f));
  CHECK(acl::get_z(r) == Catch::Approx(0.0f));
  CHECK(acl::get_w(r) == Catch::Approx(5.0f));
  r = acl::vml::set_000w(q.v, 3);
  CHECK(acl::get_x(r) == Catch::Approx(0.0f));
  CHECK(acl::get_y(r) == Catch::Approx(0.0f));
  CHECK(acl::get_z(r) == Catch::Approx(0.0f));
  CHECK(acl::get_w(r) == Catch::Approx(8.0f));
  q = acl::vec4_t<TestType>(10.0f, 12.0f, 5.0f, 8.0f);
  r = acl::vml::set_111w(q.v, 0);
  CHECK(acl::get_x(r) == Catch::Approx(1.0f));
  CHECK(acl::get_y(r) == Catch::Approx(1.0f));
  CHECK(acl::get_z(r) == Catch::Approx(1.0f));
  CHECK(acl::get_w(r) == Catch::Approx(10.0f));
  r = acl::vml::set_111w(q.v, 1);
  CHECK(acl::get_x(r) == Catch::Approx(1.0f));
  CHECK(acl::get_y(r) == Catch::Approx(1.0f));
  CHECK(acl::get_z(r) == Catch::Approx(1.0f));
  CHECK(acl::get_w(r) == Catch::Approx(12.0f));
  r = acl::vml::set_111w(q.v, 2);
  CHECK(acl::get_x(r) == Catch::Approx(1.0f));
  CHECK(acl::get_y(r) == Catch::Approx(1.0f));
  CHECK(acl::get_z(r) == Catch::Approx(1.0f));
  CHECK(acl::get_w(r) == Catch::Approx(5.0f));
  r = acl::vml::set_111w(q.v, 3);
  CHECK(acl::get_x(r) == Catch::Approx(1.0f));
  CHECK(acl::get_y(r) == Catch::Approx(1.0f));
  CHECK(acl::get_z(r) == Catch::Approx(1.0f));
  CHECK(acl::get_w(r) == Catch::Approx(8.0f));
}
