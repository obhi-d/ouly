
#pragma once
#include <acl/dsl/lite_yml.hpp>
#include <acl/serializers/detail/lite_yml_parser_context.hpp>
#include <acl/serializers/detail/lite_yml_writer_context.hpp>
#include <acl/serializers/serializers.hpp>
#include <acl/utility/transforms.hpp>

namespace acl::yml
{

template <typename Class, typename Config = acl::config<>>
auto to_string(Class const& obj) -> std::string
{
  auto state = acl::detail::writer_state();
  write(state, obj);
  return state.get();
}

template <typename Class, typename Config = acl::config<>>
void from_string(Class& obj, std::string_view data)
{
  auto in_context_impl = acl::detail::in_context_impl<Class&, Config>(obj);
  {
    auto state = acl::detail::parser_state(data);
    state.parse(in_context_impl);
  }
}
} // namespace acl::yml
