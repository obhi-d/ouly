
#include "yaml_large_data_test.hpp"
#include "catch2/catch_all.hpp"
#include "ouly/reflection/detail/base_concepts.hpp"
#include "ouly/reflection/visitor.hpp"
#include "ouly/serializers/lite_yml.hpp"
#include <array>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <variant>
#include <vector>
// NOLINTBEGIN
struct BehaviorNodeData
{
  std::string_view name;
  std::string_view type;
  std::string_view method;
  uint32_t         duration = 0;

  std::vector<BehaviorNodeData> children;
};

struct BehaviorTreeEntry
{
  std::string_view name;
  BehaviorNodeData root;
};

TEST_CASE("yaml_object: Test read")
{
  std::vector<BehaviorTreeEntry> entries;
  ouly::yml::from_string(entries, large_yml);

  REQUIRE(entries.empty() == false);
}

TEST_CASE("yaml_object: Test read with null characters")
{
  std::string yml_with_null{large_yml};
  auto        first  = yml_with_null.find("\n- name:");
  auto        second = yml_with_null.find("\n- name:", first + 1);
  REQUIRE(second != std::string::npos);

  auto insert_at = second + 1;
  yml_with_null.insert(insert_at, 1, '\0');

  std::vector<BehaviorTreeEntry> entries;
  ouly::yml::from_string(entries, std::string_view(yml_with_null.data(), yml_with_null.size()));

  std::vector<BehaviorTreeEntry> expected;
  ouly::yml::from_string(expected, std::string_view(yml_with_null.data(), insert_at));

  REQUIRE(entries.empty() == false);
  REQUIRE(entries.size() == expected.size());
}

// NOLINTEND
