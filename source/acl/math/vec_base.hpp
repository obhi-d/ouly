
#pragma once

#include "deduced_types.hpp"
#include "real.hpp"

namespace acl
{

template <NonQuadVector V>
inline bool equals(V const& v1, V const& v2)
{
  for (std::uint32_t i = 0; i < V::element_count; ++i)
  {
    if constexpr (std::is_same_v<float, typename V::scalar_type>)
    {
      if (!equals(v1[i], v2[i]))
        return false;
    }
    else
    {
      if (v1[i] != v2[i])
        return false;
    }
  }

  return true;
}
template <NonQuadVector V>
inline bool isnan(V const& v)
{
  if constexpr (!std::is_same_v<float, typename V::scalar_type>)
    return false;

  for (std::uint32_t i = 0; i < V::element_count; ++i)
    if (!isnan(v[i]))
      return true;
  return false;
}
template <NonQuadVector V>
inline bool isinf(V const& v)
{
  if constexpr (!std::is_same_v<float, typename V::scalar_type>)
    return false;

  for (std::uint32_t i = 0; i < V::element_count; ++i)
    if (!isinf(v[i]))
      return true;
  return false;
}
template <NonQuadVector V>
inline V isnanv(V const& v)
{
  V ret;
  for (std::uint32_t i = 0; i < V::element_count; ++i)
    ret[i] = (v[i] != v[i]);
  return ret;
}
template <NonQuadVector V>
inline V isinfv(V const& v)
{
  if constexpr (!std::is_same_v<float, typename V::scalar_type>)
    return false;

  V ret;
  for (std::uint32_t i = 0; i < V::element_count; ++i)
    ret[i] = (typename V::scalar_type)isinf(v[i]);
  return ret;
}
template <NonQuadVector V>
inline V set(typename V::scalar_type v)
{
  V ret;
  for (std::uint32_t i = 0; i < V::element_count; ++i)
    ret[i] = v;
  return ret;
}
template <NonQuadVector V>
inline V set(typename V::scalar_type x, typename V::scalar_type y)
{
  return {x, y};
}
template <NonQuadVector V>
inline V set(typename V::scalar_type x, typename V::scalar_type y, typename V::scalar_type z)
{
  return {x, y, z};
}
template <NonQuadVector V>
inline V set(typename V::scalar_type x, typename V::scalar_type y, typename V::scalar_type z, typename V::scalar_type w)
{
  return {x, y, z, w};
}
template <NonQuadVector V>
inline V set(typename V::scalar_type const* v)
{
  V ret;
  for (std::uint32_t i = 0; i < V::element_count; ++i)
    ret[i] = v[i];
  return ret;
}
template <NonQuadVector V>
inline V set_unaligned(typename V::scalar_type const* v)
{
  V ret;
  for (std::uint32_t i = 0; i < V::element_count; ++i)
    ret[i] = v[i];
  return ret;
}
template <NonQuadVector V>
inline V x(V const& v, typename V::scalar_type x)
{
  if constexpr (V::element_count == 1)
    return {x};
  else if constexpr (V::element_count == 2)
    return {x, v[1]};
  else if constexpr (V::element_count == 3)
    return {x, v[1], v[2]};
  else if constexpr (V::element_count == 4)
    return {x, v[1], v[2], v[3]};
  else
  {
    V ret  = v;
    ret[0] = x;
    return ret;
  }
}
template <NonQuadVector V>
inline V y(V const& v, typename V::scalar_type y)
{
  if constexpr (V::element_count == 1)
    return v;
  else if constexpr (V::element_count == 2)
    return {v[0], y};
  else if constexpr (V::element_count == 3)
    return {v[0], y, v[2]};
  else if constexpr (V::element_count == 4)
    return {v[0], y, v[2], v[3]};
  else
  {
    V ret  = v;
    ret[1] = y;
    return ret;
  }
}
template <NonQuadVector V>
inline V z(V const& v, typename V::scalar_type z)
{
  if constexpr (V::element_count == 1)
    return v;
  else if constexpr (V::element_count == 2)
    return v;
  else if constexpr (V::element_count == 3)
    return {v[0], v[1], z};
  else if constexpr (V::element_count == 4)
    return {v[0], v[1], z, v[3]};
  else
  {
    V ret  = v;
    ret[2] = z;
    return ret;
  }
}
template <NonQuadVector V>
inline V w(V const& v, typename V::scalar_type w)
{
  if constexpr (V::element_count == 1)
    return v;
  else if constexpr (V::element_count == 2)
    return v;
  else if constexpr (V::element_count == 3)
    return v;
  else if constexpr (V::element_count == 4)
    return {v[0], v[1], v[2], w};
  else
  {
    V ret  = v;
    ret[3] = w;
    return ret;
  }
}
template <NonQuadVector V>
inline typename  V::scalar_type x(V const& v)
{
  return v[0];
}
template <NonQuadVector V>
inline typename  V::scalar_type y(V const& v)
{
  return v[1];
}
template <NonQuadVector V>
inline typename  V::scalar_type z(V const& v)
{
  return v[2];
}
template <NonQuadVector V>
inline typename  V::scalar_type w(V const& v)
{
  return v[3];
}
template <NonQuadVector V>
inline V zero()
{
  return {};
}
template <NonQuadVector V>
inline V splat_x(V const& v)
{
  constexpr std::uint32_t k_index = 0;
  if constexpr (V::element_count == 1)
    return {v[k_index]};
  else if constexpr (V::element_count == 2)
    return {v[k_index], v[k_index]};
  else if constexpr (V::element_count == 3)
    return {v[k_index], v[k_index], v[k_index]};
  else if constexpr (V::element_count == 4)
    return {v[k_index], v[k_index], v[k_index], v[k_index]};
  else
  {
    V ret;
    for (std::uint32_t i = 0; i < V::element_count; ++i)
      ret[i] = v[k_index];
    return ret;
  }
}
template <NonQuadVector V>
inline V splat_y(V const& v)
{
  constexpr std::uint32_t k_index = 1;
  if constexpr (V::element_count == 2)
    return {v[k_index], v[k_index]};
  else if constexpr (V::element_count == 3)
    return {v[k_index], v[k_index], v[k_index]};
  else if constexpr (V::element_count == 4)
    return {v[k_index], v[k_index], v[k_index], v[k_index]};
  else
  {
    V ret;
    for (std::uint32_t i = 0; i < V::element_count; ++i)
      ret[i] = v[k_index];
    return ret;
  }
}
template <NonQuadVector V>
inline V splat_z(V const& v)
{
  constexpr std::uint32_t k_index = 2;
  if constexpr (V::element_count == 3)
    return {v[k_index], v[k_index], v[k_index]};
  else if constexpr (V::element_count == 4)
    return {v[k_index], v[k_index], v[k_index], v[k_index]};
  else
  {
    V ret;
    for (std::uint32_t i = 0; i < V::element_count; ++i)
      ret[i] = v[k_index];
    return ret;
  }
}
template <NonQuadVector V>
inline V splat_w(V const& v)
{
  constexpr std::uint32_t k_index = 3;
  if constexpr (V::element_count == 4)
    return {v[k_index], v[k_index], v[k_index], v[k_index]};
  else
  {
    V ret;
    for (std::uint32_t i = 0; i < V::element_count; ++i)
      ret[i] = v[k_index];
    return ret;
  }
}
template <NonQuadVector V>
inline V select(V const& v1, V const& v2, V const& control)
{
  V              ret;
  std::uint32_t* iret = reinterpret_cast<std::uint32_t*>(&ret);
  std::uint32_t* iv1  = reinterpret_cast<std::uint32_t*>(&v1);
  std::uint32_t* iv2  = reinterpret_cast<std::uint32_t*>(&v2);
  std::uint32_t* ic   = reinterpret_cast<std::uint32_t*>(&control);
  for (std::uint32_t i = 0; i < V::element_count; ++i)
    iret[i] = (~ic[i] & iv1[i]) | (ic[i] & iv2[i]);
  return ret;
}
template <NonQuadVector V>
inline typename  V::scalar_type get(V const& v, std::uint32_t i)
{
  return v[i];
}
template <NonQuadVector V>
inline V abs(V const& v)
{
  V ret;
  for (std::uint32_t i = 0; i < V::element_count; ++i)
    ret[i] = std::abs(v[i]);
  return ret;
}
template <NonQuadVector V>
inline V negate(V const& v)
{
  V ret;
  for (std::uint32_t i = 0; i < V::element_count; ++i)
    ret[i] = -(v[i]);
  return ret;
}
template <NonQuadVector V>
inline V add(V const& a, V const& b)
{
  V ret;
  for (std::uint32_t i = 0; i < V::element_count; ++i)
    ret[i] = a[i] + b[i];
  return ret;
}
template <NonQuadVector V>
inline V sub(V const& a, V const& b)
{
  V ret;
  for (std::uint32_t i = 0; i < V::element_count; ++i)
    ret[i] = a[i] - b[i];
  return ret;
}
template <NonQuadVector V>
inline V mul(V const& a, V const& b)
{
  V ret;
  for (std::uint32_t i = 0; i < V::element_count; ++i)
    ret[i] = a[i] * b[i];
  return ret;
}
template <NonQuadVector V>
inline V mul(V const& a, typename V::scalar_type b)
{
  V ret;
  for (std::uint32_t i = 0; i < V::element_count; ++i)
    ret[i] = a[i] * b;
  return ret;
}
template <NonQuadVector V>
inline V mul(typename V::scalar_type b, V const& a)
{
  V ret;
  for (std::uint32_t i = 0; i < V::element_count; ++i)
    ret[i] = a[i] * b;
  return ret;
}
template <NonQuadVector V>
inline V half(V const& a)
{
  V ret;
  for (std::uint32_t i = 0; i < V::element_count; ++i)
    ret[i] = a[i] / 2;
  return ret;
}
template <NonQuadVector V>
inline V div(V const& a, V const& b)
{
  V ret;
  for (std::uint32_t i = 0; i < V::element_count; ++i)
    ret[i] = a[i] / b[i];
  return ret;
}
template <NonQuadVector V>
inline V madd(V const& v, V const& m, V const& a)
{
  V ret;
  for (std::uint32_t i = 0; i < V::element_count; ++i)
    ret[i] = (v[i] * m[i]) + a[i];
  return ret;
}
template <NonQuadVector V>
inline typename  V::scalar_type hadd(V const& q)
{
  if constexpr (V::element_count == 1)
    return {q[0]};
  else if constexpr (V::element_count == 2)
    return q[0] + q[1];
  else if constexpr (V::element_count == 3)
    return q[0] + q[1] + q[2];
  else if constexpr (V::element_count == 4)
    return q[0] + q[1] + q[2] + q[3];
  else
  {
    typename V::scalar_type ret = q[0];
    for (std::uint32_t i = 1; i < V::element_count; ++i)
      ret[i] += q[i];
    return ret;
  }
}
template <NonQuadVector V>
inline V min(V const& q1, V const& q2)
{
  V r;
  for (std::uint32_t i = 0; i < V::element_count; ++i)
    r[i] = std::min(q1[i], q2[i]);
  return r;
}
template <NonQuadVector V>
inline V max(V const& q1, V const& q2)
{
  V r;
  for (std::uint32_t i = 0; i < V::element_count; ++i)
    r[i] = std::max(q1[i], q2[i]);
  return r;
}
template <NonQuadVector V>
inline bool greater_all(V const& q1, V const& q2)
{
  for (std::uint32_t i = 0; i < V::element_count; ++i)
    if (q1[i] <= q2[i])
      return false;
  return true;
}
template <NonQuadVector V>
inline bool greater_any(V const& q1, V const& q2)
{
  for (std::uint32_t i = 0; i < V::element_count; ++i)
    if (q1[i] > q2[i])
      return true;
  return false;
}
template <NonQuadVector V>
inline bool lesser_all(V const& q1, V const& q2)
{
  for (std::uint32_t i = 0; i < V::element_count; ++i)
    if (q1[i] >= q2[i])
      return false;
  return true;
}
template <NonQuadVector V>
inline bool lesser_any(V const& q1, V const& q2)
{
  for (std::uint32_t i = 0; i < V::element_count; ++i)
    if (q1[i] < q2[i])
      return true;
  return false;
}
template <NonQuadVector V>
inline V vdot(V const& q1, V const& q2)
{
  return {dot(q1, q2)};
}
template <NonQuadVector V>
inline typename  V::scalar_type dot(V const& q1, V const& q2)
{
  return hadd(mul(q1, q2));
}
template <NonQuadVector V>
inline typename  V::scalar_type sqlength(V const& c1)
{
  return (dot(c1, c1));
}
template <NonQuadVector V>
inline typename  V::scalar_type length(V const& c1)
{
  return std::sqrt(sqlength(c1));
}
template <NonQuadVector V>
inline typename  V::scalar_type distance(V const& vec1, V const& vec2)
{
  return length(sub(vec2, vec1));
}
template <NonQuadVector V>
inline typename  V::scalar_type sqdistance(V const& vec1, V const& vec2)
{
  return sqlength(sub(vec2, vec1));
}
template <NonQuadVector V>
inline V normalize(V const& v)
{
  return mul(v, acl::recip_sqrt(sqlength(v)));
}
template <NonQuadVector V>
inline V lerp(V const& src, V const& dst, typename V::scalar_type t)
{
  return madd(set(t), sub(dst, src), src);
}
template <NonQuadVector V>
inline V recip_sqrt(V const& qpf)
{
  V ret;
  for (std::uint32_t i = 0; i < V::element_count; ++i)
    ret[i] = acl::recip_sqrt(qpf[i]);
  return ret;
}

template <NonQuadVector V>
inline auto operator+(V const& a, V const& b) noexcept
{
  return add(a, b);
}

template <NonQuadVector V>
inline auto operator-(V const& a, V const& b) noexcept
{
  return sub(a, b);
}

template <NonQuadVector V>
inline auto operator/(V const& a, V const& b) noexcept
{
  return div(a, b);
}

template <NonQuadVector V>
inline auto operator*(V const& a, V const& b) noexcept
{
  return mul(a, b);
}

template <NonQuadVector V>
inline auto operator*(V const& a, typename  V::scalar_type b) noexcept
{
  return mul(a, b);
}

template <NonQuadVector V>
inline auto operator*(typename  V::scalar_type a, V const& b) noexcept
{
  return mul(a, b);
}

} // namespace acl
