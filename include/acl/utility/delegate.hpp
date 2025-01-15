
#pragma once

#include <acl/utility/common.hpp>
#include <acl/utility/tuple.hpp>
#include <acl/utility/type_traits.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

/**
 * Defines a basic_delegate object with a memory store to store a function pointer and its capture inline allocated.
 */
/**
 * @brief A type-safe, lightweight delegate implementation with small object optimization.
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
namespace acl
{
template <size_t SmallSize, typename>
class basic_delegate;

/**  */
template <size_t SmallSize, typename Ret, typename... Args>
class basic_delegate<SmallSize, Ret(Args...)>
{

  using delegate_fn                   = Ret (*)(basic_delegate&, Args...);
  static constexpr size_t buffer_size = sizeof(void*) + SmallSize;

  // Small object optimization buffer

  alignas(std::max_align_t) std::byte buffer_[buffer_size] = {};

  template <typename P>
  struct compressed_pair
  {
    static constexpr size_t small_functor_size = buffer_size - sizeof(P);
    alignas(std::max_align_t) std::byte functor_[small_functor_size];
    alignas(alignof(P)) std::byte data_[sizeof(P)];
  };

  // Helper to call free functions
  template <typename F>
  static auto invoke_free_function_by_pointer(basic_delegate& d, Args... args) -> Ret
  {
    // Retrieve the stored function and cast it back to its original type
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto& func = *reinterpret_cast<F*>(d.buffer_ + sizeof(delegate_fn));
    return func(std::forward<Args>(args)...);
  }

  // Helper to call free functions
  template <auto F>
  static auto invoke_free_function_by_binding(basic_delegate& d, Args... args) -> Ret
  {
    // Retrieve the stored function and cast it back to its original type
    return F(std::forward<Args>(args)...);
  }

  // Helper to call member functions
  template <typename M>
  static auto invoke_member_function(basic_delegate& d, Args... args) -> Ret
  {
    using C = typename M::class_type;
    using F = typename M::function_type;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto data = *reinterpret_cast<C**>(d.buffer_ + sizeof(delegate_fn));
    return M::invoke(*data, std::forward<Args>(args)...);
  }

  // Helper to call lambdas or functors
  template <typename F>
  static auto invoke_lambda(basic_delegate& d, Args... args) -> Ret
  {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto& func = *reinterpret_cast<F*>(d.buffer_ + sizeof(delegate_fn));
    return func(std::forward<Args>(args)...);
  }

  // Helper to call free functions
  template <typename F, typename P>
  static auto p_invoke_free_function_by_pointer(basic_delegate& d, Args... args) -> Ret
  {
    // Retrieve the stored function and cast it back to its original type
    auto& func =
     // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
     *reinterpret_cast<F*>(reinterpret_cast<compressed_pair<P>*>(d.buffer_)->functor_ + sizeof(delegate_fn));

    return func(std::forward<Args>(args)...);
  }

  // Helper to call member functions
  template <typename M, typename P>
  static auto p_invoke_member_function(basic_delegate& d, Args... args) -> Ret
  {
    using C = typename M::class_type;
    using F = typename M::function_type;

    auto data =
     // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
     *reinterpret_cast<C**>(reinterpret_cast<compressed_pair<P>*>(d.buffer_)->functor_ + sizeof(delegate_fn));
    return M::invoke(*data, std::forward<Args>(args)...);
  }

  // Helper to call lambdas or functors
  template <typename F, typename P>
  static auto p_invoke_lambda(basic_delegate& d, Args... args) -> Ret
  {
    auto& func =
     // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
     *reinterpret_cast<F*>(reinterpret_cast<compressed_pair<P>*>(d.buffer_)->functor_ + sizeof(delegate_fn));
    return func(std::forward<Args>(args)...);
  }

  void construct(delegate_fn fn)
  {
    *(delegate_fn*)(buffer_) = fn;
  }

  template <typename F>
  void construct(delegate_fn fn, F&& arg)
  {
    using DecayedF = std::decay_t<F>;
    typed_static_assert<sizeof(DecayedF) <= SmallSize && "Function/Lamda object too large for inline storage.",
                        DecayedF>();
    typed_static_assert<std::is_trivially_destructible_v<DecayedF> && "Capture type should be trivially destructible",
                        DecayedF>();

    *(delegate_fn*)(buffer_) = fn;
    new (buffer_ + sizeof(delegate_fn)) DecayedF(std::forward<F>(arg));
  }

  template <typename P>
  void pconstruct(delegate_fn fn, P&& p)
  {
    using DecayedP       = std::decay_t<P>;
    using CompressedPair = compressed_pair<DecayedP>;
    typed_static_assert<sizeof(CompressedPair) <= buffer_size && "Function/Lamda object too large for inline storage.",
                        DecayedP>();
    typed_static_assert<std::is_trivially_destructible_v<DecayedP>, "Parameter type should be trivially destructible",
                        DecayedP>();

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto pair                       = reinterpret_cast<compressed_pair<P>*>(buffer_);
    *(delegate_fn*)(pair->functor_) = fn;
    new (pair->data_) DecayedP(std::forward<P>(p));
  }

  template <typename F, typename P>
  void pconstruct(delegate_fn fn, F&& arg, P&& p)
  {
    using DecayedF           = std::decay_t<F>;
    using DecayedP           = std::decay_t<P>;
    using CompressedPair     = compressed_pair<DecayedP>;
    constexpr bool condition = (sizeof(CompressedPair) <= buffer_size &&
                                CompressedPair::small_functor_size >= (sizeof(delegate_fn) + sizeof(DecayedF)));
    typed_static_assert<condition && "Function/Lamda object too large for inline storage.", DecayedP>();
    typed_static_assert<std::is_trivially_destructible_v<DecayedF> && "Capture type should be trivially destructible",
                        DecayedF>();
    typed_static_assert<std::is_trivially_destructible_v<DecayedP> && "Parameter type should be trivially destructible",
                        DecayedP>();

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto pair                       = reinterpret_cast<compressed_pair<P>*>(buffer_);
    *(delegate_fn*)(pair->functor_) = fn;
    new (pair->functor_ + sizeof(delegate_fn)) DecayedF(std::forward<F>(arg));
    new (pair->data_) DecayedP(std::forward<P>(p));
  }

public:
  using fnptr = delegate_fn;

  ~basic_delegate() noexcept = default;
  // Default constructor
  basic_delegate() noexcept = default;

  // Move constructor
  basic_delegate(basic_delegate&& other) noexcept
  {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    std::memcpy(reinterpret_cast<void*>(buffer_), reinterpret_cast<void const*>(other.buffer_), buffer_size);
    *(delegate_fn*)other.buffer_ = nullptr;
  }

  // Move assignment operator
  auto operator=(basic_delegate&& other) noexcept -> basic_delegate&
  {
    if (this != &other)
    {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      std::memcpy(reinterpret_cast<void*>(buffer_), reinterpret_cast<void const*>(other.buffer_), buffer_size);
      *(delegate_fn*)other.buffer_ = nullptr;
    }
    return *this;
  }

  // Move constructor
  basic_delegate(basic_delegate const& other) noexcept
  {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    std::memcpy(reinterpret_cast<void*>(buffer_), reinterpret_cast<void const*>(other.buffer_), buffer_size);
  }

  // Move assignment operator
  auto operator=(basic_delegate const& other) noexcept -> basic_delegate&
  {
    if (this != &other)
    {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      std::memcpy(reinterpret_cast<void*>(buffer_), reinterpret_cast<void const*>(other.buffer_), buffer_size);
    }
    return *this;
  }

  /** Bind method for a functor_ or lambda like object */
  template <typename F>
  static auto bind(F&& func) -> basic_delegate
  {
    using DecayedF = std::decay_t<F>;
    basic_delegate r;
    if constexpr (std::is_function_v<DecayedF>)
    {
      if constexpr (std::is_assignable_v<fnptr&, DecayedF>)
      {
        r.construct(func);
      }
      else
      {
        r.construct(&invoke_free_function_by_pointer<DecayedF>, std::forward<F>(func));
      }
    }
    else
    {
      if constexpr (std::is_assignable_v<fnptr&, DecayedF>)
      {
        r.construct(func);
      }
      else
      {
        r.construct(&invoke_lambda<DecayedF>, std::forward<F>(func));
      }
    }
    return r;
  }

  /** Bind method for a class member and its class instance */
  template <auto M>
  static auto bind(typename acl::member_function<M>::class_type& instance) -> basic_delegate
  {
    using C = typename acl::member_function<M>::class_type;
    using F = typename acl::member_function<M>::function_type;

    basic_delegate r;
    r.construct(&invoke_member_function<acl::member_function<M>>, &instance);
    return r;
  }

  /** Bind a free function */
  template <auto F>
  static auto bind() -> basic_delegate
  {
    basic_delegate r;
    r.construct(&invoke_free_function_by_binding<F>);
    return r;
  }

  template <typename L, typename F>
  static auto bind(L&& func, F&& data) -> basic_delegate
  {
    basic_delegate r;
    using DecayedL = std::decay_t<L>;
    static_assert(std::is_assignable_v<fnptr&, DecayedL>, "Can only accept function pointer");
    r.construct(std::forward<L>(func), std::forward<F>(data));
    return r;
  }

  /**
   * Bind method for a compressed pair of function + some data, that is not passed to the function, but can be later
   * obtained from the packaged function object. The compressed pair must fit within the SmallBuffer along with its
   * alignment requirements.
   */
  template <typename F, typename P>
  static auto pbind(F&& func, P&& p) -> basic_delegate
  {
    using DecayedF = std::decay_t<F>;
    using DecayedP = std::decay_t<P>;

    basic_delegate r;
    if constexpr (std::is_function_v<DecayedF>)
    {
      if constexpr (std::is_assignable_v<fnptr&, DecayedF>)
      {
        r.pconstruct(func, std::forward<P>(p));
      }
      else
      {
        r.pconstruct(&p_invoke_free_function_by_pointer<DecayedF, DecayedP>, std::forward<F>(func), std::forward<P>(p));
      }
    }
    else
    {
      if constexpr (std::is_assignable_v<fnptr&, DecayedF>)
      {
        r.pconstruct(func, std::forward<P>(p));
      }
      else
      {
        r.pconstruct(&p_invoke_lambda<DecayedF, DecayedP>, std::forward<F>(func), std::forward<P>(p));
      }
    }
    return r;
  }

  /** Bind method for a class member and its class instance along with an extra data, that is not passed to the
   * function, but can be later obtained from the packaged function object. The compressed pair must fit within the
   * SmallBuffer along with its alignment requirements.
   */
  template <auto M, typename P>
  static auto pbind(typename acl::member_function<M>::class_type& instance, P&& p) -> basic_delegate
  {
    using C = typename acl::member_function<M>::class_type;
    using F = typename acl::member_function<M>::function_type;
    basic_delegate r;
    r.pconstruct(&p_invoke_member_function<acl::member_function<M>, std::decay_t<P>>, &instance, std::forward<P>(p));
    return r;
  }

  /** Bind a free function along with an extra data, that is not passed to the
   * function, but can be later obtained from the packaged function object. The compressed pair must fit within the
   * SmallBuffer along with its alignment requirements.
   */
  template <auto F, typename P>
    requires(!acl::member_function<F>::is_member_function_traits)
  static auto pbind(P&& arg) -> basic_delegate
  {
    using DecayedP = std::decay_t<P>;
    basic_delegate r;

    r.pconstruct(&invoke_free_function_by_binding<F>, std::forward<P>(arg));
    return r;
  }

  template <typename F, typename P>
  static auto pbind(fnptr fn_ptr, F&& data, P&& p) -> basic_delegate
  {
    basic_delegate r;
    r.pconstruct(fn_ptr, std::forward<F>(data), std::forward<P>(p));
    return r;
  }

  template <typename... PArgs>
  auto args() const noexcept -> std::tuple<PArgs...> const&
  {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return *reinterpret_cast<std::tuple<PArgs...> const*>(buffer_ + sizeof(delegate_fn));
  }

  template <typename... PArgs>
  auto args() noexcept -> std::tuple<PArgs...>&
  {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return *reinterpret_cast<std::tuple<PArgs...>*>(buffer_ + sizeof(delegate_fn));
  }

  /** Get compressed pair's data */
  template <typename P>
  [[nodiscard]] auto get_compressed_data() const -> P const&
  {
    using CompressedPair = compressed_pair<P>;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto pair = reinterpret_cast<CompressedPair const*>(buffer_);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return *reinterpret_cast<P const*>(pair->data_);
  }

  explicit operator bool() const noexcept
  {
    return (*(delegate_fn*)buffer_) != nullptr;
  }

  auto operator=(std::nullptr_t) noexcept -> basic_delegate&
  {
    (*(delegate_fn*)buffer_) = nullptr;
    return *this;
  }

  // Invocation operator
  auto operator()(Args... args) -> Ret
  {
    assert(*(delegate_fn*)buffer_);
    return ((*(delegate_fn*)buffer_))(*this, std::forward<Args>(args)...);
  }
};

constexpr uint32_t max_delegate_base_size = 24;
template <typename V>
using delegate = basic_delegate<max_delegate_base_size, V>;

} // namespace acl
