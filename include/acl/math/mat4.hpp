#pragma once
#include "aabb.hpp"
#include "mat_base.hpp"
namespace acl
{

template <typename scalar_t>
inline mat4_t<scalar_t> make_identity_mat4() noexcept
{
  return mat4_t<scalar_t>();
}

/**
 * @brief Returns maximum scaling
 */
template <typename scalar_t>
inline scalar_t max_scale(mat4_t<scalar_t> const& m) noexcept
{
  return std::sqrt(std::max(std::max(vml::sqlength(m.v[0]), vml::sqlength(m.v[1])), vml::sqlength(m.v[2])));
}

template <typename scalar_t>
inline bool equals(mat4_t<scalar_t> const& a, mat4_t<scalar_t> const& b) noexcept
{
  return equals(a[0], b[0]) && equals(a[1], b[1]) && equals(a[2], b[2]) && equals(a[3], b[3]);
}

/**
 * @brief Full matrix multiplication
 */
template <typename scalar_t>
inline mat4_t<scalar_t> operator*(mat4_t<scalar_t> const& m1, mat4_t<scalar_t> const& m2) noexcept
{
  return vml::mul_mat4(m1.v, m2.v);
}

/**
 * @brief Full matrix multiplication
 */
template <typename scalar_t>
inline vec3a_t<scalar_t> operator*(vec3a_t<scalar_t> const& m1, mat4_t<scalar_t> const& m2) noexcept
{
  return vml::mul_transform(m1.v, m2.v);
}

/**
 * @brief Full matrix multiplication
 */
template <typename scalar_t>
inline vec4_t<scalar_t> operator*(vec4_t<scalar_t> const& m1, mat4_t<scalar_t> const& m2) noexcept
{
  return vml::mul_quad_mat4(m1.v, m2.v);
}

template <typename scalar_t>
inline extends3d_t<scalar_t> operator*(extends3d_t<scalar_t> const& v, mat4_t<scalar_t> const& m) noexcept
{
  return vml::mul_extends_mat4(v.v, m.v);
}

template <typename scalar_t>
inline aabb_t<scalar_t> operator*(aabb_t<scalar_t> const& box, mat4_t<scalar_t> const& m) noexcept
{
  return vml::mul_aabb_mat4(box.v, m.v);
}

template <typename scalar_t>
inline mat4_t<scalar_t> make_mat4(scalar_t scale, quat_t<scalar_t> const& rot, vec3a_t<scalar_t> const& pos) noexcept
{
  return vml::make_mat4(scale, rot.v, pos.v);
}

template <typename scalar_t>
inline mat4_t<scalar_t> make_mat4_from_scale(vec3a_t<scalar_t> const& scale) noexcept
{
  return mat4_t<scalar_t>{scale[0], 0, 0, 0, 0, scale[1], 0, 0, 0, 0, scale[2], 0, 0, 0, 0, 1};
}

template <typename scalar_t>
inline mat4_t<scalar_t> make_mat4_from_translation(vec3a_t<scalar_t> const& pos) noexcept
{
  return mat4_t<scalar_t>{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, pos[0], pos[1], pos[2], 1};
}

template <typename scalar_t>
inline mat4_t<scalar_t> make_mat4_form_quaternion(quat_t<scalar_t> const& rot) noexcept
{
  mat4_t<scalar_t> ret{noinit_v};
  set_rotation(ret, rot);
  ret[3] = vec4_t<scalar_t>(0, 0, 0, 1);
  return ret;
}

template <typename scalar_t>
inline mat3_t<scalar_t> const& as_mat3(mat4_t<scalar_t> const& m) noexcept
{
  return reinterpret_cast<mat3_t<scalar_t> const&>(m);
}

template <typename scalar_t>
inline mat3_t<scalar_t>& as_mat3(mat4_t<scalar_t>& m) noexcept
{
  return reinterpret_cast<mat3_t<scalar_t>&>(m);
}

template <typename scalar_t>
inline mat4_t<scalar_t> inverse_assume_ortho(mat4_t<scalar_t> const& m) noexcept
{
  // inverse = [ T(R)       0 ]
  //           [ -vpos*T(R) 1 ]
  mat4_t<scalar_t> ret{noinit_v};
  as_mat3(ret) = transpose(as_mat3(m));
  ret[3]       = vec4_t<scalar_t>(vml::set_w(vml::mul_quad_mat3(vml::negate(m.v[3]), as_mat3(ret).v), 1));
  return ret;
}

template <typename scalar_t>
inline mat4_t<scalar_t> make_view_from_world_mat4(mat4_t<scalar_t> const& m) noexcept
{
  return inverse_assume_ortho(m);
}

template <typename scalar_t>
inline mat4_t<scalar_t> make_view_from_look_at(vec3a_t<scalar_t> const& eye, vec3a_t<scalar_t> const& look_at,
                                               vec3a_t<scalar_t> const& vup) noexcept
{
  mat4_t<scalar_t> m{noinit_v};
  auto             zaxis = m.v[2] = vml::normalize(vml::sub(look_at.v, eye.v));
  auto             xaxis = m.v[0] = vml::normalize(vml::cross(vup.v, m.v[2]));
  auto             yaxis = m.v[1] = vml::cross(m.v[2], m.v[0]);
  as_mat3(m)                      = vml::transpose(as_mat3(m).v);
  m[3]                            = vec4_t<scalar_t>(vml::set_w(vml::mul_quad_mat3(vml::negate(eye), as_mat3(m).v), 1));

  return m;
}

template <typename scalar_t>
inline mat4_t<scalar_t> make_orthographic_projection(scalar_t min_x, scalar_t max_x, scalar_t min_y, scalar_t max_y,
                                                     scalar_t zn, scalar_t zf) noexcept
{
  scalar_t dx_recip = 1.0f / (zf - zn);
  // clang-format off
  return mat4_t<scalar_t>{
		2.0f / (max_x - min_x),              0.0f,   	                          0.0f,             0.0f,
	    0.0f,                                2.0f / (max_y - min_y),              0.0f,             0.0f,
	    0.0f,                                0.0f,                                dx_recip,         0.0f,
	    (max_x + min_x) / (min_x - max_x),   (max_y + min_y) / (max_y - min_y),   -(zn) * dx_recip, 1.0f
  };
  // clang-format on
}

template <typename scalar_t>
inline mat4_t<scalar_t> make_orthographic_projection(scalar_t w, scalar_t h, scalar_t zn, scalar_t zf) noexcept
{
  scalar_t dx_recip = 1.0f / (zf - zn);
  // clang-format off
  return mat4_t<scalar_t>{
		2.0f / w,  0.0f,   	   0.0f,                0.0f,
	    0.0f,      2.0f / h,   0.0f,                0.0f,
	    0.0f,      0.0f,       dx_recip,            0.0f,
	    0.0f,      0.0f,      -(zn) * dx_recip,     1.0f
  };
  // clang-format on
}

template <typename scalar_t>
inline mat4_t<scalar_t> make_perspective_projection(scalar_t field_of_view, scalar_t aspect_ratio, scalar_t zn,
                                                    scalar_t zf) noexcept
{
  field_of_view *= 0.5f;

  scalar_t yscale = 1 / std::tan(field_of_view);
  scalar_t q      = zf / (zf - zn);
  // clang-format off
    return mat4_t<scalar_t>{
		(yscale / aspect_ratio),        0,        0,        0,
	     0,                        yscale,        0,        0,
		 0,                             0,        q,        1,
		 0,                             0,  -q * zn,        0
	};
  // clang-format on
}

template <FloatingType scalar_t, ScalarType mult_t>
inline mat4_t<scalar_t> operator*(mat4_t<scalar_t> const& m, mult_t scale) noexcept
{
  mat4_t<scalar_t> ret{noinit_v};
  vec4_t<scalar_t> s{static_cast<scalar_t>(scale)};
  for (int i = 0; i < 4; ++i)
    ret.r[i] = m.r[i] * s;

  return ret;
}

template <FloatingType scalar_t, ScalarType mult_t>
inline mat4_t<scalar_t> operator*(mult_t scale, mat4_t<scalar_t> const& m) noexcept
{
  return m * scale;
}

template <typename scalar_t>
inline mat4_t<scalar_t> transpose(mat4_t<scalar_t> const& m) noexcept
{
  return vml::transpose(m.v);
}

/**
 * @brief Matrix full inverse computation
 */
template <typename scalar_t>
inline mat4_t<scalar_t> inverse(mat4_t<scalar_t> const& m) noexcept
{
  return vml::inverse(m.v);
}

template <typename scalar_t>
inline auto operator+(mat4_t<scalar_t> const& a, mat4_t<scalar_t> const& b) noexcept
{
  mat4_t<scalar_t> r{noinit_v};
  for (int i = 0; i < 4; ++i)
    r.r[i] = a.r[i] + b.r[i];
  return r;
}

template <typename scalar_t>
inline auto operator-(mat4_t<scalar_t> const& a, mat4_t<scalar_t> const& b) noexcept
{
  mat4_t<scalar_t> r{noinit_v};
  for (int i = 0; i < 4; ++i)
    r.r[i] = a.r[i] - b.r[i];
  return r;
}

template <typename scalar_t>
inline bool test_orthogonal(mat4_t<scalar_t> const& m) noexcept
{
  return test_orthogonal(as_mat3(m));
}

} // namespace acl
