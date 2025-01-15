#include <acl/utility/string_utils.hpp>
#include <catch2/catch_all.hpp>
#include <sstream>

// NOLINTBEGIN

TEST_CASE("Validate word_list", "[word_list]")
{
  using wlutils = acl::word_list<>;

  std::string my_words;

  wlutils::push_back(my_words, "first");
  wlutils::push_back(my_words, std::string_view{"second"});
  wlutils::push_back(my_words, std::string("again"));

  REQUIRE(wlutils::length(my_words) == 3);
  REQUIRE(wlutils::index_of(my_words, "first") == 0);
  REQUIRE(wlutils::index_of(my_words, "second") == 1);
  REQUIRE(wlutils::index_of(my_words, "again") == 2);

  auto             it = wlutils::iter(my_words);
  std::string_view r;
  REQUIRE(it.has_next(r) == true);
  REQUIRE(r == "first");
  REQUIRE(it.has_next(r) == true);
  REQUIRE(r == "second");
  REQUIRE(it.has_next(r) == true);
  REQUIRE(r == "again");

  it = wlutils::iter(my_words);
  REQUIRE((bool)it == true);
  REQUIRE(*it == "first");
  REQUIRE(it == "first");
  REQUIRE("first" == it);
  auto copy = it;
  REQUIRE(copy(r) == true);
  REQUIRE(r == "first");
  REQUIRE(it.index() == 0);
  REQUIRE(copy.index() == 1);
  ++it;

  REQUIRE((bool)it == true);
  REQUIRE(*it == "second");
  REQUIRE(it == "second");
  REQUIRE("second" == it);
  copy = it;
  REQUIRE(copy(r) == true);
  REQUIRE(r == "second");
  REQUIRE(it.index() == 1);
  REQUIRE(copy.index() == 2);
  ++it;

  std::stringstream ss;
  ss << it;
  REQUIRE(ss.str() == "again");
  REQUIRE((bool)it == true);
  REQUIRE(*it == "again");
  REQUIRE(it == "again");
  REQUIRE("again" == it);
  copy = it;
  REQUIRE(copy(r) == true);
  REQUIRE(r == "again");
  REQUIRE(it.index() == 2);
  REQUIRE(copy.index() == 3);
  REQUIRE(copy(r) == false);
  ++it;
}

TEST_CASE("Validate string utils wordlist", "[string_utils]")
{
  std::string words;
  acl::word_push_back(words, "first");
  acl::word_push_back(words, "second");
  acl::word_push_back(words, "third");

  REQUIRE(acl::index_of(words, "first") != std::numeric_limits<uint32_t>::max());
  REQUIRE(acl::index_of(words, "_first") == std::numeric_limits<uint32_t>::max());
  REQUIRE(acl::contains(words, "first") == true);
  REQUIRE(acl::contains(words, "second") == true);
  REQUIRE(acl::contains(words, "soup") == false);
  REQUIRE(acl::time_stamp().empty() == false);
  REQUIRE(acl::time_string().empty() == false);

  acl::word_push_back(words, "a111");
  acl::word_push_back(words, "a121");
  acl::word_push_back(words, "a161");

  std::string replace;
  std::regex  re("a[0-9][0-9][0-9]");
  replace = acl::regex_replace(words, re,
                               [](auto const& match)
                               {
                                 return match.str(1);
                               });
  REQUIRE(acl::index_of(replace, "111") != 3);
  REQUIRE(acl::index_of(replace, "121") != 4);
  REQUIRE(acl::index_of(replace, "161") != 5);
}

TEST_CASE("Validate string functions", "[string_utils]")
{
  std::string cool = "a cool string that cools on its own cooling coolant";
  std::string hot  = cool;
  REQUIRE(acl::replace_first(hot, "cool", "hot"));
  REQUIRE(hot == "a hot string that cools on its own cooling coolant");
  hot = cool;
  REQUIRE(acl::replace(hot, "cool", "hot") == 4);
  REQUIRE(hot == "a hot string that hots on its own hoting hotant");

  cool = "CoolIngLikeKing";
  REQUIRE(acl::format_name(cool) == "Cool Ing Like King");
  cool = "cool_ing_like_King";
  REQUIRE(acl::format_name(cool) == "Cool Ing Like King");
  cool = "CoolIngLikeKing";
  REQUIRE(acl::to_lower(cool) == "coolinglikeking");
  REQUIRE(acl::to_upper(cool) == "COOLINGLIKEKING");
  REQUIRE(acl::split("Abc:Bcd") == acl::string_view_pair("Abc", "Bcd"));
  REQUIRE(acl::split(":Bcd") == acl::string_view_pair("", "Bcd"));
  REQUIRE(acl::split("Abc:") == acl::string_view_pair("Abc", ""));
  REQUIRE(acl::split("Abc") == acl::string_view_pair("Abc", ""));
  REQUIRE(acl::split("Abc", '.', false) == acl::string_view_pair("", "Abc"));
  REQUIRE(acl::split_last("Abc:Cde:fgc", ':', false) == acl::string_view_pair("Abc:Cde", "fgc"));
  REQUIRE(acl::split("Abc:Cde:fgc") == acl::string_view_pair("Abc", "Cde:fgc"));

  std::vector<std::string_view> store;
  std::string_view              line = ",some thing\tunlike anything  Other   than, this  ";
  acl::tokenize(
   [&store, &line](size_t start, size_t end, char)
   {
     store.emplace_back(line.begin() + start, line.begin() + end);
     return acl::response::e_continue;
   },
   line, " \t,");

  REQUIRE(store == std::vector<std::string_view>{"some", "thing", "unlike", "anything", "Other", "than", "this"});

  auto data = std::string(" \t \nAbc");
  REQUIRE(acl::trim_leading(data) == "Abc");
  data = acl::trim_leading(data);
  REQUIRE(data == "Abc");
  REQUIRE(acl::trim_leading(std::string("Abc   \t\n")) == "Abc   \t\n");
  REQUIRE(acl::trim_trailing(std::string("  \t\nAbc")) == "  \t\nAbc");
  REQUIRE(acl::trim_trailing(std::string("Abc\t \n   ")) == "Abc");

  REQUIRE(acl::trim(" \t\n Abc\t \n   ") == "Abc");
  REQUIRE(acl::trim_leading(" \t \nAbc") == "Abc");
  REQUIRE(acl::trim_leading("Abc   \t\n") == "Abc   \t\n");
  REQUIRE(acl::trim_trailing("  \t\nAbc") == "  \t\nAbc");
  REQUIRE(acl::trim_trailing("Abc\t \n   ") == "Abc");

  REQUIRE(acl::trim(" \t\n Abc\t \n   ") == "Abc");
  REQUIRE(acl::is_ascii(" \t\n Abc\t \n   ") == true);

  std::string_view              text_wall = "a long wall of text\tthat will be\t wrapped into multiple lines ";
  std::vector<std::string_view> lines;
  acl::word_wrap(
   [&lines, &text_wall](size_t start, size_t end)
   {
     lines.emplace_back(text_wall.begin() + start, text_wall.begin() + end);
   },
   32, text_wall);
  std::string rec;
  for (auto l : lines)
  {
    CHECK(l.size() <= 32);
    rec += l;
  }
  CHECK(rec == text_wall);
  std::string_view text_wall_ml =
   "a long wall of text made of steel\nthat will be in time\trecorded\n and then\n\tthat will be\t\n split into\t "
   "multiline\n\tof text and then again wrapped into multiple lines ";
  lines.clear();
  rec.clear();
  acl::word_wrap_multiline(
   [&lines, &text_wall_ml](size_t start, size_t end)
   {
     lines.emplace_back(text_wall_ml.begin() + start, text_wall_ml.begin() + end);
   },
   20, text_wall_ml, 8);
  for (auto l : lines)
  {
    REQUIRE(l.size() <= 20);
    rec += l;
    rec += "\n";
  }
  using namespace std::string_view_literals;

  CHECK(acl::is_number(" -43r"sv) == false);
  CHECK(acl::is_number("-43"sv) == true);
  CHECK(acl::is_number("a43r"sv) == false);
  CHECK(acl::is_number("43"sv) == true);
}

// NOLINTEND