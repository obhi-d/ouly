//
// Created by obhi on 9/18/20.
//
#pragma once

#include "string_literal.hpp"
#include <bit>
#include <concepts>
#include <cstdint>
#include <string>
#include <tuple>
#include <type_traits>
#include <variant>

namespace acl
{
template <typename C>
concept ClassWithReflect = requires { C::reflect(); };

// String transforms
template <typename T>
T& from_string(T& ref, std::string_view) = delete;

// String transforms
template <typename T>
T from_string(std::string_view) = delete;

template <typename T>
std::string to_string(T const& ref) = delete;

template <typename T>
std::string_view to_string_view(T const& ref) = delete;

template <ClassWithReflect Class>
auto reflect() noexcept
{
  return Class::reflect();
}

template <typename Class = void>
auto reflect() noexcept
{
  return std::tuple<>();
}

namespace detail
{
// Types
template <auto>
struct member_ptr_type;

template <typename T, typename M, M T::*P>
struct member_ptr_type<P>
{
  using class_t  = std::decay_t<T>;
  using member_t = std::decay_t<M>;
};

template <auto>
struct member_getter_type;

template <typename T, typename R, R (T::*MF)() const>
struct member_getter_type<MF>
{
  using class_t  = std::decay_t<T>;
  using return_t = R;
  using value_t  = std::decay_t<R>;
};

template <auto>
struct member_setter_type;

template <typename T, typename R, void (T::*MF)(R)>
struct member_setter_type<MF>
{
  using class_t  = std::decay_t<T>;
  using return_t = R;
  using value_t  = std::decay_t<R>;
};

template <auto>
struct free_getter_type;
template <auto>
struct getter_by_value_type;

template <typename T, typename R, R (*F)(T const&)>
struct free_getter_type<F>
{
  using class_t  = std::decay_t<T>;
  using return_t = R;
  using value_t  = std::decay_t<R>;
};

template <typename T, typename R, R (*F)(T)>
struct getter_by_value_type<F>
{
  using class_t  = std::decay_t<T>;
  using return_t = R;
  using value_t  = std::decay_t<R>;
};

template <auto>
struct free_setter_type;

template <typename T, typename R, void (*F)(T&, R)>
struct free_setter_type<F>
{
  using class_t  = std::decay_t<T>;
  using return_t = R;
  using value_t  = std::decay_t<R>;
};

// Concepts
// Decl
template <typename Class>
using bind_type = decltype(reflect<Class>());

// Utils
template <typename Class>
inline constexpr std::size_t tuple_size = std::tuple_size_v<bind_type<std::decay_t<Class>>>;

template <typename Class>
concept BoundClass = (tuple_size<Class>) > 0;

template <typename Class, typename Serializer>
concept InputSerializableClass = requires(Class& o, Serializer s) { s >> o; };

template <typename Class, typename Serializer>
concept OutputSerializableClass = requires(Class const& o, Serializer s) { s << o; };

template <auto MPtr>
concept IsMemberPtr = requires {
                        typename member_ptr_type<MPtr>::class_t;
                        typename member_ptr_type<MPtr>::member_t;
                      };

template <auto Getter, auto Setter>
concept IsMemberGetterSetter = requires {
                                 typename member_getter_type<Getter>::return_t;
                                 typename member_getter_type<Getter>::class_t;
                                 typename member_setter_type<Setter>::return_t;
                                 typename member_setter_type<Setter>::class_t;
                               };

template <auto Getter, auto Setter>
concept IsFreeGetterSetter = requires {
                               typename free_getter_type<Getter>::return_t;
                               typename free_getter_type<Getter>::class_t;
                               typename free_setter_type<Setter>::return_t;
                               typename free_setter_type<Setter>::class_t;
                             };

template <auto Getter, auto Setter>
concept IsFreeGetterByValSetter = requires {
                                    typename getter_by_value_type<Getter>::return_t;
                                    typename getter_by_value_type<Getter>::class_t;
                                    typename free_setter_type<Setter>::return_t;
                                    typename free_setter_type<Setter>::class_t;
                                  };

template <typename T>
using remove_cref = std::remove_const_t<std::remove_reference_t<T>>;

// Strings
template <typename T>
concept NativeStringLike =
  std::is_same_v<std::string, remove_cref<T>> || std::is_same_v<std::string_view, remove_cref<T>> ||
  std::is_same_v<std::string, remove_cref<T>> || std::is_same_v<char*, T> ||
  std::is_same_v<char const*, remove_cref<T>>;

// String type check

template <typename T>
concept SignedIntLike = std::is_signed_v<remove_cref<T>> &&
                        (std::is_integral_v<remove_cref<T>> || std::is_enum_v<remove_cref<T>>) &&
                        (!std::is_same_v<remove_cref<T>, bool>);

template <typename T>
concept UnsignedIntLike = std::is_unsigned_v<remove_cref<T>> &&
                          (std::is_integral_v<remove_cref<T>> || std::is_enum_v<remove_cref<T>>) &&
                          (!std::is_same_v<remove_cref<T>, bool>);

template <typename T>
concept IntegerLike = SignedIntLike<T> || UnsignedIntLike<T>;

// Float
template <typename T>
concept FloatLike = std::is_floating_point_v<remove_cref<T>>;

// Bool
template <typename T>
concept BoolLike = std::is_same_v<remove_cref<T>, bool>;

template <typename T>
concept StringLike = NativeStringLike<T>;

template <typename T>
concept NativeLike = BoolLike<T> || SignedIntLike<T> || UnsignedIntLike<T> || FloatLike<T> || StringLike<T>;

template <typename T>
concept CastableToStringView = requires(T t) { std::string_view(t); } && (!StringLike<T>);

template <typename T>
concept CastableToString = requires(T t) { std::string(t); } && (!CastableToStringView<T>) && (!StringLike<T>);

template <typename T>
concept ConvertibleToString = requires(T t) { std::to_string(t); } &&
                              (!FloatLike<T> && !BoolLike<T> && !SignedIntLike<T> && !UnsignedIntLike<T> &&
                               !CastableToString<T> && !StringLike<T>);

template <typename T>
concept TransformFromString = requires(T ref) {
                                {
                                  acl::from_string(ref, std::string_view())
                                  } -> std::same_as<T&>;
                                {
                                  acl::from_string<T>(std::string_view())
                                  } -> std::same_as<T>;
                              };

template <typename T>
concept TransformToString = requires(T ref) {
                              {
                                acl::to_string(ref)
                                } -> std::same_as<std::string>;
                            } && (!ConvertibleToString<T>) && (!StringLike<T>);

template <typename T>
concept TransformToStringView = requires(T ref) {
                                  {
                                    acl::to_string_view(ref)
                                    } -> std::same_as<std::string_view>;
                                } && (!TransformToString<T>) && (!StringLike<T>);

// Array
template <typename Class>
concept Itereable = requires(Class obj) {
                      (*std::begin(obj));
                      (*std::end(obj));
                    };

template <typename Class>
using array_value_type = std::decay_t<decltype(*std::begin(Class()))>;

template <typename Class>
concept HasValueType = requires(Class obj) { typename Class::value_type; };

// Map
template <typename Class>
concept ValuePairList = requires(Class obj) {
                          (*std::begin(obj)).first;
                          (*std::begin(obj)).second;
                          (*std::end(obj)).first;
                          (*std::end(obj)).second;
                        };

// Optional
template <typename Class>
concept OptionalLike = requires(Class o) {
                         typename Class::value_type;
                         o.emplace(std::declval<typename Class::value_type>());
                         o.has_value();
                         (bool)o;
                         {
                           o.has_value()
                           } -> std::same_as<bool>;
                         o.reset();
                         {
                           (*o)
                           } -> std::same_as<std::add_lvalue_reference_t<typename Class::value_type>>;
                         {
                           o.operator->()
                           } -> std::same_as<typename Class::value_type*>;
                       };

template <typename T>
concept ConstructedFromStringView = requires { T(std::string_view()); } && (!OptionalLike<T>);

template <typename T>
concept ConstructedFromString = requires { T(std::string()); } && (!OptionalLike<T>);

template <typename Class>
concept HasReserve = requires(Class obj) { obj.reserve(std::size_t()); };
template <typename Class>
concept HasResize = requires(Class obj) { obj.resize(std::size_t()); };

template <typename Class>
concept HasSize = requires(Class obj) {
                    {
                      obj.size()
                      } -> std::convertible_to<std::size_t>;
                  };

template <typename Class, typename ValueType>
concept HasEmplace = requires(Class obj, ValueType value) { obj.emplace(value); };

template <typename Class, typename ValueType>
concept HasPushBack = requires(Class obj, ValueType value) { obj.push_back(value); };

template <typename Class, typename ValueType>
concept HasEmplaceBack = requires(Class obj, ValueType value) { obj.emplace_back(value); };

template <typename Class, typename ValueType>
concept HasEmplaceFn =
  HasEmplace<Class, ValueType> || HasEmplaceBack<Class, ValueType> || HasPushBack<Class, ValueType>;

template <typename Class>
concept HasCapacity = requires(Class const& c) {
                        {
                          c.capacity()
                          } -> std::convertible_to<std::size_t>;
                      };

template <typename Class>
concept CanConstructFromString = detail::TransformFromString<Class> || detail::ConstructedFromStringView<Class> ||
                                 detail::ConstructedFromString<Class>;

template <typename Class>
concept MapLike = requires(Class t) {
                    typename Class::key_type;
                    typename Class::mapped_type;
                    typename Class::value_type;
                    {
                      t.begin()
                      } -> std::same_as<typename Class::iterator>;
                    {
                      t.end()
                      } -> std::same_as<typename Class::iterator>;
                    {
                      t.find(typename Class::key_type{})
                      } -> std::same_as<typename Class::iterator>;
                  };

template <typename Class>
concept StringMapLike = MapLike<Class> && StringLike<typename Class::key_type>;

template <HasValueType Class>
using container_value_type = typename Class::value_type;

// Pointers
template <typename Class>
concept IsSmartPointer = requires(Class o) {
                           typename Class::element_type;
                           (bool)o;
                           {
                             (*o)
                             } -> std::same_as<std::add_lvalue_reference_t<typename Class::element_type>>;
                           {
                             o.operator->()
                             } -> std::same_as<typename Class::element_type*>;
                         };

template <typename Class>
concept IsBasicPointer = std::is_pointer_v<Class> && (!NativeStringLike<Class>);

template <typename Class>
concept PointerLike = IsBasicPointer<Class> || IsSmartPointer<Class>;

template <typename T>
constexpr auto get_pointer_class_type()
{
  if constexpr (IsBasicPointer<T>)
    return std::decay_t<std::remove_pointer_t<remove_cref<T>>>();
  else if constexpr (IsSmartPointer<T>)
    return std::decay_t<remove_cref<typename T::element_type>>();
}

template <typename T>
using pointer_class_type = decltype(get_pointer_class_type<T>());

// Variant
template <typename Class>
concept VariantLike = requires(Class o) {
                        {
                          o.index()
                          } -> std::same_as<std::size_t>;
                      } && std::variant_size_v<Class> > 0;

template <typename Class>
concept HasArrayValueAssignable =
  Itereable<Class> && requires(Class a, std::size_t i) { a[i++] = std::declval<array_value_type<Class>>(); };

template <typename Class>
concept ContainerLike = (HasEmplaceFn<Class, array_value_type<Class>> || HasArrayValueAssignable<Class>) &&
                        (Itereable<Class> && !StringLike<Class>);

template <typename Class>
concept ArrayLike = ContainerLike<Class> && (!StringMapLike<Class>);

// Tuple
template <class T, std::size_t N>
concept HasTupleElement = requires(T t) {
                            typename std::tuple_element_t<N, std::remove_const_t<T>>;
                            {
                              get<N>(t)
                              } -> std::convertible_to<std::tuple_element_t<N, T> const&>;
                          };
template <typename Class>
concept TupleLike = (!ArrayLike<Class>) && requires(Class t) {
                                             typename std::tuple_size<Class>::type;
                                             []<std::size_t... N>(std::index_sequence<N...>)
                                             {
                                               return (HasTupleElement<Class, N> && ...);
                                             }
                                             (std::make_index_sequence<std::tuple_size_v<Class>>());
                                           };

template <typename Class>
concept MonostateLike = std::same_as<Class, std::monostate>;

// clang-format off
template <typename Class, typename Serializer>
concept LinearArrayLike =  requires(Class c) 
                           {
                               typename Class::value_type;
                               c.data();
                               { c.size() } -> std::convertible_to<std::size_t>;
                           } && 
                           std::is_standard_layout_v<typename Class::value_type> &&  
                           std::is_trivially_copyable_v<typename Class::value_type> &&
                           std::has_unique_object_representations_v<typename Class::value_type> && 
                           !BoundClass<typename Class::value_type> &&
                           !OutputSerializableClass<typename Class::value_type, Serializer> &&
                           !InputSerializableClass<typename Class::value_type, Serializer>;
// clang-format on

// @remarks
// Highly borrowed from
// https://github.com/eliasdaler/MetaStuff
template <fixed_string Name, typename Class, typename M>
class decl_base
{
public:
  using ClassTy = std::decay_t<Class>;
  using MemTy   = std::decay_t<M>;

  inline static constexpr std::uint32_t key_hash() noexcept
  {
    return Name.hash();
  }

  inline static constexpr std::string_view key() noexcept
  {
    return (std::string_view)Name;
  }
};

template <fixed_string Name, typename Class, typename MPtr, auto Ptr>
class decl_member_ptr : public decl_base<Name, Class, MPtr>
{
public:
  using super = decl_base<Name, Class, MPtr>;
  using M     = typename super::MemTy;

  inline static void value(Class& obj, M const& value) noexcept
  {
    obj.*Ptr = value;
  }

  inline static void value(Class& obj, M&& value) noexcept
  {
    obj.*Ptr = std::move(value);
  }

  inline static M const& value(Class const& obj) noexcept
  {
    return (obj.*Ptr);
  }
};

template <fixed_string Name, typename Class, typename RetTy, auto Getter, auto Setter>
class decl_get_set : public decl_base<Name, Class, RetTy>
{
public:
  using super = decl_base<Name, Class, RetTy>;
  using M     = typename super::MemTy;

  inline static void value(Class& obj, M&& value) noexcept
  {
    (obj.*Setter)(std::move(value));
  }

  inline static auto value(Class const& obj) noexcept
  {
    return ((obj.*Getter)());
  }
};

template <fixed_string Name, typename Class, typename RetTy, auto Getter, auto Setter>
class decl_free_get_set : public decl_base<Name, Class, RetTy>
{
public:
  using super = decl_base<Name, Class, RetTy>;
  using M     = typename super::MemTy;

  inline static void value(Class& obj, M const& value) noexcept
  {
    (*Setter)(obj, value);
  }

  inline static auto value(Class const& obj) noexcept
  {
    return (*Getter)(obj);
  }
};

// using ClassTy = std::decay_t<Class>;
// using MemTy   = std::decay_t<M>;
//
// constexpr decl_base(std::string_view iName) : name(iName) {}
//
// std::string_view key() const
// {
//   return name;
// }

template <typename T>
concept DeclBase = requires {
                     typename T::ClassTy;
                     typename T::MemTy;
                     {
                       T::key()
                       } -> std::same_as<std::string_view>;
                   };

template <typename Class>
auto const reflect_() noexcept
{
  return acl::reflect<Class>();
}

template <std::integral T>
constexpr T byteswap(T value) noexcept
{
  static_assert(std::has_unique_object_representations_v<T>, "T may not have padding bits");
  auto value_representation = std::bit_cast<std::array<std::byte, sizeof(T)>>(value);
  std::ranges::reverse(value_representation);
  return std::bit_cast<T>(value_representation);
}

} // namespace detail

/// @brief This function iterates over members registered by bind
/// @param fn A lambda accepting the instance of the object, a type info and the member index
///           The typ info has a value method to access the internal member given the instance of the object
/// @param obj  The instance of the class which has a reflection interface.
template <typename Class, typename Fn>
void for_each_field(Fn&& fn, Class& obj) noexcept
{
  using ClassType = std::remove_const_t<Class>;
  static_assert(detail::tuple_size<ClassType> > 0, "Invalid tuple size");
  return [&]<size_t... I>(std::index_sequence<I...>, auto tup)
  {
    (fn(obj, std::get<I>(tup), std::integral_constant<size_t, I>()), ...);
  }
  (std::make_index_sequence<detail::tuple_size<ClassType>>(), detail::reflect_<ClassType>());
}
/// @remarks Desired syntax would be
/// bind< bind<"MyField", &T::my_field>,

template <fixed_string Name, auto MPtr>
constexpr auto bind() noexcept
requires(detail::IsMemberPtr<MPtr>)
{
  return detail::decl_member_ptr<Name, typename detail::member_ptr_type<MPtr>::class_t,
                                 typename detail::member_ptr_type<MPtr>::member_t, MPtr>();
}

template <fixed_string Name, auto Getter, auto Setter>
constexpr auto bind() noexcept
requires(detail::IsMemberGetterSetter<Getter, Setter>)
{
  return detail::decl_get_set<Name, typename detail::member_getter_type<Getter>::class_t,
                              typename detail::member_getter_type<Getter>::return_t, Getter, Setter>();
}

template <fixed_string Name, auto Getter, auto Setter>
constexpr auto bind() noexcept
requires(detail::IsFreeGetterSetter<Getter, Setter>)
{
  return detail::decl_free_get_set<Name, typename detail::free_getter_type<Getter>::class_t,
                                   typename detail::free_getter_type<Getter>::return_t, Getter, Setter>();
}

template <fixed_string Name, auto Getter, auto Setter>
constexpr auto bind() noexcept
requires(detail::IsFreeGetterByValSetter<Getter, Setter>)
{
  return detail::decl_free_get_set<Name, typename detail::getter_by_value_type<Getter>::class_t,
                                   typename detail::getter_by_value_type<Getter>::return_t, Getter, Setter>();
}

template <detail::DeclBase... Args>
constexpr auto bind(Args&&... args) noexcept
{
  return std::make_tuple(std::forward<Args>(args)...);
}

} // namespace acl
