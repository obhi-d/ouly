
#include "parameter.hpp"
#include "scli.hpp"
#include <charconv>

namespace acl
{

template <typename T>
T convert_to(std::string_view value, T default_value)
{
  T    result = 0;
  auto r      = std::from_chars(value.data(), value.data() + value.size(), result);
  if (r.ec == std::errc::invalid_argument)
    return default_value;
  return result;
}

default_parameter const* default_parameter::get_instance()
{
  static default_parameter param;
  return &param;
}

int64_t default_parameter::as_int64(int64_t default_value) const noexcept
{
  return default_value;
}

uint64_t default_parameter::as_uint64(uint64_t default_value) const noexcept
{
  return default_value;
}

float default_parameter::as_float(float default_value) const noexcept
{
  return default_value;
}

double default_parameter::as_double(double default_value) const noexcept
{
  return default_value;
}

bool default_parameter::as_bool(bool default_value) const noexcept
{
  return default_value;
}

parameter const* default_parameter::at(uint32_t i) const noexcept
{
  return this;
}

parameter const* default_parameter::find(std::string_view name) const noexcept
{
  return this;
}

std::string default_parameter::to_string() const noexcept
{
  return {};
}

int64_t parameter_value::as_int64(int64_t default_value) const noexcept
{
  return convert_to<int64_t>(param_value, default_value);
}

uint64_t parameter_value::as_uint64(uint64_t default_value) const noexcept
{
  return convert_to<uint64_t>(param_value, default_value);
}

float parameter_value::as_float(float default_value) const noexcept
{
  return convert_to<float>(param_value, default_value);
}

double parameter_value::as_double(double default_value) const noexcept
{
  return convert_to<double>(param_value, default_value);
}

bool parameter_value::as_bool(bool default_value) const noexcept
{
  if (param_value == "true")
    return true;
  else if (param_value == "false")
    return false;
  else
    return default_value;
}

parameter const* parameter_value::at(uint32_t i) const noexcept
{
  return default_parameter::get_instance();
}

parameter const* parameter_value::find(std::string_view name) const noexcept
{
  return default_parameter::get_instance();
}

std::string parameter_value::to_string() const noexcept
{
  return name().empty() ? std::string{param_value} : std::string(name()) + " = \"" + param_value + "\" ";
}

int64_t parameter_list::as_int64(int64_t default_value) const noexcept
{
  if (!param_value.empty())
    return param_value[0]->as_int64(default_value);
  return default_value;
}

uint64_t parameter_list::as_uint64(uint64_t default_value) const noexcept
{
  if (!param_value.empty())
    return param_value[0]->as_uint64(default_value);
  return default_value;
}

float parameter_list::as_float(float default_value) const noexcept
{
  if (!param_value.empty())
    return param_value[0]->as_float(default_value);
  return default_value;
}

double parameter_list::as_double(double default_value) const noexcept
{
  if (!param_value.empty())
    return param_value[0]->as_double(default_value);
  return default_value;
}

bool parameter_list::as_bool(bool default_value) const noexcept
{
  if (!param_value.empty())
    return param_value[0]->as_bool(default_value);
  return default_value;
}

parameter const* parameter_list::at(uint32_t i) const noexcept
{
  if (i < param_value.size())
    return param_value[i].get();
  return default_parameter::get_instance();
}

parameter const* parameter_list::find(std::string_view name) const noexcept
{
  for (auto const& p : param_value)
  {
    if (p->name() == name)
      return p.get();
  }

  return default_parameter::get_instance();
}

std::string parameter_list::to_string() const noexcept
{
  return to_string(true);
}

std::string parameter_list::to_string(bool wt) const noexcept
{
  std::string value;
  if (!name().empty())
  {
    value += name();
    value += " = ";
  }

  if (wt)
    value += "[ ";
  bool first = true;
  for (auto const& v : param_value)
  {
    if (!first)
      value += ", ";
    value += v->to_string();
    first = false;
  }
  if (wt)
    value += " ]";
  return value;
}

std::string parameter_main::to_string() const noexcept
{
  return parameter_list::to_string(false);
}

} // namespace acl