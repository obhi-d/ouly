
#pragma once

#include "quad.hpp"

namespace acl
{

template <typename scalar_t, typename scalar2_t>
color_t<scalar_t> convert(color_t<scalar2_t> const& c) noexcept
  requires(std::is_integral_v<scalar2_t> && !std::is_same_v<scalar_t, scalar2_t>)
{
  if constexpr (std::is_integral_v<scalar_t>)
  {
    constexpr float recip  = 1.0f / static_cast<float>(std::numeric_limits<scalar2_t>::max());
    constexpr float factor = static_cast<float>(std::numeric_limits<scalar_t>::max());
    constexpr float cf     = recip * factor;

    auto color = c * cf;
    return color_t<scalar_t>{static_cast<scalar_t>(color.r), static_cast<scalar_t>(color.g),
                             static_cast<scalar_t>(color.b), static_cast<scalar_t>(color.a)};
  }
  else
  {
    constexpr scalar_t recip = static_cast<scalar2_t>(1) / static_cast<scalar_t>(std::numeric_limits<scalar2_t>::max());
    return c * recip;
  }
}

template <typename scalar_t, typename scalar2_t>
color_t<scalar_t> convert(color_t<scalar2_t> const& c) noexcept
  requires(std::is_floating_point_v<scalar2_t> && std::is_integral_v<scalar_t>)
{
  constexpr scalar2_t factor = static_cast<scalar2_t>(std::numeric_limits<scalar_t>::max());
  return mul(c, factor);
}

template <typename scalar_t>
color_t<scalar_t> linear_to_gamma(color_t<scalar_t> const& c, scalar_t gamma = k_default_gamma) noexcept
  requires(std::is_floating_point_v<scalar_t>)
{
  return color_t<scalar_t>{vml::ppow(c, static_cast<scalar_t>(1) / gamma), c.a};
}

template <typename scalar_t>
color_t<scalar_t> gamma_to_linear(color_t<scalar_t> const& c, scalar_t gamma = k_default_gamma) noexcept
  requires(std::is_floating_point_v<scalar_t>)
{
  return color_t<scalar_t>{vml::ppow(c, gamma), c.a};
}

template <typename scalar_t>
color_t<scalar_t> linear_to_gamma(color_t<scalar_t> const& c, float gamma = k_default_gamma) noexcept
  requires(std::is_integral_v<scalar_t>)
{
  constexpr float recip  = 1.0f / static_cast<float>(std::numeric_limits<scalar_t>::max());
  constexpr float factor = static_cast<float>(std::numeric_limits<scalar_t>::max());
  color_t<float>  fcol   = {c.r * recip, c.g * recip, c.b * recip, 0.0f};
  auto            color  = vml::mul(vml::ppow(fcol.v, 1.0f / gamma), vml::set(factor));
  return color_t<scalar_t>{static_cast<scalar_t>(vml::get_x(color)), static_cast<scalar_t>(vml::get_y(color)),
                           static_cast<scalar_t>(vml::get_z(color)), static_cast<scalar_t>(c.v[3])};
}

template <typename scalar_t>
color_t<scalar_t> gamma_to_linear(color_t<scalar_t> const& c, float gamma = k_default_gamma) noexcept
  requires(std::is_integral_v<scalar_t>)
{
  constexpr float recip  = 1.0f / static_cast<float>(std::numeric_limits<scalar_t>::max());
  constexpr float factor = static_cast<float>(std::numeric_limits<scalar_t>::max());
  color_t<float>  fcol   = {c.r * recip, c.g * recip, c.b * recip, 0.0f};
  auto            color  = vml::mul(vml::ppow(fcol.v, gamma), vml::set(factor));
  return color_t<scalar_t>{static_cast<scalar_t>(vml::get_x(color)), static_cast<scalar_t>(vml::get_y(color)),
                           static_cast<scalar_t>(vml::get_z(color)), static_cast<scalar_t>(c.v[3])};
}

} // namespace acl