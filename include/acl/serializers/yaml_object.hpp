
#pragma once
#include <acl/dsl/yaml.hpp>
#include <acl/utils/error_codes.hpp>
#include <acl/utils/type_traits.hpp>

namespace acl::yaml
{
namespace detail
{
template <typename T>
class context : public acl::yaml::context
{
public:
  context(T& obj, istream& buff) noexcept : obj_(obj), buffer_(buff) {}

  void start_mapping(std::string_view key) override {}

private:
  istream& buffer_;
  T&       obj_;
};
} // namespace detail

class object
{
public:
  object(std::string_view source, std::string_view content) : buffer_(source, content) {}

  template <typename Class>
  inline auto& operator>>(Class& obj)
  {}

private:
  istream buffer_;
};

} // namespace acl::yaml
