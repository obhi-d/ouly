// SPDX-License-Identifier: MIT
#pragma once

#if __has_include("user_defines.hpp")
#include "user_defines.hpp"
#endif

#ifndef OULY_ASSERT
#include <cassert>
#define OULY_ASSERT assert
#endif

#ifndef OULY_CACHE_LINE_SIZE
constexpr unsigned int ouly_cache_line_size = 64;
#else
constexpr unsigned int ouly_cache_line_size = OULY_CACHE_LINE_SIZE;
#endif