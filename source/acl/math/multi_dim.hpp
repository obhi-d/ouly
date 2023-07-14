#pragma once
#include "deduced_types.hpp"
#include "quad.hpp"
namespace acl
{

template <Matrix M, typename scalar_t>
inline M mul(scalar_t v, M const& m) noexcept
{
    M r;
    for (uint32_t i = 0 ; i < m.r.size(); ++i)
       r.r[i] = mul(v, m.r[i]);
    return r;
}

template <Matrix M, typename scalar_t>
inline M mul(M const& m, scalar_t v) noexcept
{
  return mul(v, m);
}

template <Matrix M>
inline bool equals(M const& m1, M const& m2) noexcept
{
  for (uint32_t i = 0; i < m1.r.size(); ++i)
    if (!equals(m1.r[i], m2.r[i]))
      return false;
  return true;
}

template <Matrix M>
inline auto const& row(M const& m, uint32_t i) noexcept
{
  return m.r[i];
}

template <Matrix M, typename R>
inline void set_row(M const& m, uint32_t i, R const& r) noexcept
{
  m.r[i] = r;
}

template <Matrix M>
inline auto const& get(M const& m, uint32_t i, uint32_t j) noexcept
{
  return m.e[i][j];
}


template <Matrix M>
inline auto const& add(M const& m1, M const& m2) noexcept
{
  M r;
  for (uint32_t i = 0; i < m1.r.size(); ++i)
    r.r[i] = add(m1.r[i], m2.r[i]);
  return r;
}


template <Matrix M>
inline auto const& sub(M const& m1, M const& m2) noexcept
{
  M r;
  for (uint32_t i = 0; i < m1.r.size(); ++i)
    r.r[i] = sub(m1.r[i], m2.r[i]);
  return r;
}

} // namespace acl
