#pragma once
//
// Created by obhi on 9/18/20.
//
#pragma once

#include <acl/reflection/reflect.hpp>

/**
 * @file reflection.hpp
 * @brief Provides compile-time reflection utilities for user-defined types.
 *
 * This header defines various concepts, utility types, and functions intended to enable
 * reflection of struct members, free functions, and class accessors. It is designed to
 * serve as a customization point for types, allowing them to be:
 *   - Bound for static introspection via @c bind(...)
 *   - Serialized, deserialized, or otherwise processed based on compile-time type properties
 *
 * @section Concepts
 *  - ClassWithReflect: Requires a static @c reflect() method on the class.
 *  - BoundClass: Checks if a type has a non-empty tuple of reflections.
 *  - InputSerializableClass / OutputSerializableClass: Requires compiler checks against
 *    user-defined serializers.
 *  - String- and number-related concepts: Determine if a type is @c BoolLike, @c IntegerLike,
 *    @c FloatLike, etc. These allow specialized handling for native types or custom transformations.
 *  - PointerLike and SmartPointer: Distinguish between raw pointer and smart-pointer types.
 *  - VariantLike: Allows identification of types that behave like @c std::variant.
 *  - ContainerLike / MapLike / ArrayLike / OptionalLike: Identifies various container types
 *    and how they might be iterated, indexed, or mutated.
 *
 * @section Functions_and_Utilities
 *
 *  - @c reflect(): Primary entry point to retrieve a tuple of reflective declarations
 *    for a given class. Offers customization points to provide either default (empty)
 *    reflection or user-defined reflection via a class's static @c reflect() method.
 *
 *  - @c for_each_field(...):
 *    @code{.cpp}
 *    template <typename Class, typename Fn>
 *    void for_each_field(Fn&& fn, Class& obj) noexcept
 *    @endcode
 *    Iterates over each reflected member of a class, calling a provided callable that
 *    receives both the class instance and descriptive reflection info about that field.
 *
 *  - @c field_at():
 *    @code{.cpp}
 *    template <typename Class, std::size_t I>
 *    constexpr auto field_at() noexcept
 *    @endcode
 *    Retrieves the reflection info object for a specific field by index.
 *
 *  - @c field_size():
 *    @code{.cpp}
 *    template <typename ClassType>
 *    constexpr uint32_t field_size() noexcept
 *    @endcode
 *    Returns the number of reflected fields in the given type.
 *
 *  - @c bind():
 *    These are helper functions for creating reflection declarations. Overloads exist
 *    to bind direct member pointers, member access via getter/setter, or free functions
 *    that behave like property accessors. They return small wrapper objects that can
 *    be used to build a reflection tuple.
 *
 * @section Usage_Example
 *
 * 1) In your type, define a static @c reflect() that returns @c bind(...) of each field:
 *    @code{.cpp}
 *    struct MyClass {
 *      int value;
 *      static constexpr auto reflect() {
 *        return acl::bind(
 *          acl::bind<"value", &MyClass::value>()
 *        );
 *      }
 *    };
 *    @endcode
 *
 * 2) Use @c for_each_field(...) to apply actions on each reflected field:
 *    @code{.cpp}
 *    MyClass instance{42};
 *    acl::for_each_field([](auto& obj, auto& decl, auto) {
 *      std::cout << decl.key() << " = " << decl.value(obj) << std::endl;
 *    }, instance);
 *    @endcode
 *
 * This customization point is particularly useful for serialization, GUI binding,
 * or any scenario requiring compile-time inspection of user-defined type members.
 */

// @remarks
// Highly borrowed from
// https://github.com/eliasdaler/MetaStuff

// using ClassTy = std::decay_t<Class>;
// using MemTy   = std::decay_t<M>;
//
// constexpr decl_base(std::string_view iName) : name(iName) {}
//
// std::string_view key() const
// {
//   return name;
// }
