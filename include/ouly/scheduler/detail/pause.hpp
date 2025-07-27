
#pragma once

#ifdef _MSC_VER
#if (defined(__aarch64__) || defined(__arm__)) // Arm & AArch64)
#include <Windows.h>
#else
#include <intrin.h> // For _mm_pause() intrinsic
#endif
#else
#if (defined(__i386__) || defined(__x86_64__))   // 32- & 64-bit x86
#include <immintrin.h>                           // For __builtin_ia32_pause() intrinsic
#elif (defined(__aarch64__) || defined(__arm__)) // Arm & AArch64
#include <arm_acle.h>                            // For __builtin_arm_yield() intrinsic
#elif defined(__riscv)                           // RISC-V
#include <riscv_acle.h>                          // For __builtin_riscv_pause() intrinsic
#endif
#endif

namespace ouly::detail
{

// Always-inline to keep the call zero-overhead in tight loops
#ifndef _MSC_VER
[[gnu::always_inline]]
#endif
inline void pause_exec() noexcept
{
#if defined(__i386__) || defined(__x86_64__) // 32- & 64-bit x86
#if defined(_MSC_VER)
  _mm_pause(); // MSVC / Intel C++
#else
  __builtin_ia32_pause(); // GCC / Clang
#endif

#elif defined(__aarch64__) || defined(__arm__) // Arm & AArch64
#if defined(_MSC_VER)
  __yield(); // MSVC intrinsic
#else
  __builtin_arm_yield(); // ACLE 8.4 intrinsic
#endif

#elif defined(__riscv) // RISC-V Zihintpause
  __builtin_riscv_pause(); // GCC/Clang ≥ 13

#elif defined(YieldProcessor) // any Windows target
  YieldProcessor(); // expands appropriately

#else // last-ditch fallback
  std::this_thread::yield();
#endif
}

} // namespace ouly::detail
