
#include "test_common.hpp"
#include <acl/utility/projected_view.hpp>
#include <catch2/catch_all.hpp>
#include <string>
#include <vector>

// NOLINTBEGIN
struct MyClass
{
  std::string name;
  int         value = 0;
};

TEST_CASE("Projected view basic tests")
{
  std::vector<MyClass> vector = {
   MyClass{   "Hey", 1},
   MyClass{  "Fill", 2},
   MyClass{  "This", 3},
   MyClass{"Vector", 4}
  };
  auto view = acl::projected_view<&MyClass::name>(vector.data(), vector.size());

  REQUIRE(view.size() == 4);
  REQUIRE(view[0] == "Hey");
  view[0] = "You";
  REQUIRE(view[0] == "You");

  auto iview = acl::projected_view<&MyClass::value>(vector.data(), vector.size());
  int  c     = 0;
  std::ranges::for_each(iview,
                        [&c](int v)
                        {
                          c += v;
                        });
  REQUIRE(c == 10);

  std::vector<MyClass> const cvector = {
   MyClass{   "Hey", 1},
   MyClass{  "Fill", 2},
   MyClass{  "This", 3},
   MyClass{"Vector", 4}
  };

  auto cview = acl::projected_cview<&MyClass::name>(cvector.data(), cvector.size());

  REQUIRE(cview.size() == 4);
  REQUIRE(cview[0] == "Hey");

  auto icview = acl::projected_cview<&MyClass::value>(cvector.data(), cvector.size());
  c           = 0;
  std::ranges::for_each(icview,
                        [&c](int v)
                        {
                          c += v;
                        });
  REQUIRE(c == 10);
}
// NOLINTEND