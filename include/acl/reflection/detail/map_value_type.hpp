#include <acl/reflection/bind.hpp>

namespace acl::detail
{

template <typename K, typename V, typename Opt>
struct map_value_type
{
  using key_type   = K;
  using value_type = V;

  using map_key_field_name   = key_field_name_t<Opt>;
  using map_value_field_name = value_field_name_t<Opt>;

  K key_;
  V value_;

  map_value_type() noexcept = default;
  map_value_type(K k, V v) noexcept : key_(std::move(k)), value_(std::move(v)) {}
  ~map_value_type() noexcept = default;

  map_value_type(map_value_type const&) noexcept                    = default;
  map_value_type(map_value_type&&) noexcept                         = default;
  auto operator=(map_value_type const&) noexcept -> map_value_type& = default;
  auto operator=(map_value_type&&) noexcept -> map_value_type&      = default;

  static auto constexpr reflect() noexcept
  {
    return acl::bind(acl::bind<map_key_field_name::value, &map_value_type::key_>(),
                     acl::bind<map_value_field_name::value, &map_value_type::value_>());
  }
};

template <typename V, typename Opt>
struct string_map_value_type
{
  using key_type             = std::string_view;
  using value_type           = V;
  using map_value_field_name = value_field_name_t<Opt>;

  using is_string_map_value_type = std::true_type;
  std::string_view key_;
  V                value_;

  string_map_value_type() noexcept = default;
  string_map_value_type(std::string_view k, V v) noexcept : key_(k), value_(std::move(v)) {}
  ~string_map_value_type() noexcept = default;

  string_map_value_type(string_map_value_type const&) noexcept                    = default;
  string_map_value_type(string_map_value_type&&) noexcept                         = default;
  auto operator=(string_map_value_type const&) noexcept -> string_map_value_type& = default;
  auto operator=(string_map_value_type&&) noexcept -> string_map_value_type&      = default;

  static auto constexpr reflect() noexcept
  {
    return acl::bind(acl::bind<map_value_field_name::value, &string_map_value_type::value_>());
  }
};
} // namespace acl::detail