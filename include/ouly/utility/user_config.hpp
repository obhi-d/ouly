// SPDX-License-Identifier: MIT
#pragma once

#if __has_include("user_defines.hpp")
#include "user_defines.hpp"
#endif

#ifndef OULY_ASSERT
#include <cassert>
#define OULY_ASSERT(x) assert(x)
#endif
