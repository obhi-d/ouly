
#include <acl/dsl/scli.hpp>
#include <catch2/catch_all.hpp>

struct user_context
{
  std::string value;
  int         indent = 0;
  int         errors = 0;
};

struct default_reg_handler
{
  static void enter(acl::scli&, std::string_view id) noexcept {}
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
	  + acl::reg<"root", default_reg_handler> 
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

struct classic_cmd
{
  bool execute(acl::scli& s, acl::parameter_list const& params)
  {
    auto& ctx = s.get<user_context>();
    ctx.value += std::string(ctx.indent, ' ');
    ctx.value += s.get_command_name();
    ctx.value += ": ";
    ctx.value += params.to_string();
    ctx.value += "\n";
    return true;
  }

  bool enter(acl::scli& s)
  {
    auto& ctx = s.get<user_context>();
    ctx.value += std::string(ctx.indent, ' ');
    ctx.value += "{\n";
    ctx.indent++;
    return true;
  }

  void exit(acl::scli& s)
  {
    auto& ctx = s.get<user_context>();
    ctx.indent--;
    ctx.value += std::string(ctx.indent, ' ');
    ctx.value += "}\n";
  }
};

std::string_view trim(std::string_view str, std::string_view whitespace = " \t\n\r")
{
  const auto str_begin = str.find_first_not_of(whitespace);
  if (str_begin == std::string::npos)
    return str;

  const auto str_end   = str.find_last_not_of(whitespace);
  const auto str_range = str_end - str_begin + 1;

  return str.substr(str_begin, str_range);
}

bool diff(std::string_view first, std::string_view second)
{
  return trim(first) != trim(second);
}

TEST_CASE("Test classic", "[scli][classic]")
{
  acl::scli::builder builder;

  std::string_view input = R"(
                c1 [first, line];
                c2 c2p1="c2p1.value" [c2p2="c2p2 value", [c2p3="c2p3 1", c2p4 = 100]];
                g1 g2p1="20.4"
                {
                    c2.1 c2_called;
                }
)";

  std::string_view expected_output = R"(

c1: [ first, line ]
c2: c2p1 = "c2p1.value" , [ c2p2 = "c2p2 value" , [ c2p3 = "c2p3 1" , c2p4 = "100"  ] ]
g1: g2p1 = "20.4" 
{
 c2.1: c2_called
}

)";

  // clang-format off
  builder
	  + acl::reg<"root", default_reg_handler> 
	    - acl::cmd<"c1", classic_cmd>
        - acl::cmd<"c2", classic_cmd>
        - acl::cmd<"c3", classic_cmd>
        + acl::cmd<"g1", classic_cmd>
            - acl::cmd<"c2.1", classic_cmd>
            - acl::cmd<"c2.2", classic_cmd>
            - acl::cmd<"c2.3", classic_cmd>
            - acl::endl
        + acl::cmd<"g2", classic_cmd>
            - acl::cmd<"c3.1", classic_cmd>
            - acl::cmd<"c3.2", classic_cmd>
            - acl::endl;
      ;
  // clang-format on

  auto         ctx = builder.build();
  user_context uc;
  acl::scli::parse(*ctx.get(), uc, "memory", input, {},
                   [&uc](acl::scli::location const&, std::string_view error, std::string_view context)
                   {
                     uc.errors++;
                   });

  REQUIRE(uc.errors == 0);
  REQUIRE(diff(uc.value, expected_output) == false);
}

TEST_CASE("Test multi-level classic", "[scli][classic]")
{
  acl::scli::builder builder;

  std::string_view input = R"(
                first [word, spoken];
                second word [tested];
                third word=tested
                { 
                    sky-wrath mage
                    {
                      ursa warrior;
                      bara charging;
                      into the storm
                      {
                        found : crystal;
                      }
                      eventually;
                    }
                    we need a "support!";
                }
                mid or feed;
)";

  std::string_view expected_output = R"(

first: [ word, spoken ]
second: word, [ tested ]
third: word = "tested" 
{
 sky-wrath: mage
 {
  ursa: warrior
  bara: charging
  into: the, storm
  {
   found: crystal
  }
  eventually: 
 }
 we: need, a, support!
}
mid: or, feed

)";

  // clang-format off
  builder
  + acl::reg<"root", default_reg_handler> 
  - acl::cmd<"first", classic_cmd>
  - acl::cmd<"second", classic_cmd>
  + acl::cmd<"third", classic_cmd>
      + acl::cmd<"sky-wrath", classic_cmd>
          - acl::cmd<"ursa", classic_cmd>
          - acl::cmd<"bara", classic_cmd>
          + acl::cmd<"into", classic_cmd>
              - acl::cmd<"found", classic_cmd>
              - acl::endl
          - acl::cmd<"eventually", classic_cmd>
          - acl::endl
      - acl::cmd<"we", classic_cmd>
      - acl::endl
  - acl::cmd<"mid", classic_cmd>
        ;
  // clang-format on

  auto         ctx = builder.build();
  user_context uc;
  acl::scli::parse(*ctx.get(), uc, "memory", input, {},
                   [&uc](acl::scli::location const&, std::string_view error, std::string_view context)
                   {
                     uc.errors++;
                   });

  REQUIRE(uc.errors == 0);
  REQUIRE(diff(uc.value, expected_output) == false);
}

struct region_handler
{
  static void enter(acl::scli& s, std::string_view id)
  {
    auto& ctx = s.get<user_context>();
    ctx.value += "-- code: ";
    ctx.value += id;
    ctx.value += "\n";
  }

  static void enter(acl::scli& s, std::string_view id, std::string_view content)
  {
    auto& ctx = s.get<user_context>();
    ctx.value += "-- text: ";
    ctx.value += id;
    ctx.value += "\n";
    ctx.value += content;
    ctx.value += "\n";
  }
};

TEST_CASE("Test region entry points", "[scli][classic]")
{

  acl::scli::builder builder;

  std::string_view input = R"(
-- code : region1 --
first command;
-- text : region2 --
this is a long line 
of text that is not
a series of cmds.
-- code : region3 --
second command;
-- code : region4 --
third command;
-- glsl : region5 --
glsl code
-- hlsl : region6 --
hsls code
)";

  std::string_view expected_output = R"(
  )";

  // clang-format off
  builder
  + acl::reg<"root", region_handler> 
  + acl::reg<"code", region_handler> 
    - acl::cmd<"first", classic_cmd>
    - acl::cmd<"second", classic_cmd>
    - acl::cmd<"third", classic_cmd>
  + acl::reg<"glsl", region_handler> 
  + acl::reg<"hlsl", region_handler>
  + acl::reg<"text", region_handler>;

  // clang-format on
  auto         ctx = builder.build();
  user_context uc;
  acl::scli::parse(*ctx.get(), uc, "memory", input, {},
                   [&uc](acl::scli::location const&, std::string_view error, std::string_view context)
                   {
                     uc.errors++;
                   });

  REQUIRE(uc.errors == 0);
  REQUIRE(diff(uc.value, expected_output) == false);
}