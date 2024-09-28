
#pragma once

#include <acl/utils/type_traits.hpp>
#include <acl/utils/tuple.hpp>

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

  using delegate_fn               = Ret (*)(basic_delegate&, Args...);
  static constexpr size_t BufferSize = sizeof(void*) + SmallSize;

  // Small object optimization buffer

  alignas(std::max_align_t) uint8_t buffer[BufferSize];

  template <typename P>
  struct compressed_pair
  {
    static constexpr size_t SmallFunctorSize = BufferSize - sizeof(P);
    alignas(std::max_align_t) uint8_t functor[SmallFunctorSize];
    alignas(alignof(P)) uint8_t data[sizeof(P)];
  };

  // Helper to call free functions
  template <typename F>
  static Ret invoke_free_function_by_pointer(basic_delegate& d, Args... args)
  {
    // Retrieve the stored function and cast it back to its original type
    auto& func = *reinterpret_cast<F*>(d.buffer + sizeof(delegate_fn));
    return func(std::forward<Args>(args)...);
  }

  // Helper to call free functions
  template <auto F>
  static Ret invoke_free_function_by_binding(basic_delegate& d, Args... args)
  {
    // Retrieve the stored function and cast it back to its original type
    return F(std::forward<Args>(args)...);
  }

  // Helper to call member functions
  template <typename M>
  static Ret invoke_member_function(basic_delegate& d, Args... args)
  {
    using C = typename M::class_type;
    using F = typename M::function_type;

    auto data = *reinterpret_cast<C**>(d.buffer + sizeof(delegate_fn));
    return M::invoke(*data, std::forward<Args>(args)...);
  }

  // Helper to call lambdas or functors
  template <typename F>
  static Ret invoke_lambda(basic_delegate& d, Args... args)
  {
    auto& func = *reinterpret_cast<F*>(d.buffer + sizeof(delegate_fn));
    return func(std::forward<Args>(args)...);
  }

  // Helper to call free functions
  template <typename F, typename P>
  static Ret p_invoke_free_function_by_pointer(basic_delegate& d, Args... args)
  {
    // Retrieve the stored function and cast it back to its original type
    auto& func = *reinterpret_cast<F*>(reinterpret_cast<compressed_pair<P>*>(d.buffer)->functor + sizeof(delegate_fn));

    return func(std::forward<Args>(args)...);
  }

  // Helper to call member functions
  template <typename M, typename P>
  static Ret p_invoke_member_function(basic_delegate& d, Args... args)
  {
    using C = typename M::class_type;
    using F = typename M::function_type;

    auto data = *reinterpret_cast<C**>(reinterpret_cast<compressed_pair<P>*>(d.buffer)->functor + sizeof(delegate_fn));
    return M::invoke(*data, std::forward<Args>(args)...);
  }

  // Helper to call lambdas or functors
  template <typename F, typename P>
  static Ret p_invoke_lambda(basic_delegate& d, Args... args)
  {
    auto& func = *reinterpret_cast<F*>(reinterpret_cast<compressed_pair<P>*>(d.buffer)->functor + sizeof(delegate_fn));
    return func(std::forward<Args>(args)...);
  }

  void construct(delegate_fn fn)
  {
    *(delegate_fn*)(buffer) = fn;
  }

  template <typename F>
  void construct(delegate_fn fn, F&& arg)
  {
    using DecayedF = std::decay_t<F>;
    static_assert(sizeof(DecayedF) <= SmallSize, "Function/Lamda object too large for inline storage.");
    static_assert(std::is_trivially_destructible_v<DecayedF> && std::is_trivially_copyable_v<DecayedF>,
                  "Capture type should be trivially copyable and destructible");

    *(delegate_fn*)(buffer) = fn;
    new (buffer + sizeof(delegate_fn)) DecayedF(std::forward<F>(arg));
  }

  template <typename P>
  void pconstruct(delegate_fn fn, P&& p)
  {
    using DecayedP = std::decay_t<P>;
    using CompressedPair = compressed_pair<DecayedP>;
    static_assert(sizeof(CompressedPair) <= BufferSize,
                  "Function/Lamda object too large for inline storage.");
    static_assert(std::is_trivially_destructible_v<DecayedP> && std::is_trivially_copyable_v<DecayedP>,
                  "Capture/Parameter type should be trivially copyable and destructible");

    auto pair = reinterpret_cast<compressed_pair<P>*>(buffer);
    *(delegate_fn*)(pair->functor) = fn;
    new (pair->data) DecayedP(std::forward<P>(p));
  }

  template <typename F, typename P>
  void pconstruct(delegate_fn fn, F&& arg, P&& p)
  {
    using DecayedF = std::decay_t<F>;
    using DecayedP = std::decay_t<P>;
    using CompressedPair = compressed_pair<DecayedP>;
    static_assert(sizeof(CompressedPair) <= BufferSize && CompressedPair::SmallFunctorSize >= (sizeof(delegate_fn)+sizeof(DecayedF)),
                  "Function/Lamda object too large for inline storage.");
    static_assert(std::is_trivially_destructible_v<DecayedF> && std::is_trivially_copyable_v<DecayedF> &&
                    std::is_trivially_destructible_v<DecayedP> && std::is_trivially_copyable_v<DecayedP>,
                  "Capture/Parameter type should be trivially copyable and destructible");

    auto pair = reinterpret_cast<compressed_pair<P>*>(buffer);
    *(delegate_fn*)(pair->functor) = fn;
    new (pair->functor + sizeof(delegate_fn)) DecayedF(std::forward<F>(arg));
    new (pair->data) DecayedP(std::forward<P>(p));
  }

public:
  using fnptr = delegate_fn;

  // Default constructor
  basic_delegate() noexcept = default;

  // Move constructor
  basic_delegate(basic_delegate&& other) noexcept
  {
    std::memcpy(buffer, other.buffer, BufferSize);
    *(delegate_fn*)other.buffer = nullptr;
  }

  // Move assignment operator
  basic_delegate& operator=(basic_delegate&& other) noexcept
  {
    if (this != &other)
    {
      std::memcpy(buffer, other.buffer, BufferSize);
      *(delegate_fn*)other.buffer = nullptr;
    }
    return *this;
  }

  // Move constructor
  basic_delegate(basic_delegate const& other) noexcept
  {
    std::memcpy(buffer, other.buffer, BufferSize);
  }

  // Move assignment operator
  basic_delegate& operator=(basic_delegate const& other) noexcept
  {
    if (this != &other)
    {
      std::memcpy(buffer, other.buffer, BufferSize);
    }
    return *this;
  }

  /** Bind method for a functor or lambda like object */
  template <typename F>
  inline static basic_delegate bind(F&& func)
  {
    using DecayedF = std::decay_t<F>;
    basic_delegate r;
    if constexpr (std::is_function_v<DecayedF>)
    {
      if constexpr (std::is_assignable_v<fnptr&, DecayedF>)
        r.construct(func);
      else
        r.construct(&invoke_free_function_by_pointer<DecayedF>, std::forward<F>(func));
    }
    else
    {
      if constexpr (std::is_assignable_v<fnptr&, DecayedF>)
        r.construct(func);
      else
        r.construct(&invoke_lambda<DecayedF>, std::forward<F>(func));
    }
    return r;
  }

  /** Bind method for a class member and its class instance */
  template <auto M>
  inline static basic_delegate bind(typename acl::member_function<M>::class_type& instance)
  {
    using C = typename acl::member_function<M>::class_type;
    using F = typename acl::member_function<M>::function_type;

    basic_delegate r;
    r.construct(&invoke_member_function<acl::member_function<M>>, &instance);
    return r;
  }

  /** Bind a free function */
  template <auto F>
  inline static basic_delegate bind()
  {
    basic_delegate r;
    r.construct(&invoke_free_function_by_binding<F>);
    return r;
  }

  template <typename L, typename F>
  inline static basic_delegate bind(L&& func, F&& data)
  {
    basic_delegate r;
    using DecayedL = std::decay_t<L>;
    static_assert(std::is_assignable_v<fnptr&, DecayedL>, "Can only accept function pointer");
    r.construct(func, std::forward<F>(data));
    return r;
  }

  /**
   * Bind method for a compressed pair of function + some data, that is not passed to the function, but can be later
   * obtained from the packaged function object. The compressed pair must fit within the SmallBuffer along with its
   * alignment requirements.
   */
  template <typename F, typename P>
  inline static basic_delegate pbind(F&& func, P&& p)
  {
    using DecayedF = std::decay_t<F>;
    using DecayedP = std::decay_t<P>;

    basic_delegate r;
    if constexpr (std::is_function_v<DecayedF>)
    {
      if constexpr (std::is_assignable_v<fnptr&, DecayedF>)
        r.pconstruct(func, std::forward<P>(p));
      else
        r.pconstruct(&p_invoke_free_function_by_pointer<DecayedF, DecayedP>, std::forward<F>(func), std::forward<P>(p));
    }
    else
    {
      if constexpr (std::is_assignable_v<fnptr&, DecayedF>)
        r.pconstruct(func, std::forward<P>(p));
      else
        r.pconstruct(&p_invoke_lambda<DecayedF, DecayedP>, std::forward<F>(func), std::forward<P>(p));
    }
    return r;
  }

  /** Bind method for a class member and its class instance along with an extra data, that is not passed to the
   * function, but can be later obtained from the packaged function object. The compressed pair must fit within the
   * SmallBuffer along with its alignment requirements.
   */
  template <auto M, typename P>
  inline static basic_delegate pbind(typename acl::member_function<M>::class_type& instance, P&& p)
  {
    using C              = typename acl::member_function<M>::class_type;
    using F              = typename acl::member_function<M>::function_type;
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
  inline static basic_delegate pbind(P&& arg)
  {
    using DecayedP       = std::decay_t<P>;
    basic_delegate r;

    r.pconstruct(&invoke_free_function_by_binding<F>, std::forward<P>(arg));
    return r;
  }

  template <typename F, typename P>
  inline static basic_delegate pbind(fnptr fn_ptr, F&& data, P&& p)
  {
    basic_delegate r;
    r.pconstruct(fn_ptr, std::forward<F>(data), std::forward<P>(p));
    return r;
  }

  template <typename...PArgs>
  acl::tuple<PArgs...> const& args() const noexcept
  {
    return *reinterpret_cast<acl::tuple<PArgs...> const*>(buffer + sizeof(delegate_fn));
  }

  template <typename...PArgs>
  acl::tuple<PArgs...>& args() noexcept
  {
    return *reinterpret_cast<acl::tuple<PArgs...>*>(buffer + sizeof(delegate_fn));
  }

  /** Get compressed pair's data */
  template <typename P>
  P const& get_compressed_data() const
  {
    using CompressedPair = compressed_pair<P>;
    auto pair            = reinterpret_cast<CompressedPair const*>(buffer);
    return *reinterpret_cast<P const*>(pair->data);
  }

  inline explicit operator bool() const noexcept
  {
    return (*(delegate_fn*)buffer) != nullptr;
  }

  inline basic_delegate& operator=(std::nullptr_t) noexcept
  {
    (*(delegate_fn*)buffer) = nullptr;
    return *this;
  }


  // Invocation operator
  Ret operator()(Args... args)
  {
    assert(*(delegate_fn*)buffer);
    return ((*(delegate_fn*)buffer))(*this, std::forward<Args>(args)...);
  }
};

template <typename V>
using delegate = basic_delegate<24, V>;

} // namespace acl
