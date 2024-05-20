
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
  static void enter(acl::scli&, std::string_view id, std::string_view name) noexcept {}
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
      - acl::endl;

  // clang-format on
  auto         ctx = builder.build();
  user_context uc;
  acl::scli::parse(*ctx.get(), uc, "memory", R"(   
                    echo (first, line);
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
                c1 (first, line);
                c2 c2p1="c2p1.value" (c2p2="c2p2 value", (c2p3="c2p3 1", c2p4 = 100));
                g1 g2p1="20.4"
                {
                    c2.1 c2_called;
                }
)";

  std::string_view expected_output = R"(

c1: ( first, line )
c2: c2p1 = "c2p1.value" , ( c2p2 = "c2p2 value" , ( c2p3 = "c2p3 1" , c2p4 = "100"  ) )
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
          - acl::endl
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
                first (word, spoken);
                second word (tested);
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

first: ( word, spoken )
second: word, ( tested )
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
    - acl::endl
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
  static void enter(acl::scli& s, std::string_view id, std::string_view name)
  {
    auto& ctx = s.get<user_context>();
    ctx.value += "-- code: ";
    ctx.value += name;
    ctx.value += "\n";
  }
};

struct text_region_handler
{
  static void enter(acl::scli& s, std::string_view id, std::string_view name, acl::text_content&& content)
  {
    auto& ctx = s.get<user_context>();
    ctx.value += "-- text: ";
    ctx.value += name;
    ctx.value += "\n";
    ctx.value += acl::view(content);
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

  std::string_view expected_output =
    "-- code: \n-- code: region1\nfirst: command\n-- text: region2\n\nthis is a long line \nof text that is not\na "
    "series of cmds.\n\n-- code: region3\nsecond: command\n-- code: region4\nthird: command\n-- text: region5\n\nglsl "
    "code\n\n-- text: region6\n\nhsls code\n\n";

  // clang-format off
  builder
  + acl::reg<"root", region_handler> 
    - acl::endl
  + acl::reg<"code", region_handler> 
    - acl::cmd<"first", classic_cmd>
    - acl::cmd<"second", classic_cmd>
    - acl::cmd<"third", classic_cmd>
    - acl::endl
  - acl::reg<"glsl", text_region_handler> 
  - acl::alias<"hlsl", "glsl">
  - acl::alias<"text", "glsl">;
  // clang-format on
  auto         ctx = builder.build();
  user_context uc;
  acl::scli::parse(*ctx.get(), uc, "memory", input, {},
                   [&uc](acl::scli::location const&, std::string_view error, std::string_view context)
                   {
                     uc.errors++;
                   });

  REQUIRE(uc.errors == 0);
  REQUIRE(uc.value == expected_output);
}

TEST_CASE("Test alias", "[scli][classic]")
{

  acl::scli::builder builder;

  std::string_view input = R"(
-- code : region1 --
first 1
{
  sec 2;
}
third 3
{
  fourth 4;
}
)";

  std::string_view expected_output = R"(-- code: 
-- code: region1
first: 1
{
 sec: 2
}
third: 3
{
 fourth: 4
}
)";

  // clang-format off
  builder
  + acl::reg<"root", region_handler> 
    - acl::endl
  + acl::reg<"code", region_handler> 
    + acl::cmd<"first", classic_cmd>
      - acl::cmd<"sec", classic_cmd>
      - acl::endl
    + acl::cmd<"third", classic_cmd>
      - acl::alias<"fourth", "code.first.sec">
      - acl::endl
    - acl::endl;
  // clang-format on

  auto         ctx = builder.build();
  user_context uc;
  acl::scli::parse(*ctx.get(), uc, "memory", input, {},
                   [&uc](acl::scli::location const&, std::string_view error, std::string_view context)
                   {
                     uc.errors++;
                   });

  REQUIRE(uc.errors == 0);
  REQUIRE(uc.value == expected_output);
}

TEST_CASE("Cover empty API", "[scli][classic]")
{
  acl::scli::builder builder;
  auto               ctx = builder.build();
  user_context       uc;
  acl::scli::parse(
    *ctx.get(), uc, "memory", "", {},
    [&uc](acl::scli::location const&, std::string_view error, std::string_view context)
    {
      uc.errors++;
    },
    {},
    [](acl::scli& scli)
    {
      acl::cmd_context cmd_ctx;
      cmd_ctx.construct(scli);
      cmd_ctx.destroy(scli, nullptr);
      cmd_ctx.execute(scli, nullptr);
      cmd_ctx.enter_region(scli, "", "");
      cmd_ctx.enter_region(scli, "", "", std::string_view());
      REQUIRE(cmd_ctx.is_text_context() == false);
      cmd_ctx.enter(scli, nullptr);
      cmd_ctx.exit(scli, nullptr);
      cmd_ctx.add_sub_command("", nullptr);
      REQUIRE(cmd_ctx.get_sub_command("") == nullptr);
    });
}

struct value_user_context
{
  uint64_t    uint64_v     = {};
  int64_t     int64_v      = {};
  uint32_t    uint32_v     = {};
  int32_t     int32_v      = {};
  float       float_v      = {};
  double      double_v     = {};
  bool        boolean      = {};
  std::string string_value = {};
  std::string sview        = {};
};

struct uint64_cmd
{
  uint64_t value = {};

  bool execute(acl::scli& s)
  {
    auto& ctx    = s.get<value_user_context>();
    ctx.uint64_v = value;
    return true;
  }

  static auto reflect() noexcept
  {
    return acl::bind(acl::bind<"value", &uint64_cmd::value>());
  }
};

struct int64_cmd
{
  int64_t value = {};

  bool execute(acl::scli& s)
  {
    auto& ctx   = s.get<value_user_context>();
    ctx.int64_v = value;
    return true;
  }

  static auto reflect() noexcept
  {
    return acl::bind(acl::bind<"value", &int64_cmd::value>());
  }
};

struct uint32_cmd
{
  uint32_t value = {};

  bool execute(acl::scli& s)
  {
    auto& ctx    = s.get<value_user_context>();
    ctx.uint32_v = value;
    return true;
  }

  static auto reflect() noexcept
  {
    return acl::bind(acl::bind<"value", &uint32_cmd::value>());
  }
};

struct int32_cmd
{
  int32_t value = {};

  bool execute(acl::scli& s)
  {
    auto& ctx   = s.get<value_user_context>();
    ctx.int32_v = value;
    return true;
  }

  static auto reflect() noexcept
  {
    return acl::bind(acl::bind<"value", &int32_cmd::value>());
  }
};

struct float_cmd
{
  float value = {};

  bool execute(acl::scli& s)
  {
    auto& ctx   = s.get<value_user_context>();
    ctx.float_v = value;
    return true;
  }

  static auto reflect() noexcept
  {
    return acl::bind(acl::bind<"value", &float_cmd::value>());
  }
};

struct double_cmd
{
  double value = {};

  bool execute(acl::scli& s)
  {
    auto& ctx    = s.get<value_user_context>();
    ctx.double_v = value;
    return true;
  }

  static auto reflect() noexcept
  {
    return acl::bind(acl::bind<"value", &double_cmd::value>());
  }
};

struct bool_cmd
{
  bool value = {};

  bool execute(acl::scli& s)
  {
    auto& ctx   = s.get<value_user_context>();
    ctx.boolean = value;
    return true;
  }

  static auto reflect() noexcept
  {
    return acl::bind(acl::bind<"value", &bool_cmd::value>());
  }
};

struct string_cmd
{
  std::string value = {};

  bool execute(acl::scli& s)
  {
    auto& ctx        = s.get<value_user_context>();
    ctx.string_value = value;
    return true;
  }

  static auto reflect() noexcept
  {
    return acl::bind(acl::bind<"value", &string_cmd::value>());
  }
};

struct string_view_cmd
{
  std::string_view value = {};

  bool execute(acl::scli& s)
  {
    auto& ctx = s.get<value_user_context>();
    ctx.sview = value;
    return true;
  }

  static auto reflect() noexcept
  {
    return acl::bind(acl::bind<"value", &string_view_cmd::value>());
  }
};

TEST_CASE("Cover types", "[scli][classic]")
{
  std::string_view input = R"(
   uint64 1002;
   int64 -153;
   uint32 1002;
   int32 -13;
   float 10.0;
   double -21.0;
   boolean true;
   string "string";   
   string_view "view";   
)";

  acl::scli::builder builder;

  // clang-format off
  builder
	  + acl::reg<"root", default_reg_handler> 
	    - acl::cmd<"uint64", uint64_cmd>
      - acl::cmd<"int64", int64_cmd>
      - acl::cmd<"uint32", uint32_cmd>
      - acl::cmd<"int32", int32_cmd>
      - acl::cmd<"float", float_cmd>
      - acl::cmd<"double", double_cmd>
      - acl::cmd<"boolean", bool_cmd>
      - acl::cmd<"string", string_cmd>
      - acl::cmd<"string_view", string_view_cmd>
      - acl::endl;
  // clang-format on
  auto               ctx = builder.build();
  value_user_context uc;
  acl::scli::parse(*ctx.get(), uc, "memory", input, {},
                   [&uc](acl::scli::location const&, std::string_view error, std::string_view context) {});

  REQUIRE(uc.uint64_v == 1002);
  REQUIRE(uc.int64_v == -153);
  REQUIRE(uc.uint32_v == 1002);
  REQUIRE(uc.int32_v == -13);
  REQUIRE(uc.float_v == 10.0f);
  REQUIRE(uc.double_v == -21.0f);
  REQUIRE(uc.boolean == true);
  REQUIRE(uc.string_value == "string");
  REQUIRE(uc.sview == "view");
}

struct ignore_checker_ctx
{
  bool failed = false;
};

struct ignore_block
{
  bool enter(acl::scli&)
  {
    return false;
  }

  bool execute(acl::scli&, acl::parameter_list const&)
  {
    return true;
  }
};

struct ignore_checker
{
  bool enter(acl::scli& s)
  {
    auto& ctx  = s.get<ignore_checker_ctx>();
    ctx.failed = true;
    return false;
  }

  bool execute(acl::scli& s, acl::parameter_list const&)
  {
    return true;
  }
};

TEST_CASE("Ignore block check", "[scli][ignore]")
{
  std::string_view input = R"(
   ignore {
     checker;
   };
)";

  acl::scli::builder builder;

  // clang-format off
  builder
    + acl::reg<"root", default_reg_handler>
      + acl::cmd<"*", ignore_block>
        - acl::cmd<"*", ignore_checker>
        - acl::endl
      - acl::endl
    - acl::endl;
  // clang-format on
  auto               ctx = builder.build();
  ignore_checker_ctx uc;
  acl::scli::parse(*ctx.get(), uc, "memory", input, {},
                   [&uc](acl::scli::location const&, std::string_view error, std::string_view context) {});

  REQUIRE(uc.failed == false);
}

struct string_list_test
{
  struct ctx
  {
    std::string result;
    std::string what;
    bool        failed = false;
  };
  std::vector<std::string> value;
  std::string              what;

  bool execute(acl::scli& s)
  {
    auto& c = s.get<ctx>();
    for (auto const& v : value)
      c.result += v;
    c.what = what;
    return true;
  }

  static auto reflect() noexcept
  {
    return acl::bind(acl::bind<"something", &string_list_test::value>(),
                     acl::bind<"something-else", &string_list_test::what>());
  }
};

TEST_CASE("Check string list", "[scli][ignore]")
{
  std::string_view input = R"(
   call something = (what, is, going, on), something-else =  what;
)";

  acl::scli::builder builder;

  // clang-format off
  builder
    + acl::reg<"root", default_reg_handler>
      - acl::cmd<"call", string_list_test>
    - acl::endl;
  // clang-format on
  auto                  ctx = builder.build();
  string_list_test::ctx uc;
  acl::scli::parse(*ctx.get(), uc, "memory", input, {},
                   [&uc](acl::scli::location const&, std::string_view error, std::string_view context)
                   {
                     uc.result = error;
                     uc.failed = true;
                   });

  REQUIRE(uc.failed == false);
  REQUIRE(uc.result == "whatisgoingon");
  REQUIRE(uc.what == "what");
}

struct ignore_block_cmd
{
  bool enter(acl::scli&)
  {
    return false;
  }

  bool execute(acl::scli&, acl::parameter_list const&)
  {
    return true;
  }

  void exit(acl::scli&) {}
};

struct echo_cmd
{
  struct ctx
  {
    bool set    = false;
    bool failed = false;
  };

  bool execute(acl::scli& s, acl::parameter_list const&)
  {
    auto& c = s.get<ctx>();
    c.set   = true;
    return true;
  }
};

struct accept_block_cmd
{
  bool enter(acl::scli&)
  {
    return true;
  }

  bool execute(acl::scli&, acl::parameter_list const&)
  {
    return true;
  }

  void exit(acl::scli&) {}
};

TEST_CASE("Ignore multiple blocks", "[scli][ignore]")
{
  std::string_view input = R"(
   cmd1 something 
   {
     cmd2 other;
     cmd3 thing;
   }

   cmd4 something_more;
   cmd5 ignore_this too;
   cmd6 
   {
     ignore me;
   }
   accept
   {
     echo;
   } 
   cmd7;
   cmd8;
)";

  acl::scli::builder builder;
  // clang-format off
  builder
    + acl::reg<"root", default_reg_handler>
      + acl::cmd<"accept", accept_block_cmd>
        - acl::cmd<"echo", echo_cmd>
        - acl::endl
      + acl::cmd<"*", ignore_block_cmd>
        - acl::endl
    - acl::endl;
  // clang-format on
  auto          ctx = builder.build();
  echo_cmd::ctx uc;
  acl::scli::parse(*ctx.get(), uc, "memory", input, {},
                   [&uc](acl::scli::location const&, std::string_view error, std::string_view context)
                   {
                     uc.failed = true;
                   });

  REQUIRE(!uc.failed);
  REQUIRE(uc.set);
}
