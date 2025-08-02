// SPDX-License-Identifier: MIT
#pragma once

#include "ouly/utility/common.hpp"
#include "ouly/utility/tuple.hpp"
#include "ouly/utility/type_traits.hpp"
#include "ouly/utility/utils.hpp"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <utility>

/**
 * Defines a basic_delegate object with a memory store to store a function pointer and its capture inline allocated.
 */
/**
 * @brief A type-safe, lightweight delegate implementation with small object optimization. The captured objects are
 * expected to be trivially destructible.
 *
 * @tparam SmallSize The size in bytes reserved for small object optimization
 * @tparam Ret Return type of the delegate function
 * @tparam Args Argument types of the delegate function
 *
 * This class implements a delegate pattern that can wrap and store different types of callables:
 * - Free functions
 * - Member functions
 * - Lambdas and functors
 * - Functions with associated data (compressed pairs)
 *
 * Features:
 * - Small object optimization for storing callable objects
 * - Type-safe function wrapping
 * - Support for move semantics
 * - Ability to store extra data alongside the function (compressed pairs)
 * - Trivially destructible storage
 *
 * The delegate uses a small buffer optimization technique to avoid heap allocations
 * for small callable objects. The size of this buffer is determined by SmallSize template parameter.
 *
 * Usage examples:
 * ```cpp
 * // Binding a free function
 * auto d1 = basic_delegate<32, void(int)>::bind(&free_function);
 *
 * // Binding a member function
 * auto d2 = basic_delegate<32, void(int)>::bind<&Class::method>(instance);
 *
 * // Binding a lambda
 * auto d3 = basic_delegate<32, void(int)>::bind([](int x) { ... });
 *
 * // Binding with associated data
 * auto d4 = basic_delegate<32, void(int)>::pbind(func, extra_data);
 * ```
 *
 * @note All stored callable objects must be trivially destructible
 * @warning The size of stored callables must not exceed the specified SmallSize
 */
namespace ouly
{
/**
 * @brief A lightweight delegate implementation with small object optimization. The captured objects are expected to be
 * trivially destructible.
 *
 * This class provides a mechanism to store and invoke callable objects, including
 * free functions, member functions, and lambdas, with support for small object optimization.
 *
 * @tparam SmallSize The size threshold for small object optimization.
 * @tparam Ret The return type of the callable.
 * @tparam Args The argument types for the callable.
 */
template <size_t SmallSize, typename>
class basic_delegate;

/**  */
template <size_t SmallSize, typename Ret, typename... Args>
class basic_delegate<SmallSize, Ret(Args...)>
{
  using fnptr = Ret (*)(basic_delegate& data, Args...);

  struct invocable_base
  {
    fnptr callable_ = nullptr;
  };

  template <typename Lambda>
  struct invocable_impl : invocable_base
  {
    static auto execute(basic_delegate& data, Args... args) -> Ret
    {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      auto* invocable = reinterpret_cast<invocable_impl<Lambda>*>(data.buffer_.data());
      return invocable->lambda_(std::forward<Args>(args)...);
    }

    invocable_impl(Lambda&& lambda) noexcept : invocable_base{&invocable_impl::execute}, lambda_(std::move(lambda)) {}
    invocable_impl(Lambda const& lambda) noexcept : invocable_base{&invocable_impl::execute}, lambda_(lambda) {}
    invocable_impl(fnptr callable, Lambda&& lambda) noexcept : invocable_base{callable}, lambda_(std::move(lambda)) {}
    invocable_impl(fnptr callable, Lambda const& lambda) noexcept : invocable_base{callable}, lambda_(lambda) {}

    Lambda lambda_;
  };

public:
  using function_type = fnptr;

  struct noinit_t
  {};
  static constexpr noinit_t noinit{};

  basic_delegate() noexcept : buffer_{} {}
  // NOLINTNEXTLINE
  explicit basic_delegate(noinit_t /* unused */) noexcept {}
  basic_delegate(basic_delegate const&)     = default;
  basic_delegate(basic_delegate&&) noexcept = default;
  ~basic_delegate() noexcept                = default;

  // NOLINTNEXTLINE
  auto operator=(basic_delegate const&) -> basic_delegate& = default;
  // NOLINTNEXTLINE
  auto operator=(basic_delegate&&) noexcept -> basic_delegate& = default;

  template <typename Lambda>
    requires(std::is_invocable_r_v<Ret, Lambda, Args...>)
  static auto bind(Lambda&& lambda) noexcept -> basic_delegate<SmallSize, Ret(Args...)>
  {
    using LambdaType = std::decay_t<Lambda>;
    static_assert(sizeof(invocable_impl<LambdaType>) <= SmallSize,
                  "Lambda size exceeds small object optimization size");
    static_assert(std::is_trivially_destructible_v<LambdaType> && std::is_trivially_copyable_v<LambdaType>,
                  "Lambda must be trivially destructible and copyable for basic_delegate");

    basic_delegate ret{noinit};
    new (ret.buffer_.data()) invocable_impl<LambdaType>(std::forward<Lambda>(lambda));
    return ret;
  }

  template <typename... PackArgs>
  static auto bind(fnptr fn, PackArgs&&... args) -> basic_delegate<SmallSize, Ret(Args...)>
  {
    using ArgPack = std::tuple<std::decay_t<PackArgs>...>;
    basic_delegate ret{noinit};
    new (ret.buffer_.data()) invocable_impl<ArgPack>(fn, ArgPack(std::forward<PackArgs>(args)...));
    return ret;
  }

  template <typename... PackArgs>
  auto packed_args() const noexcept -> std::tuple<PackArgs...> const&
  {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return *reinterpret_cast<std::tuple<PackArgs...> const*>(buffer_.data() + sizeof(invocable_base));
  }

  // Invocation operator
  auto operator()(Args... args) -> Ret
  {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* invocable = reinterpret_cast<invocable_base*>(buffer_.data());
    OULY_ASSERT(invocable);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return invocable->callable_(*this, std::forward<Args>(args)...);
  }

  explicit operator bool() const noexcept
  {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto const* invocable = reinterpret_cast<invocable_base const*>(buffer_.data());
    return invocable->callable_ != nullptr;
  }

private:
  alignas(alignof(std::max_align_t)) std::array<uint8_t, SmallSize> buffer_;
};

constexpr uint32_t max_delegate_base_size = 64;
template <typename V>
using delegate = basic_delegate<max_delegate_base_size, V>;

} // namespace ouly
