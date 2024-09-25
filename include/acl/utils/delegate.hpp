
#pragma once

#include <acl/utils/type_traits.hpp>
#include <cstdint>
#include <type_traits>

/**
 * Defines a basic_delegate object with a memory store to store a function pointer and its capture inline allocated.
 */
namespace acl
{

/**
 *
 * A basic_delegate can be constructed as such:
 *
 * This form captures the instance and member function
 * `Class object; auto fn = basic_delegate<16>(small_size, &Class::method, &object, args...);`
 *
 * This form captures a free function.
 * `* auto fn = basic_delegate<16>(&free_function, args...);`
 *
 * This form captures lambda. Keep in mind the inlined buffer size is set to 16 and the lambda size must not exceed this
 * size.
 * ```
 * auto fn = basic_delegate<16>([this](Args&&...args){}, args...);
 * ```
 *
 * The idea is basic_delegate has a small object buffer of configurable size passed as first parameter. It performs a
 * type erasure of the passed method, and converts it into a free function of this type, and calls this method in
 * operator(): `using delegate_fn = Ret(*)(basic_delegate<s>& data,Args...);`
 */

template <size_t SmallSize, typename>
class basic_delegate;

template <size_t SmallSize, typename Ret, typename... Args>
class basic_delegate<SmallSize, Ret(Args...)>
{
  using delegate_fn               = Ret (*)(basic_delegate&, Args&&...);
  delegate_fn             invoker = nullptr;

  // Small object optimization buffer
  alignas(std::max_align_t) uint8_t buffer[SmallSize];

  // Helper to call free functions
  template <typename F>
  static Ret invoke_free_function(basic_delegate& d, Args&&... args)
  {
    // Retrieve the stored function and cast it back to its original type
    auto& func = *reinterpret_cast<F*>(d.buffer);
    return func(std::forward<Args>(args)...);
  }

  // Helper to call member functions
  template <typename M>
  static Ret invoke_member_function(basic_delegate& d, Args&&... args)
  {
    using C = typename M::class_type;
    using F = typename M::function_type;

    auto data = *reinterpret_cast<C**>(d.buffer);
    return M::invoke(*data, std::forward<Args>(args)...);
  }

  // Helper to call lambdas or functors
  template <typename F>
  static Ret invoke_lambda(basic_delegate& d, Args&&... args)
  {
    auto& func = *reinterpret_cast<F*>(d.buffer);
    return func(std::forward<Args>(args)...);
  }

public:
  // Default constructor
  basic_delegate() noexcept = default;

  // Constructor for free functions or lambda objects
  template <typename F>
    requires(!std::is_same_v<basic_delegate<SmallSize, Ret(Args...)>, std::decay_t<F>>)
  basic_delegate(F&& func)
  {
    using DecayedF = std::decay_t<F>;
    static_assert(sizeof(DecayedF) <= SmallSize, "Function/Lamda object too large for inline storage.");
    static_assert(std::is_trivially_destructible_v<DecayedF> && std::is_trivially_copyable_v<DecayedF>,
                  "Capture type should be trivially copyable and destructible");

    new (buffer) DecayedF(std::forward<F>(func));
    if constexpr (std::is_function_v<DecayedF>)
      invoker = &invoke_free_function<DecayedF>;
    else
      invoker = &invoke_lambda<DecayedF>;
  }

  // Constructor for member functions
  template <auto M>
  basic_delegate(typename acl::member_function<M>::class_type& instance, acl::member_function<M> = {})
  {
    using C = typename acl::member_function<M>::class_type;
    using F = typename acl::member_function<M>::function_type;

    static_assert(sizeof(C*) <= SmallSize, "Member function object too large for inline storage.");
    *reinterpret_cast<C**>(buffer) = &instance;
    invoker                        = &invoke_member_function<acl::member_function<M>>;
  }

  // Move constructor
  basic_delegate(basic_delegate&& other) noexcept
  {
    std::memcpy(buffer, other.buffer, SmallSize);
    invoker       = other.invoker;
    other.invoker = nullptr;
  }

  // Move assignment operator
  basic_delegate& operator=(basic_delegate&& other) noexcept
  {
    if (this != &other)
    {
      std::memcpy(buffer, other.buffer, SmallSize);
      invoker       = other.invoker;
      other.invoker = nullptr;
    }
    return *this;
  }

  // Move constructor
  basic_delegate(basic_delegate const& other) noexcept
  {
    std::memcpy(buffer, other.buffer, SmallSize);
    invoker = other.invoker;
  }

  // Move assignment operator
  basic_delegate& operator=(basic_delegate const& other) noexcept
  {
    if (this != &other)
    {
      std::memcpy(buffer, other.buffer, SmallSize);
      invoker = other.invoker;
    }
    return *this;
  }

  inline explicit operator bool() const noexcept
  {
    return invoker != nullptr;
  }

  // Invocation operator
  Ret operator()(Args... args)
  {
    assert(invoker);
    return invoker(*this, std::forward<Args>(args)...);
  }
};

// Deduction guide for free function
template <typename Ret, typename... Args>
basic_delegate(Ret (*)(Args...)) -> basic_delegate<sizeof(void*), Ret(Args...)>;

// Deduction guide for lambdas and functors
template <typename F>
basic_delegate(F&&) -> basic_delegate<sizeof(F), decltype(&std::decay_t<F>::operator())>;

// template <typename C, auto M>
// basic_delegate(C&, acl::member_function<M>) -> basic_delegate<sizeof(C*), typename
// member_function<M>::function_type>;

template <typename V>
using delegate = basic_delegate<24, V>;

} // namespace acl