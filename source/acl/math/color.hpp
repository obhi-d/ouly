
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

    auto color = mul(
      quad_t<float>{
        c.r * recip,
        c.g * recip,
        c.b * recip,
        c.a * recip,
      },
      factor);
    return color_t<scalar_t>{static_cast<scalar_t>(color.r), static_cast<scalar_t>(color.g),
                             static_cast<scalar_t>(color.b), static_cast<scalar_t>(color.a)};
  }
  else
  {
    constexpr scalar_t recip = 1 / static_cast<scalar_t>(std::numeric_limits<scalar2_t>::max());
    return color_t<scalar_t>{
      c.r * recip,
      c.g * recip,
      c.b * recip,
      c.a * recip,
    };
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
  return color_t<scalar_t>{ppow(c, 1 / gamma), c.a};
}

template <typename scalar_t>
color_t<scalar_t> gamma_to_linear(color_t<scalar_t> const& c, scalar_t gamma = k_default_gamma) noexcept
  requires(std::is_floating_point_v<scalar_t>)
{
  return color_t<scalar_t>{ppow(c, gamma), c.a};
}

template <typename scalar_t>
color_t<scalar_t> linear_to_gamma(color_t<scalar_t> const& c, scalar_t gamma = k_default_gamma) noexcept
  requires(std::is_integral_v<scalar_t>)
{
  constexpr float recip  = 1.0f / static_cast<float>(std::numeric_limits<scalar_t>::max());
  constexpr float factor = static_cast<float>(std::numeric_limits<scalar_t>::max());
  color_t<float>  fcol   = {c.r * recip, c.g * recip, c.b * recip, 0.0f};
  auto            color  = mul(ppow(fcol, 1 / gamma), factor);
  return color_t<scalar_t>{static_cast<scalar_t>(color.r), static_cast<scalar_t>(color.g),
                           static_cast<scalar_t>(color.b), static_cast<scalar_t>(c.a)};
}

template <typename scalar_t>
color_t<scalar_t> gamma_to_linear(color_t<scalar_t> const& c, scalar_t gamma = k_default_gamma) noexcept
  requires(std::is_integral_v<scalar_t>)
{
  constexpr float recip  = 1.0f / static_cast<float>(std::numeric_limits<scalar_t>::max());
  constexpr float factor = static_cast<float>(std::numeric_limits<scalar_t>::max());
  color_t<float>  fcol   = {c.r * recip, c.g * recip, c.b * recip, 0.0f};
  auto            color  = mul(ppow(fcol, gamma), factor);
  return color_t<scalar_t>{static_cast<scalar_t>(color.r), static_cast<scalar_t>(color.g),
                           static_cast<scalar_t>(color.b), static_cast<scalar_t>(c.a)};
}

} // namespace acl