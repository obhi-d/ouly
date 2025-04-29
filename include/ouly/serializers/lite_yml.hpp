// SPDX-License-Identifier: MIT

#pragma once
#include "ouly/dsl/lite_yml.hpp"
#include "ouly/serializers/detail/lite_yml_parser_context.hpp"
#include "ouly/serializers/detail/lite_yml_writer_context.hpp"
#include "ouly/serializers/serializers.hpp"
#include "ouly/utility/convert.hpp"

namespace ouly::yml
{

template <typename Class, typename Config = ouly::config<>>
auto to_string(Class const& obj) -> std::string
{
  auto state = ouly::detail::writer_state();
  write(state, obj);
  return state.get();
}

template <typename Class, typename Config = ouly::config<>>
void from_string(Class& obj, std::string_view data)
{
  auto in_context_impl = ouly::detail::in_context_impl<Class&, Config>(obj);
  {
    auto state = ouly::detail::parser_state(data);
    state.parse(in_context_impl);
  }
}
} // namespace ouly::yml
