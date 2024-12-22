
#include <acl/dsl/yaml.hpp>

namespace acl::yaml
{

void istream::set_idention_level(uint16_t level)
{
  current_indent_level = level;
}

void istream::set_idention_level(string_slice level)
{
  current_indent_level = level.count;
}

void istream::add_dash_indention(string_slice dash_level)
{
  current_indent_level += dash_level.count;
}

void istream::close_all_mappings()
{
  if (!handler)
    return;
  while (indent_stack.empty())
  {
    switch (indent_stack.back())
    {
    case indent_type::object:
      handler->end_object();
      break;
    case indent_type::array:
      handler->end_array();
      break;
    }
    indent_stack.pop_back();
  }
}

} // namespace acl::yaml
