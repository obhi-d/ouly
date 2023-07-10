
#include <acl/dsl/scli.hpp>
#include <catch2/catch_all.hpp>

TEST_CASE("Test builder", "[scli][builder]")
{
  struct echo
  {
    std::vector<std::string_view> fragments;

    bool execute(acl::scli& s)
    {
      for (auto& f : fragments)
      {
        auto& ctx = s.get<std::string>();
        ctx += f;
      }
      return true;
    }

    static auto reflect() noexcept
    {
      return acl::bind(acl::bind<"fragment", &echo::fragments>());
    }
  };

  struct say_hi
  {
    std::string_view name;

    bool execute(acl::scli& s)
    {
      auto& ctx = s.get<std::string>();
      ctx += "hi-";
      ctx += name;
      return true;
    }

    static auto reflect() noexcept
    {
      return acl::bind(acl::bind<"hi", &say_hi::name>());
    }
  };

  acl::scli::builder builder;

  // clang-format off
  builder
	  * acl::region<"root">
	    - acl::cmd<"echo", echo>
        - acl::cmd<"echo", say_hi>
      ;

  // clang-format on
}
