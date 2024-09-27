
#pragma once

#include <acl/utils/type_traits.hpp>
#include <cstdint>
#include <cstring>
#include <type_traits>

/**
 * Defines a basic_delegate object with a memory store to store a function pointer and its capture inline allocated.
 */
namespace acl
{

/**
 *
 * A basic_delegate can be constructed either by a lambda expression
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

  template <typename P>
  struct compressed_pair
  {
    static constexpr size_t SmallFunctorSize = SmallSize - sizeof(P);
    alignas(std::max_align_t) uint8_t functor[SmallFunctorSize];
    alignas(alignof(P)) uint8_t data[sizeof(P)];
  };

  // Helper to call free functions
  template <typename F>
  static Ret invoke_free_function_by_pointer(basic_delegate& d, Args&&... args)
  {
    // Retrieve the stored function and cast it back to its original type
    auto& func = *reinterpret_cast<F*>(d.buffer);
    return func(std::forward<Args>(args)...);
  }

  // Helper to call free functions
  template <auto F>
  static Ret invoke_free_function_by_binding(basic_delegate& d, Args&&... args)
  {
    // Retrieve the stored function and cast it back to its original type
    return F(std::forward<Args>(args)...);
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

  // Helper to call free functions
  template <typename F, typename P>
  static Ret invoke_free_function_by_pointer(basic_delegate& d, Args&&... args)
  {
    // Retrieve the stored function and cast it back to its original type
    auto& func = *reinterpret_cast<F*>(reinterpret_cast<compressed_pair<P>*>(d.buffer)->functor);

    return func(std::forward<Args>(args)...);
  }

  // Helper to call member functions
  template <typename M, typename P>
  static Ret invoke_member_function(basic_delegate& d, Args&&... args)
  {
    using C = typename M::class_type;
    using F = typename M::function_type;

    auto data = *reinterpret_cast<C**>(reinterpret_cast<compressed_pair<P>*>(d.buffer)->functor);
    return M::invoke(*data, std::forward<Args>(args)...);
  }

  // Helper to call lambdas or functors
  template <typename F, typename P>
  static Ret invoke_lambda(basic_delegate& d, Args&&... args)
  {
    auto& func = *reinterpret_cast<F*>(reinterpret_cast<compressed_pair<P>*>(d.buffer)->functor);
    return func(std::forward<Args>(args)...);
  }

public:
  // Default constructor
  basic_delegate() noexcept = default;

  // Move constructor
  basic_delegate(basic_delegate&& other) noexcept
  {
    std::memcpy(buffer, other.buffer, SmallSize);
    invoker       = other.invoker;
    other.invoker = nullptr;
  }

  /** Bind method for a functor or lambda like object */
  template <typename F>
  inline static basic_delegate bind(F&& func)
  {
    using DecayedF = std::decay_t<F>;
    static_assert(sizeof(DecayedF) <= SmallSize, "Function/Lamda object too large for inline storage.");
    static_assert(std::is_trivially_destructible_v<DecayedF> && std::is_trivially_copyable_v<DecayedF>,
                  "Capture type should be trivially copyable and destructible");

    basic_delegate r;
    new (r.buffer) DecayedF(std::forward<F>(func));
    if constexpr (std::is_function_v<DecayedF>)
      r.invoker = &invoke_free_function_by_pointer<DecayedF>;
    else
      r.invoker = &invoke_lambda<DecayedF>;
    return r;
  }

  /**
   * Bind method for a compressed pair of function + some data, that is not passed to the function, but can be later
   * obtained from the packaged function object. The compressed pair must fit within the SmallBuffer along with its
   * alignment requirements.
   */
  template <typename F, typename P>
  inline static basic_delegate bind(F&& func, P&& arg)
  {
    using DecayedF = std::decay_t<F>;
    using DecayedP = std::decay_t<P>;
    static_assert(sizeof(DecayedF) <= 20, "DecayedF is large");
    static_assert(sizeof(DecayedP) <= 4, "DecayedP is large");
    using CompressedPair = compressed_pair<DecayedP>;
    static_assert(sizeof(CompressedPair) <= SmallSize && CompressedPair::SmallFunctorSize >= sizeof(DecayedF),
                  "Function/Lamda object too large for inline storage.");
    static_assert(std::is_trivially_destructible_v<DecayedF> && std::is_trivially_copyable_v<DecayedF> &&
                    std::is_trivially_destructible_v<DecayedP> && std::is_trivially_copyable_v<DecayedP>,
                  "Capture/Parameter type should be trivially copyable and destructible");

    basic_delegate r;
    auto           pair = reinterpret_cast<CompressedPair*>(r.buffer);
    new (pair->functor) DecayedF(std::forward<F>(func));
    new (pair->data) DecayedP(arg);
    if constexpr (std::is_function_v<DecayedF>)
      r.invoker = &invoke_free_function_by_pointer<DecayedF, DecayedP>;
    else
      r.invoker = &invoke_lambda<DecayedF, DecayedP>;
    return r;
  }

  /** Bind method for a class member and its class instance */
  template <auto M>
  inline static basic_delegate bind(typename acl::member_function<M>::class_type& instance)
  {
    using C = typename acl::member_function<M>::class_type;
    using F = typename acl::member_function<M>::function_type;

    basic_delegate r;
    static_assert(sizeof(C*) <= SmallSize, "Member function object too large for inline storage.");
    *reinterpret_cast<C**>(r.buffer) = &instance;
    r.invoker                        = &invoke_member_function<acl::member_function<M>>;
    return r;
  }

  /** Bind method for a class member and its class instance along with an extra data, that is not passed to the
   * function, but can be later obtained from the packaged function object. The compressed pair must fit within the
   * SmallBuffer along with its alignment requirements.
   */
  template <auto M, typename P>
  inline static basic_delegate bind(typename acl::member_function<M>::class_type& instance, P&& arg)
  {
    using C              = typename acl::member_function<M>::class_type;
    using F              = typename acl::member_function<M>::function_type;
    using DecayedP       = std::decay_t<P>;
    using CompressedPair = compressed_pair<DecayedP>;
    basic_delegate r;
    static_assert(sizeof(CompressedPair) <= SmallSize && CompressedPair::SmallFunctorSize >= sizeof(C*),
                  "Function/Lamda object too large for inline storage.");
    static_assert(std::is_trivially_destructible_v<DecayedP> && std::is_trivially_copyable_v<DecayedP>,
                  "Capture/Parameter type should be trivially copyable and destructible");
    auto pair                             = reinterpret_cast<CompressedPair*>(r.buffer);
    *reinterpret_cast<C**>(pair->functor) = &instance;
    new (pair->data) DecayedP(arg);
    r.invoker = &invoke_member_function<acl::member_function<M>, DecayedP>;
    return r;
  }

  /** Bind a free function */
  template <auto F>
  inline static basic_delegate bind()
  {
    basic_delegate r;
    r.invoker = &invoke_free_function_by_binding<F>;
    return r;
  }

  /** Bind a free function along with an extra data, that is not passed to the
   * function, but can be later obtained from the packaged function object. The compressed pair must fit within the
   * SmallBuffer along with its alignment requirements.
   */
  template <auto F, typename P>
    requires(!acl::member_function<F>::is_member_function_traits)
  inline static basic_delegate bind(P&& arg)
  {
    using DecayedP       = std::decay_t<P>;
    using CompressedPair = compressed_pair<DecayedP>;
    basic_delegate r;
    static_assert(sizeof(CompressedPair) <= SmallSize, "Function/Lamda object too large for inline storage.");
    static_assert(std::is_trivially_destructible_v<DecayedP> && std::is_trivially_copyable_v<DecayedP>,
                  "Capture/Parameter type should be trivially copyable and destructible");
    auto pair = reinterpret_cast<CompressedPair*>(r.buffer);
    new (pair->data) DecayedP(arg);
    r.invoker = &invoke_free_function_by_binding<F>;
    return r;
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

  /** Get compressed pair's data */
  template <typename P>
  P const& get_compressed_data() const
  {
    using CompressedPair = compressed_pair<P>;
    auto pair            = reinterpret_cast<CompressedPair const*>(buffer);
    return *reinterpret_cast<P const*>(pair->data);
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

  inline basic_delegate& operator=(nullptr_t) noexcept
  {
    invoker = nullptr;
    return *this;
  }

  // Invocation operator
  Ret operator()(Args... args)
  {
    assert(invoker);
    return invoker(*this, std::forward<Args>(args)...);
  }
};

template <typename V>
using delegate = basic_delegate<24, V>;

} // namespace acl