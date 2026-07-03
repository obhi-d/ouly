#include "ouly/utility/utils.hpp"
#include <compare>

namespace ouly::detail
{
template <typename Agg, typename TupleType, bool IsConst>
class soavector_value_wrapper
{
public:
  using value_type = Agg;

  using pointer =
   std::conditional_t<IsConst, ouly::detail::tuple_of_cptrs<TupleType>, ouly::detail::tuple_of_ptrs<TupleType>>;

  soavector_value_wrapper() = default;

  explicit soavector_value_wrapper(pointer ptr) : pointer_(ptr) {}

  template <typename... Ptrs>
    requires(sizeof...(Ptrs) == std::tuple_size_v<TupleType>)
  explicit soavector_value_wrapper(Ptrs... ptrs) : pointer_(ptrs...)
  {}

  auto operator=(value_type const& value) -> soavector_value_wrapper&
    requires(!IsConst)
  {
    [&]<std::size_t... I>(std::index_sequence<I...>)
    {
      ((*std::get<I>(pointer_) = get_ref<I>(value)), ...);
    }(std::make_index_sequence<std::tuple_size_v<TupleType>>{});

    return *this;
  }

  operator value_type() const
  {
    return std::apply(
     []<typename... Ptrs>(Ptrs... ptrs) -> value_type
     {
       return value_type{(*ptrs)...};
     },
     pointer_);
  }

  auto get() const -> value_type
  {
    return static_cast<value_type>(*this);
  }

  template <std::size_t I>
  auto get() const -> decltype(auto)
  {
    return *std::get<I>(pointer_);
  }

  inline auto operator<=>(soavector_value_wrapper const&) const noexcept = default;

private:
  template <std::size_t I, typename T>
  static auto get_ref(T&& value) -> decltype(auto)
  {
    if constexpr (requires { ouly::detail::get_field_ref<I>(std::forward<T>(value)); })
    {
      return ouly::detail::get_field_ref<I>(std::forward<T>(value));
    }
    else
    {
      return std::get<I>(std::forward<T>(value));
    }
  }

  pointer pointer_{};
};


template <std::size_t I, typename Agg, typename TupleType, bool IsConst>
auto get(soavector_value_wrapper<Agg, TupleType, IsConst> const& value) -> decltype(auto)
{
  return value.template get<I>();
}
} // namespace ouly::detail


namespace std
{
template <typename Agg, typename TupleType, bool IsConst>
struct tuple_size<ouly::detail::soavector_value_wrapper<Agg, TupleType, IsConst>>
    : integral_constant<size_t, tuple_size_v<TupleType>>
{};

template <size_t I, typename Agg, typename TupleType, bool IsConst>
struct tuple_element<I, ouly::detail::soavector_value_wrapper<Agg, TupleType, IsConst>>
{
  using raw_type = tuple_element_t<I, TupleType>;

  using type = conditional_t<IsConst, raw_type const, raw_type>;
};

template <typename Agg, typename TupleType, bool IsConst, template <typename> typename TQual,
          template <typename> typename UQual>
struct basic_common_reference<ouly::detail::soavector_value_wrapper<Agg, TupleType, IsConst>, Agg, TQual, UQual>
{
  using type = Agg;
};

template <typename Agg, typename TupleType, bool IsConst, template <typename> typename TQual,
          template <typename> typename UQual>
struct basic_common_reference<Agg, ouly::detail::soavector_value_wrapper<Agg, TupleType, IsConst>, TQual, UQual>
{
  using type = Agg;
};
} // namespace std
