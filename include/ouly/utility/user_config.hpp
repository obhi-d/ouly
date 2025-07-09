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
#if defined(__x86_64__) || defined(__i386__)
#define OULY_CACHE_LINE_SIZE 64
#elif defined(__aarch64__) || defined(__arm__)
#define OULY_CACHE_LINE_SIZE 64
#elif defined(__riscv)
#define OULY_CACHE_LINE_SIZE 64
#else
#define OULY_CACHE_LINE_SIZE 64 // Default fallback
#endif
#endif