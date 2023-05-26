#include "acl/utility/program_args.hpp"
#include <catch2/catch_all.hpp>

struct arg_formatter
{
  std::string text;
  inline void operator()(acl::program_document_type type, std::string_view, std::string_view txt) noexcept
  {
    text += txt;
  }
};

TEST_CASE("Validate program args creation and destruction", "[program_args][basic]")
{
  char                 buffer[] = "arg=1";
  std::array<char*, 1> args     = {buffer};

  acl::program_args<> pgargs;
  arg_formatter       argfmt;
  pgargs.parse_args(1, args.data());

  REQUIRE(pgargs.decl<int>("arg").doc("help_int").value() == 1);
  REQUIRE(pgargs.doc(argfmt).text != "");
}

TEST_CASE("Validate program args switches", "[program_args][switches]")
{
  const char* arg_set[] = {"--help", "--one=foo", "-2=bar"};

  acl::program_args pgargs;
  arg_formatter     argfmt;
  pgargs.parse_args(3, arg_set);

  pgargs.doc("settings");
  auto one = pgargs.decl("one").doc("first_arg").value();
  auto two = pgargs.decl("two", "2").doc("second_arg").value();
  pgargs.doc(argfmt);

  REQUIRE(argfmt.text.find("settings") != std::string_view::npos);
  REQUIRE(argfmt.text.find("first_arg") != std::string_view::npos);
  REQUIRE(argfmt.text.find("second_arg") != std::string_view::npos);

  REQUIRE(one.has_value() == true);
  REQUIRE(one.value() == "foo");
  REQUIRE(two.value() == "bar");
  REQUIRE(pgargs.get_max_arg_length() != 4);
  REQUIRE(pgargs.must_print_help());
}

TEST_CASE("Validate program args sink", "[program_args][sink]")
{
  const char* arg_set[] = {"--help", "--one=foo", "-2=100"};

  acl::program_args pgargs;
  pgargs.parse_args(3, arg_set);
  struct sink
  {
    std::string_view one;
    int              two  = 0;
    bool             flag = false;
  };

  sink sink_;
  pgargs.sink(sink_.one, "one");
  pgargs.sink(sink_.two, "two", "2");
  pgargs.sink(sink_.flag, "flag", "3");

  REQUIRE(sink_.one == "foo");
  REQUIRE(sink_.two == 100);
  REQUIRE(sink_.flag == false);
}

TEST_CASE("Validate program args vector", "[program_args][vector]")
{
  const char* arg_set[] = {"--help", "--flag", "--one=[foo, bar, 2]", "-2=[100, 20, 30]", "-c=[3.4, 4.1, 6.1]"};

  acl::program_args pgargs;
  pgargs.parse_args(5, arg_set);

  bool                          help   = false;
  bool                          flag   = false;
  bool                          flag_2 = false;
  std::vector<std::string_view> one;
  std::vector<int>              two;
  std::vector<float>            three;

  pgargs.sink(help, "help");
  pgargs.sink(flag, "flag");
  pgargs.sink(flag_2, "flag_2");
  pgargs.sink(one, "one", "a");
  pgargs.sink(two, "two", "2");
  pgargs.sink(three, "three", "c");

  REQUIRE(help == true);
  REQUIRE(flag == true);
  REQUIRE(flag_2 == false);
  REQUIRE(one == std::vector<std::string_view>{"foo", "bar", "2"});
  REQUIRE(two == std::vector<int>{100, 20, 30});
  REQUIRE(three == std::vector<float>{3.4f, 4.1f, 6.1f});
}

TEST_CASE("Validate program args parse failure", "[program_args][failure]")
{
  const char* arg_set[] = {"--help", "--flag", "--one=foo", "-2=[100, 20, 30", "-c=3.4, 4.1, 6.1]"};

  acl::program_args pgargs;
  pgargs.parse_args(5, arg_set);

  bool                          help   = false;
  bool                          flag   = false;
  bool                          flag_2 = false;
  std::vector<std::string_view> one;
  std::vector<int>              two;
  std::vector<float>            three;

  pgargs.sink(help, "help");
  pgargs.sink(flag, "flag");
  pgargs.sink(flag_2, "flag_2");
  REQUIRE(pgargs.sink(one, "one", "a") == false);
  REQUIRE(pgargs.sink(two, "two", "2") == false);
  REQUIRE(pgargs.sink(three, "three", "c") == false);
}
