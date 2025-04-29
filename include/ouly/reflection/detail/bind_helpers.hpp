// SPDX-License-Identifier: MIT
#pragma once

#include "ouly/reflection/detail/deduced_types.hpp"
#include "ouly/utility/string_literal.hpp"

/**
 * @brief Helper classes for binding member pointers and getter/setter functions.
 *
 * This namespace provides utilities for working with member pointers and
 * getter/setter functions, including caching keys and managing offsets.
 */
namespace ouly::detail
{
template <string_literal Name, typename Class, typename M>
class decl_base
{
public:
  using ClassTy = std::decay_t<Class>;
  using MemTy   = std::decay_t<M>;

  static constexpr auto key_hash() noexcept -> std::uint32_t
  {
    return Name.hash();
  }

  static constexpr auto key() noexcept -> std::string_view
  {
    return static_cast<std::string_view>(Name);
  }

  template <typename TransformType>
  static auto cache_key() noexcept -> std::string_view
  {
    static auto key_str = std::string{TransformType::transform(key())};
    return key_str;
  }
};

template <string_literal Name, typename Class, typename MPtr, auto Ptr>
class decl_member_ptr : public decl_base<Name, Class, MPtr>
{
public:
  using super = decl_base<Name, Class, MPtr>;
  using M     = typename super::MemTy;

  static void value(Class& obj, M const& value) noexcept
  {
    obj.*Ptr = value;
  }

  static void value(Class& obj, M&& value) noexcept
  {
    obj.*Ptr = std::move(value);
  }

  static auto value(Class const& obj) noexcept -> M const&
  {
    return (obj.*Ptr);
  }

  static auto offset(Class const& obj) noexcept -> M const*
  {
    return &(obj.*Ptr);
  }

  static auto offset(Class& obj) noexcept -> M*
  {
    return &(obj.*Ptr);
  }
};

template <string_literal Name, typename Class, typename RetTy, auto Getter, auto Setter>
class decl_get_set : public decl_base<Name, Class, RetTy>
{
public:
  using super = decl_base<Name, Class, RetTy>;
  using M     = typename super::MemTy;

  static void value(Class& obj, M&& value) noexcept
  {
    (obj.*Setter)(std::move(value));
  }

  static auto value(Class const& obj) noexcept
  {
    return ((obj.*Getter)());
  }
};

template <string_literal Name, typename Class, typename RetTy, auto Getter, auto Setter>
class decl_free_get_set : public decl_base<Name, Class, RetTy>
{
public:
  using super = decl_base<Name, Class, RetTy>;
  using M     = typename super::MemTy;

  static void value(Class& obj, M const& value) noexcept
  {
    (*Setter)(obj, value);
  }

  static auto value(Class const& obj) noexcept
  {
    return (*Getter)(obj);
  }
};

} // namespace ouly::detail