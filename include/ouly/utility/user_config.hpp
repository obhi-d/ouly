// SPDX-License-Identifier: MIT
#pragma once

#if __has_include("ouly_user_defines.hpp")
#include "ouly_user_defines.hpp"
#endif

#ifndef OULY_ASSERT
#include <cassert>
#define OULY_ASSERT assert
#endif

#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

#ifndef OULY_SCHEDULER_VERSION
#define OULY_SCHEDULER_VERSION v2
#endif

#ifndef OULY_V3_SCHEDULER_SPIN_COUNT
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define OULY_V3_SCHEDULER_SPIN_COUNT 64
#endif

namespace ouly::detail
{

#ifdef OULY_SAFE_BOUNDS

template <typename C, typename I>
constexpr auto vector_access(C&& array, I index) -> decltype(auto)
{
  auto&&     forwarded       = std::forward<C>(array);
  auto const converted_index = static_cast<std::size_t>(index);
  if constexpr (requires { forwarded.size(); })
  {
    OULY_ASSERT(converted_index < static_cast<std::size_t>(forwarded.size()));
  }

  if constexpr (requires { forwarded.at(converted_index); })
  {
    return forwarded.at(converted_index);
  }
  else if constexpr (requires { std::begin(forwarded); })
  {
    using diff_type = std::iterator_traits<decltype(std::begin(forwarded))>::difference_type;
    return *std::next(std::begin(forwarded), static_cast<diff_type>(converted_index));
  }
  else
  {
    return *(forwarded.get() + converted_index);
  }
}

#else

template <typename C, typename I>
constexpr auto vector_access(C&& array, I index) -> decltype(auto)
{
  auto&&     forwarded       = std::forward<C>(array);
  auto const converted_index = static_cast<std::make_unsigned_t<I>>(index);

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
  if constexpr (requires { forwarded[converted_index]; })
  {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
    return forwarded[converted_index];
  }
  else
  {
    return *(forwarded.get() + converted_index);
  }
}

#endif

} // namespace ouly::detail

#ifndef OULY_CACHE_LINE_SIZE
constexpr unsigned int ouly_cache_line_size = 64;
#else
constexpr unsigned int ouly_cache_line_size = OULY_CACHE_LINE_SIZE;
#endif

#ifndef OULY_ANY_DEFAULT_INLINE_SIZE
constexpr unsigned long ouly_any_default_inline_size = 32;
#else
constexpr unsigned long ouly_any_default_inline_size = OULY_ANY_DEFAULT_INLINE_SIZE;
#endif
