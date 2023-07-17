
#include <acl/dsl/scli.hpp>
#include <catch2/catch_all.hpp>

struct user_context
{
  std::string value;
  int         errors = 0;
};

TEST_CASE("Test builder", "[scli][builder]")
{
  struct echo
  {
    std::vector<std::string_view> fragments;

    bool execute(acl::scli& s)
    {
      for (auto& f : fragments)
      {
        auto& ctx = s.get<user_context>();
        ctx.value += f;
      }
      return true;
    }

    static auto reflect() noexcept
    {
      return acl::bind(acl::bind<"fragments", &echo::fragments>());
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
	  [ "root" ]
	    - acl::cmd<"*", echo>
        - acl::cmd<"hi", say_hi>
      ;

  // clang-format on
  auto         ctx = builder.build();
  user_context uc;
  acl::scli::parse(*ctx.get(), uc, "memory", R"(   
                    echo [first, line];
                    hi hi=next;
                    echo fragments=simple;
                   )",
                   {},
                   [&uc](acl::scli::location const&, std::string_view error, std::string_view context)
                   {
                     uc.errors++;
                   });

  REQUIRE(uc.errors == 0);
  REQUIRE(uc.value == "firstlinehi-nextsimple");
}
