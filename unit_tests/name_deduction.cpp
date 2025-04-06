#include "catch2/catch_all.hpp"
#include "ouly/reflection/detail/base_concepts.hpp"
#include "ouly/reflection/visitor.hpp"
#include <iostream>

TEST_CASE("Member name inside function")
{
  using namespace std::string_view_literals;
  struct TestStruct
  {
    int variable;
  };

  auto fn = []<typename T>(T&& xobj) constexpr noexcept -> decltype(auto)
  {
    auto&& [obj] = std::forward<T>(xobj);
    return ouly::detail::field_ref{std::forward<decltype(obj)>(obj)};
  };
  auto constexpr fref = fn(ouly::detail::xobj<TestStruct>);
  REQUIRE((std::string_view)ouly::detail::deduce_field_name<TestStruct, fref>() == "variable"sv);
}

struct TestStruct
{
  int variable;
};

TEST_CASE("Member name outside function")
{
  using namespace std::string_view_literals;

  auto fn = []<typename T>(T&& xobj) constexpr noexcept -> decltype(auto)
  {
    auto&& [obj] = std::forward<T>(xobj);
    return ouly::detail::field_ref{std::forward<decltype(obj)>(obj)};
  };
  auto constexpr fref = fn(ouly::detail::xobj<TestStruct>);
  REQUIRE((std::string_view)ouly::detail::deduce_field_name<TestStruct, fref>() == "variable"sv);
}

TEST_CASE("Nested member name inside function")
{
  using namespace std::string_view_literals;
  struct NestedTestStruct
  {
    struct Internal
    {
      int variable;
    };
  };

  auto fn = []<typename T>(T&& xobj) constexpr noexcept -> decltype(auto)
  {
    auto&& [obj] = std::forward<T>(xobj);
    return ouly::detail::field_ref{std::forward<decltype(obj)>(obj)};
  };
  auto constexpr fref = fn(ouly::detail::xobj<NestedTestStruct::Internal>);
  REQUIRE((std::string_view)ouly::detail::deduce_field_name<NestedTestStruct::Internal, fref>() == "variable"sv);
}

struct NestedTestStruct
{
  struct Internal
  {
    int variable;
  };
};

TEST_CASE("Nested member name outside function")
{
  using namespace std::string_view_literals;

  auto fn = []<typename T>(T&& xobj) constexpr noexcept -> decltype(auto)
  {
    auto&& [obj] = std::forward<T>(xobj);
    return ouly::detail::field_ref{std::forward<decltype(obj)>(obj)};
  };
  auto constexpr fref = fn(ouly::detail::xobj<NestedTestStruct::Internal>);
  REQUIRE((std::string_view)ouly::detail::deduce_field_name<NestedTestStruct::Internal, fref>() == "variable"sv);
}
