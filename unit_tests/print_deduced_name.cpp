#include "ouly/reflection/detail/base_concepts.hpp"
#include "ouly/reflection/visitor.hpp"
#include <iostream>

int main()
{
  using namespace std::string_view_literals;
  struct TestStruct
  {
    int variable;
    std::string member_str;
  };

  auto fn = []<typename T>(T&& xobj) constexpr noexcept -> decltype(auto)
  {
    auto&& [obj, oth] = std::forward<T>(xobj);
    return ouly::detail::field_ref{std::forward<decltype(obj)>(obj)};
  };
  auto constexpr fref = fn(ouly::detail::xobj<TestStruct>);
  std::cout << (std::string_view)ouly::detail::deduce_field_name<TestStruct, fref>() << std::endl;

  
  auto constexpr names = ouly::detail::aggregate_lookup<TestStruct>(
   [](auto&&... args) constexpr -> decltype(auto)
   {
     return std::make_tuple(ouly::detail::field_ref{args}...);
   });

  constexpr auto name0 = std::get<0>(names);

  auto const& field_names = ouly::detail::get_cached_field_names<TestStruct, ouly::pass_through_transform>();
  std::cout << std::get<1>(field_names) << std::endl;
  return 0;
}
