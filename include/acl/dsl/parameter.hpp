
#pragma once

#include <acl/utils/config.hpp>
#include <memory>
#include <string_view>
#include <variant>
#include <vector>

namespace acl
{

class parameter
{
public:
  parameter() noexcept = default;
  inline parameter(std::string_view name) noexcept : param_name(name) {}

  inline std::string_view name() const noexcept
  {
    return param_name;
  }

  virtual int64_t          as_int64(int64_t default_value) const noexcept           = 0;
  virtual uint64_t         as_uint64(uint64_t default_value) const noexcept         = 0;
  virtual float            as_float(float default_value) const noexcept             = 0;
  virtual double           as_double(double default_value) const noexcept           = 0;
  virtual bool             as_bool(bool default_value) const noexcept               = 0;
  virtual std::string_view as_sv(std::string_view default_value) const noexcept     = 0;
  virtual std::string      as_string(std::string_view default_value) const noexcept = 0;
  /**
   * @brief Find a parameter by parameter position
   * @note This function will never return nullptr, it is safe to call any function over the pointer returned
   */
  virtual parameter const* at(uint32_t i) const noexcept = 0;
  /**
   * @brief Find a parameter by parameter name
   * @note This function will never return nullptr, it is safe to call any function over the pointer returned
   */
  virtual parameter const* find(std::string_view name) const noexcept = 0;

  /**
   * @brief Get a string representation of the parameter
   */
  virtual std::string to_string() const noexcept = 0;

private:
  std::string_view param_name;
};

class default_parameter : public parameter
{
public:
  ACL_API static default_parameter const* get_instance();

  int64_t          as_int64(int64_t default_value) const noexcept final;
  uint64_t         as_uint64(uint64_t default_value) const noexcept final;
  float            as_float(float default_value) const noexcept final;
  double           as_double(double default_value) const noexcept final;
  bool             as_bool(bool default_value) const noexcept final;
  std::string_view as_sv(std::string_view default_value) const noexcept final;
  std::string      as_string(std::string_view default_value) const noexcept final;

  parameter const* at(uint32_t i) const noexcept final;
  parameter const* find(std::string_view name) const noexcept final;

  std::string to_string() const noexcept final;
};

class parameter_value : public parameter
{
public:
  parameter_value() noexcept = default;
  inline parameter_value(std::string_view name, std::string_view value) noexcept : parameter(name), param_value(value)
  {}

  int64_t          as_int64(int64_t default_value) const noexcept final;
  uint64_t         as_uint64(uint64_t default_value) const noexcept final;
  float            as_float(float default_value) const noexcept final;
  double           as_double(double default_value) const noexcept final;
  bool             as_bool(bool default_value) const noexcept final;
  std::string_view as_sv(std::string_view default_value) const noexcept final;
  std::string      as_string(std::string_view default_value) const noexcept final;

  parameter const* at(uint32_t i) const noexcept final;
  parameter const* find(std::string_view name) const noexcept final;

  inline explicit operator std::string_view() const noexcept
  {
    return param_value;
  }

  inline bool empty() const noexcept
  {
    return param_value.empty();
  }

  ACL_API std::string to_string() const noexcept final;

private:
  std::string param_value;
};

class parameter_list : public parameter
{
public:
  using list = std::vector<std::unique_ptr<parameter>>;

  parameter_list() noexcept = default;
  inline parameter_list(std::string_view name) noexcept : parameter(name) {}

  int64_t          as_int64(int64_t default_value) const noexcept final;
  uint64_t         as_uint64(uint64_t default_value) const noexcept final;
  float            as_float(float default_value) const noexcept final;
  double           as_double(double default_value) const noexcept final;
  bool             as_bool(bool default_value) const noexcept final;
  std::string_view as_sv(std::string_view default_value) const noexcept final;
  std::string      as_string(std::string_view default_value) const noexcept final;

  parameter const* at(uint32_t i) const noexcept final;
  parameter const* find(std::string_view name) const noexcept final;

  std::string to_string() const noexcept override;

  inline std::string_view name() const noexcept
  {
    return param_name;
  }

  inline auto const& value() const noexcept
  {
    return param_value;
  }

  inline auto begin() noexcept
  {
    return param_value.begin();
  }
  inline auto begin() const noexcept
  {
    return param_value.begin();
  }
  inline auto end() noexcept
  {
    return param_value.end();
  }
  inline auto end() const noexcept
  {
    return param_value.end();
  }

  inline bool empty() const noexcept
  {
    return param_value.empty();
  }

  inline void add(std::unique_ptr<parameter> param)
  {
    param_value.emplace_back(std::move(param));
  }

protected:
  void to_string(std::string& v, bool with_braces) const noexcept;

private:
  std::string_view param_name;
  list             param_value;
};

class parameter_main : public parameter_list
{
public:
  std::string to_string() const noexcept final;
};

} // namespace acl
