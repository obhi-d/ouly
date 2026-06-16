#pragma once

#include <array>
#include <bit>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>

namespace ouly
{

class uint128_t
{
public:
  static constexpr size_t k_byte_count = 16;

  // NOLINTBEGIN(cppcoreguidelines-special-member-functions)
  constexpr uint128_t() noexcept                                    = default;
  constexpr uint128_t(uint128_t const&) noexcept                    = default;
  constexpr uint128_t(uint128_t&&) noexcept                         = default;
  constexpr auto operator=(uint128_t const&) noexcept -> uint128_t& = default;
  constexpr auto operator=(uint128_t&&) noexcept -> uint128_t&      = default;
  ~uint128_t() noexcept                                             = default;
  // NOLINTEND(cppcoreguidelines-special-member-functions)

  constexpr uint128_t(uint64_t high, uint64_t low) noexcept : m_high(high), m_low(low) {}

  template <std::integral T>
  constexpr uint128_t(T value) noexcept
  {
    if constexpr (std::is_unsigned_v<T>)
    {
      m_low  = static_cast<uint64_t>(value);
      m_high = 0;
    }
    else
    {
      if (value < 0)
      {
        m_low  = static_cast<uint64_t>(value);
        m_high = std::numeric_limits<uint64_t>::max();
      }
      else
      {
        m_low  = static_cast<uint64_t>(value);
        m_high = 0;
      }
    }
  }

  [[nodiscard]] constexpr auto high() const noexcept -> uint64_t
  {
    return m_high;
  }

  [[nodiscard]] constexpr auto low() const noexcept -> uint64_t
  {
    return m_low;
  }

  explicit constexpr operator bool() const noexcept
  {
    return (m_high | m_low) != 0U;
  }

  template <std::unsigned_integral T>
  explicit constexpr operator T() const noexcept
  {
    return static_cast<T>(m_low);
  }

  constexpr auto operator+=(uint128_t const& rhs) noexcept -> uint128_t&
  {
    uint64_t const old_low = m_low;
    m_low += rhs.m_low;
    m_high += rhs.m_high + (m_low < old_low ? 1U : 0U);
    return *this;
  }

  constexpr auto operator-=(uint128_t const& rhs) noexcept -> uint128_t&
  {
    uint64_t const old_low = m_low;
    m_low -= rhs.m_low;
    m_high -= rhs.m_high + (old_low < rhs.m_low ? 1U : 0U);
    return *this;
  }

  constexpr auto operator*=(uint128_t const& rhs) noexcept -> uint128_t&
  {
    auto const [high, low] = multiply(*this, rhs);
    m_high                 = high;
    m_low                  = low;
    return *this;
  }

  constexpr auto operator/=(uint128_t const& rhs) noexcept -> uint128_t&
  {
    auto const [quotient, remainder] = divmod(*this, rhs);
    (void)remainder;
    *this = quotient;
    return *this;
  }

  constexpr auto operator%=(uint128_t const& rhs) noexcept -> uint128_t&
  {
    auto const [quotient, remainder] = divmod(*this, rhs);
    (void)quotient;
    *this = remainder;
    return *this;
  }

  constexpr auto operator&=(uint128_t const& rhs) noexcept -> uint128_t&
  {
    m_high &= rhs.m_high;
    m_low &= rhs.m_low;
    return *this;
  }

  constexpr auto operator|=(uint128_t const& rhs) noexcept -> uint128_t&
  {
    m_high |= rhs.m_high;
    m_low |= rhs.m_low;
    return *this;
  }

  constexpr auto operator^=(uint128_t const& rhs) noexcept -> uint128_t&
  {
    m_high ^= rhs.m_high;
    m_low ^= rhs.m_low;
    return *this;
  }

  constexpr auto operator<<=(uint32_t shift) noexcept -> uint128_t&
  {
    constexpr uint32_t k_bits_total = 128U;
    constexpr uint32_t k_bits_half  = 64U;

    if (shift >= k_bits_total)
    {
      m_high = 0;
      m_low  = 0;
      return *this;
    }

    if (shift >= k_bits_half)
    {
      m_high = m_low << (shift - k_bits_half);
      m_low  = 0;
      return *this;
    }

    if (shift == 0U)
    {
      return *this;
    }

    m_high = (m_high << shift) | (m_low >> (k_bits_half - shift));
    m_low <<= shift;
    return *this;
  }

  constexpr auto operator>>=(uint32_t shift) noexcept -> uint128_t&
  {
    constexpr uint32_t k_bits_total = 128U;
    constexpr uint32_t k_bits_half  = 64U;

    if (shift >= k_bits_total)
    {
      m_high = 0;
      m_low  = 0;
      return *this;
    }

    if (shift >= k_bits_half)
    {
      m_low  = m_high >> (shift - k_bits_half);
      m_high = 0;
      return *this;
    }

    if (shift == 0U)
    {
      return *this;
    }

    m_low = (m_low >> shift) | (m_high << (k_bits_half - shift));
    m_high >>= shift;
    return *this;
  }

  constexpr auto operator++() noexcept -> uint128_t&
  {
    *this += uint128_t{1U};
    return *this;
  }

  constexpr auto operator++(int) noexcept -> uint128_t
  {
    auto tmp = *this;
    ++(*this);
    return tmp;
  }

  constexpr auto operator--() noexcept -> uint128_t&
  {
    *this -= uint128_t{1U};
    return *this;
  }

  constexpr auto operator--(int) noexcept -> uint128_t
  {
    auto tmp = *this;
    --(*this);
    return tmp;
  }

  constexpr auto operator-() const noexcept -> uint128_t
  {
    uint64_t const low  = (~m_low) + 1U;
    uint64_t const high = (~m_high) + (low == 0U ? 1U : 0U);
    return {high, low};
  }

  constexpr auto operator~() const noexcept -> uint128_t
  {
    return {~m_high, ~m_low};
  }

  [[nodiscard]] constexpr auto bytes(std::endian order = std::endian::big) const noexcept
   -> std::array<std::byte, k_byte_count>
  {
    std::array<std::byte, k_byte_count> out{};
    write_bytes(out, resolve_endian(order));
    return out;
  }

  static constexpr auto from_bytes(std::array<std::byte, k_byte_count> const& data,
                                   std::endian order = std::endian::big) noexcept -> uint128_t
  {
    constexpr uint64_t k_bits_per_byte  = 8U;
    constexpr size_t   k_bytes_per_half = 8U;

    auto const resolved = resolve_endian(order);
    uint64_t   high     = 0;
    uint64_t   low      = 0;

    if (resolved == std::endian::big)
    {
      for (size_t i = 0; i < k_bytes_per_half; ++i)
      {
        high = (high << k_bits_per_byte) | static_cast<uint64_t>(std::to_integer<uint8_t>(data[i]));
        low  = (low << k_bits_per_byte) | static_cast<uint64_t>(std::to_integer<uint8_t>(data[i + k_bytes_per_half]));
      }
    }
    else
    {
      for (size_t i = 0; i < k_bytes_per_half; ++i)
      {
        low |= static_cast<uint64_t>(std::to_integer<uint8_t>(data[i])) << (i * k_bits_per_byte);
        high |= static_cast<uint64_t>(std::to_integer<uint8_t>(data[i + k_bytes_per_half])) << (i * k_bits_per_byte);
      }
    }

    return {high, low};
  }

  [[nodiscard]] constexpr auto operator<=>(uint128_t const&) const noexcept = default;

private:
  // NOLINTBEGIN(readability-identifier-naming)
  uint64_t m_high = 0;
  uint64_t m_low  = 0;
  // NOLINTEND(readability-identifier-naming)

  static constexpr auto resolve_endian(std::endian order) noexcept -> std::endian
  {
    if (order == std::endian::native)
    {
      if constexpr (std::endian::native == std::endian::little)
      {
        return std::endian::little;
      }
      else if constexpr (std::endian::native == std::endian::big)
      {
        return std::endian::big;
      }
      else
      {
        static_assert(std::endian::native == std::endian::little || std::endian::native == std::endian::big,
                      "Only little- and big-endian architectures are supported");
      }
    }
    return order;
  }

  constexpr void write_bytes(std::array<std::byte, k_byte_count>& out, std::endian order) const noexcept
  {
    constexpr uint64_t k_bits_per_byte  = 8U;
    constexpr size_t   k_bytes_per_half = 8U;
    constexpr size_t   k_byte_mask      = 0xFFU;
    constexpr size_t   k_last_byte_idx  = 7U;

    if (order == std::endian::big)
    {
      for (size_t i = 0; i < k_bytes_per_half; ++i)
      {
        out[i] = static_cast<std::byte>((m_high >> ((k_last_byte_idx - i) * k_bits_per_byte)) & k_byte_mask);
        out[i + k_bytes_per_half] =
         static_cast<std::byte>((m_low >> ((k_last_byte_idx - i) * k_bits_per_byte)) & k_byte_mask);
      }
    }
    else
    {
      for (size_t i = 0; i < k_bytes_per_half; ++i)
      {
        out[i]                    = static_cast<std::byte>((m_low >> (i * k_bits_per_byte)) & k_byte_mask);
        out[i + k_bytes_per_half] = static_cast<std::byte>((m_high >> (i * k_bits_per_byte)) & k_byte_mask);
      }
    }
  }

  static constexpr auto multiply(uint128_t lhs, uint128_t rhs) noexcept -> std::pair<uint64_t, uint64_t>
  {
    auto const [carry, low] = mul_64_64(lhs.m_low, rhs.m_low);
    uint64_t high           = carry;
    high += mul_64_64(lhs.m_low, rhs.m_high).second;
    high += mul_64_64(lhs.m_high, rhs.m_low).second;
    return {high, low};
  }

  static constexpr auto mul_64_64(uint64_t lhs, uint64_t rhs) noexcept -> std::pair<uint64_t, uint64_t>
  {
    constexpr uint64_t k_mask32 = 0xFFFFFFFFULL;
    constexpr uint32_t k_shift  = 32U;

    uint64_t const lhs_low  = lhs & k_mask32;
    uint64_t const lhs_high = lhs >> k_shift;
    uint64_t const rhs_low  = rhs & k_mask32;
    uint64_t const rhs_high = rhs >> k_shift;

    uint64_t const low_low  = lhs_low * rhs_low;
    uint64_t const mix1     = (lhs_high * rhs_low) + (low_low >> k_shift);
    uint64_t const mid_low  = mix1 & k_mask32;
    uint64_t const mid_high = mix1 >> k_shift;
    uint64_t const mix2     = (lhs_low * rhs_high) + mid_low;
    uint64_t const high     = (lhs_high * rhs_high) + mid_high + (mix2 >> k_shift);
    uint64_t const low      = (mix2 << k_shift) | (low_low & k_mask32);

    return {high, low};
  }

  // Shift-subtract division keeps the implementation portable without relying on compiler builtins.
  static constexpr auto divmod(uint128_t dividend, uint128_t divisor) noexcept -> std::pair<uint128_t, uint128_t>
  {
    assert(divisor);

    constexpr int k_max_bit = 127;

    if (dividend < divisor)
    {
      return {uint128_t{}, dividend};
    }

    uint128_t quotient{};
    uint128_t remainder{};

    for (int bit = k_max_bit; bit >= 0; --bit)
    {
      remainder <<= 1U;
      if (bit_at(dividend, static_cast<uint32_t>(bit)))
      {
        remainder.m_low |= 1U;
      }

      if (remainder >= divisor)
      {
        remainder -= divisor;
        set_bit(quotient, static_cast<uint32_t>(bit));
      }
    }

    return {quotient, remainder};
  }

  static constexpr auto bit_at(uint128_t const& value, uint32_t index) noexcept -> bool
  {
    constexpr uint32_t k_bits_half = 64U;

    if (index < k_bits_half)
    {
      return ((value.m_low >> index) & 1U) != 0U;
    }
    index -= k_bits_half;
    return ((value.m_high >> index) & 1U) != 0U;
  }

  static constexpr void set_bit(uint128_t& value, uint32_t index) noexcept
  {
    constexpr uint32_t k_bits_half = 64U;

    if (index < k_bits_half)
    {
      value.m_low |= (uint64_t{1} << index);
    }
    else
    {
      value.m_high |= (uint64_t{1} << (index - k_bits_half));
    }
  }
};

constexpr auto operator+(uint128_t lhs, uint128_t const& rhs) noexcept -> uint128_t
{
  lhs += rhs;
  return lhs;
}

constexpr auto operator-(uint128_t lhs, uint128_t const& rhs) noexcept -> uint128_t
{
  lhs -= rhs;
  return lhs;
}

constexpr auto operator*(uint128_t lhs, uint128_t const& rhs) noexcept -> uint128_t
{
  lhs *= rhs;
  return lhs;
}

constexpr auto operator/(uint128_t lhs, uint128_t const& rhs) noexcept -> uint128_t
{
  lhs /= rhs;
  return lhs;
}

constexpr auto operator%(uint128_t lhs, uint128_t const& rhs) noexcept -> uint128_t
{
  lhs %= rhs;
  return lhs;
}

constexpr auto operator&(uint128_t lhs, uint128_t const& rhs) noexcept -> uint128_t
{
  lhs &= rhs;
  return lhs;
}

constexpr auto operator|(uint128_t lhs, uint128_t const& rhs) noexcept -> uint128_t
{
  lhs |= rhs;
  return lhs;
}

constexpr auto operator^(uint128_t lhs, uint128_t const& rhs) noexcept -> uint128_t
{
  lhs ^= rhs;
  return lhs;
}

constexpr auto operator<<(uint128_t value, uint32_t shift) noexcept -> uint128_t
{
  value <<= shift;
  return value;
}

constexpr auto operator>>(uint128_t value, uint32_t shift) noexcept -> uint128_t
{
  value >>= shift;
  return value;
}

constexpr void swap(uint128_t& lhs, uint128_t& rhs) noexcept
{
  auto const tmp = lhs;
  lhs            = rhs;
  rhs            = tmp;
}

} // namespace ouly

template <>
struct std::numeric_limits<ouly::uint128_t>
{
  static constexpr bool is_specialized = true;

  static constexpr auto min() noexcept -> ouly::uint128_t
  {
    return ouly::uint128_t{};
  }

  static constexpr auto max() noexcept -> ouly::uint128_t
  {
    return ouly::uint128_t{std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max()};
  }

  static constexpr auto lowest() noexcept -> ouly::uint128_t
  {
    return min();
  }

  static constexpr int  digits       = 128;
  static constexpr int  digits10     = 38;
  static constexpr int  max_digits10 = 0;
  static constexpr bool is_signed    = false;
  static constexpr bool is_integer   = true;
  static constexpr bool is_exact     = true;
  static constexpr int  radix        = 2;

  static constexpr auto epsilon() noexcept -> ouly::uint128_t
  {
    return ouly::uint128_t{};
  }

  static constexpr auto round_error() noexcept -> ouly::uint128_t
  {
    return ouly::uint128_t{};
  }

  static constexpr int min_exponent   = 0;
  static constexpr int min_exponent10 = 0;
  static constexpr int max_exponent   = 0;
  static constexpr int max_exponent10 = 0;

  static constexpr bool has_infinity = false;
  // NOLINTNEXTLINE(readability-identifier-naming)
  static constexpr bool has_quiet_NaN = false;
  // NOLINTNEXTLINE(readability-identifier-naming)
  static constexpr bool               has_signaling_NaN = false;
  static constexpr float_denorm_style has_denorm        = denorm_absent;
  static constexpr bool               has_denorm_loss   = false;
  static constexpr auto               infinity() noexcept -> ouly::uint128_t
  {
    return ouly::uint128_t{};
  }

  // NOLINTNEXTLINE(readability-identifier-naming)
  static constexpr auto quiet_NaN() noexcept -> ouly::uint128_t
  {
    return ouly::uint128_t{};
  }

  // NOLINTNEXTLINE(readability-identifier-naming)
  static constexpr auto signaling_NaN() noexcept -> ouly::uint128_t
  {
    return ouly::uint128_t{};
  }

  static constexpr auto denorm_min() noexcept -> ouly::uint128_t
  {
    return ouly::uint128_t{};
  }

  static constexpr bool is_iec559  = false;
  static constexpr bool is_bounded = true;
  static constexpr bool is_modulo  = true;

  static constexpr bool              traps           = std::numeric_limits<uint64_t>::traps;
  static constexpr bool              tinyness_before = false;
  static constexpr float_round_style round_style     = round_toward_zero;
};

template <>
struct std::hash<ouly::uint128_t>
{
  auto operator()(ouly::uint128_t const& value) const noexcept -> size_t
  {
    constexpr uint32_t prime    = 0x9e3779b9U;
    constexpr uint32_t h1_shift = 6;
    constexpr uint32_t h2_shift = 2;
    auto               h1       = std::hash<uint64_t>{}(value.high());
    auto               h2       = std::hash<uint64_t>{}(value.low());
    // mix bits
    return h1 ^ (h2 + prime + (h1 << h1_shift) + (h1 >> h2_shift));
  }
};

namespace ouly
{

// Custom type traits for uint128_t
// Note: Standard library type traits cannot be specialized for user-defined types in C++20
// These provide equivalent information for template metaprogramming

template <typename T>
struct is_uint128 : std::false_type
{};

template <>
struct is_uint128<uint128_t> : std::true_type
{};

template <typename T>
inline constexpr bool is_uint128_v = is_uint128<T>::value;

// Helper to check if a type is an extended integer (uint128_t or standard integral)
template <typename T>
struct is_extended_integral : std::is_integral<T>
{};

template <>
struct is_extended_integral<uint128_t> : std::true_type
{};

template <typename T>
inline constexpr bool is_extended_integral_v = is_extended_integral<T>::value;

// Helper to check if a type is an extended unsigned integer
template <typename T>
struct is_extended_unsigned : std::is_unsigned<T>
{};

template <>
struct is_extended_unsigned<uint128_t> : std::true_type
{};

template <typename T>
inline constexpr bool is_extended_unsigned_v = is_extended_unsigned<T>::value;

} // namespace ouly
