#pragma once

#include <acl/reflection/detail/accessors.hpp>
#include <acl/reflection/transforms.hpp>
#include <variant>

namespace acl::detail
{

template <typename C>
concept ClassWithReflect = requires { C::reflect(); };

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

// Strings
template <typename T>
concept NativeStringLike =
 std::is_same_v<std::string, std::decay_t<T>> || std::is_same_v<std::string_view, std::decay_t<T>> ||
 std::is_same_v<std::string, std::decay_t<T>> || std::is_same_v<char*, T> ||
 std::is_same_v<char const*, std::decay_t<T>>;

template <typename T>
concept NativeWStringLike =
 std::is_same_v<std::wstring, std::decay_t<T>> || std::is_same_v<std::wstring_view, std::decay_t<T>> ||
 std::is_same_v<std::wstring, std::decay_t<T>> || std::is_same_v<wchar_t*, T> ||
 std::is_same_v<wchar_t const*, std::decay_t<T>>;

// String type check

template <typename T>
concept SignedIntLike =
 std::is_signed_v<std::decay_t<T>> && (std::is_integral_v<std::decay_t<T>> || std::is_enum_v<std::decay_t<T>>) &&
 (!std::is_same_v<std::decay_t<T>, bool>);

template <typename T>
concept UnsignedIntLike =
 std::is_unsigned_v<std::decay_t<T>> && (std::is_integral_v<std::decay_t<T>> || std::is_enum_v<std::decay_t<T>>) &&
 (!std::is_same_v<std::decay_t<T>, bool>);

template <typename T>
concept IntegerLike = SignedIntLike<T> || UnsignedIntLike<T>;

template <typename T>
concept EnumLike = std::is_enum_v<T>;

// Float
template <typename T>
concept FloatLike = std::is_floating_point_v<std::decay_t<T>>;

// Bool
template <typename T>
concept BoolLike = std::is_same_v<std::decay_t<T>, bool>;

template <typename T>
concept ContainerIsStringLike = NativeStringLike<T>;

template <typename T>
concept WStringLike = NativeWStringLike<T>;

template <typename T>
concept NativeLike = BoolLike<T> || SignedIntLike<T> || UnsignedIntLike<T> || FloatLike<T> || ContainerIsStringLike<T>;

template <typename T>
concept CastableToStringView = requires(T t) { std::string_view(t); } && (!ContainerIsStringLike<T>);

template <typename T>
concept CastableToString =
 requires(T t) { std::string(t); } && (!CastableToStringView<T>) && (!ContainerIsStringLike<T>);

template <typename T>
concept ConvertibleToString =
 requires(T t) { std::to_string(t); } && (!FloatLike<T> && !BoolLike<T> && !SignedIntLike<T> && !UnsignedIntLike<T> &&
                                          !CastableToString<T> && !ContainerIsStringLike<T>);

template <typename T>
concept TransformFromString = requires(T ref) {
  { acl::from_string(ref, std::string_view()) } -> std::same_as<void>;
};

template <typename T>
concept TransformToString = requires(T ref) {
  { acl::to_string(ref) } -> std::same_as<std::string>;
};

template <typename T>
concept TransformToStringView = requires(T ref) {
  { acl::to_string_view(ref) } -> std::same_as<std::string_view>;
};

// Array
template <typename Class>
concept ContainerIsIterable = requires(Class obj) {
  (*std::begin(obj));
  (*std::end(obj));
};

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
  { o.has_value() } -> std::same_as<bool>;
  o.reset();
  { (*o) } -> std::same_as<std::add_lvalue_reference_t<typename Class::value_type>>;
  { o.operator->() } -> std::same_as<typename Class::value_type*>;
};

template <typename T>
concept ConstructedFromStringView = requires { T(std::string_view()); } && (!OptionalLike<T>);

template <typename T>
concept ConstructedFromString = requires { T(std::string()); } && (!OptionalLike<T>);

template <typename Class>
concept HasReserve = requires(Class obj) { obj.reserve(std::size_t()); };
template <typename Class>
concept HasResize = requires(Class obj) { obj.resize(std::uint32_t()); };

template <typename Class>
concept HasSize = requires(Class obj) {
  { obj.size() } -> std::convertible_to<std::size_t>;
};

template <typename Class, typename ValueType>
concept HasEmplace = requires(Class obj, ValueType value) { obj.emplace(value); };

template <typename Class, typename ValueType>
concept HasPushBack = requires(Class obj, ValueType value) { obj.push_back(value); };

template <typename Class, typename ValueType>
concept HasEmplaceBack = requires(Class obj, ValueType value) { obj.emplace_back(value); };

template <typename Class>
concept HasCapacity = requires(Class const& c) {
  { c.capacity() } -> std::convertible_to<std::size_t>;
};

template <typename Class>
concept StringLike = TransformFromString<Class> && (TransformToString<Class> || TransformToStringView<Class>);

template <typename Class>
concept MapLike = requires(Class t) {
  typename Class::key_type;
  typename Class::mapped_type;
  typename Class::value_type;
  { t.begin() } -> std::same_as<typename Class::iterator>;
  { t.end() } -> std::same_as<typename Class::iterator>;
  { t.find(typename Class::key_type{}) } -> std::same_as<typename Class::iterator>;
};

template <typename Class>
concept StringMapLike = MapLike<Class> && StringLike<typename Class::key_type>;

template <typename Class>
concept StringMapValueType = requires { typename Class::is_string_map_value_type; };

template <typename Class>
concept ComplexMapLike = MapLike<Class> && !StringLike<typename Class::key_type>;

// Pointers
template <typename Class>
concept IsSmartPointer = requires(Class o) {
  typename Class::element_type;
  (bool)o;
  { (*o) } -> std::same_as<std::add_lvalue_reference_t<typename Class::element_type>>;
  { o.operator->() } -> std::same_as<typename Class::element_type*>;
};

template <typename Class>
concept IsBasicPointer =
 std::is_pointer_v<Class> && (!std::is_same_v<char*, Class> && !std::is_same_v<char const*, Class>) &&
 (!std::is_same_v<wchar_t*, Class> && !std::is_same_v<wchar_t const*, Class>);

template <typename Class>
concept PointerLike = IsBasicPointer<Class> || IsSmartPointer<Class>;

// Variant
template <typename Class>
concept VariantLike = requires(Class o) {
  { o.index() } -> std::same_as<std::size_t>;
} && std::variant_size_v<Class> > 0;

template <typename Class>
concept ContainerHasArrayValueAssignable =
 ContainerIsIterable<Class> && requires(Class a, std::size_t i) { a[i++] = std::declval<array_value_type<Class>>(); };

template <typename Class>
concept ContainerHasEmplace = requires {
  typename Class::value_type;
  std::declval<Class>().emplace(std::declval<typename Class::value_type>());
};

template <typename Class>
concept ContainerHasPushBack = requires {
  typename Class::value_type;
  std::declval<Class>().push_back(std::declval<typename Class::value_type>());
};

template <typename Class>
concept ContainerHasEmplaceBack = requires {
  typename Class::value_type;
  std::declval<Class>().emplace_back(std::declval<typename Class::value_type>());
};

template <typename Class>
concept ContainerCanAppendValue =
 ContainerHasEmplace<Class> || ContainerHasEmplaceBack<Class> || ContainerHasPushBack<Class>;

template <typename Class>
concept ContainerLike = (ContainerCanAppendValue<Class> || ContainerHasArrayValueAssignable<Class>) &&
                        (ContainerIsIterable<Class> && !ContainerIsStringLike<Class>);

template <typename Class>
concept ArrayLike = ContainerLike<Class> && (!MapLike<Class>);

// Tuple
template <class T, std::size_t N>
concept HasTupleElement = requires(T t) {
  typename std::tuple_element_t<N, std::remove_const_t<T>>;
  { get<N>(t) } -> std::convertible_to<std::tuple_element_t<N, T> const&>;
};
template <typename Class>
concept TupleLike = (!ArrayLike<Class>) && requires(Class t) {
  typename std::tuple_size<Class>::type;
  []<std::size_t... N>(std::index_sequence<N...>)
  {
    return (HasTupleElement<Class, N> && ...);
  }(std::make_index_sequence<std::tuple_size_v<Class>>());
};

template <typename Class, typename Serializer>
concept LinearArrayLike =
 requires(Class c) {
   typename Class::value_type;
   c.data();
   { c.size() } -> std::convertible_to<std::size_t>;
 } && std::is_standard_layout_v<typename Class::value_type> &&
 std::is_trivially_copyable_v<typename Class::value_type> &&
 std::has_unique_object_representations_v<typename Class::value_type> && !BoundClass<typename Class::value_type> &&
 !OutputSerializableClass<typename Class::value_type, Serializer> &&
 !InputSerializableClass<typename Class::value_type, Serializer>;

template <typename T>
concept DeclBase = requires {
  typename T::ClassTy;
  typename T::MemTy;
  { T::key() } -> std::same_as<std::string_view>;
};
template <typename T>
concept HasKeyFieldName = requires(T t) { typename T::key_field_name_t; };

template <typename T>
concept HasTypeFieldName = requires(T t) { typename T::type_field_name_t; };

template <typename T>
concept HasValueFieldName = requires(T t) { typename T::value_field_name_t; };

template <typename T>
concept HasVariantTypeTransform = requires {
  acl::to_variant_index<T>(std::string_view());
  { acl::from_variant_index<T>(std::size_t()) } -> std::same_as<std::string_view>;
};

template <typename Class>
concept MonostateLike = std::same_as<Class, std::monostate>;

} // namespace acl::detail