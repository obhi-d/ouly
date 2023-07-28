#include <acl/utils/string_utils.hpp>
#include <catch2/catch_all.hpp>
#include <sstream>

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
  std::string_view white_pos = "  \t  \t here";
  std::string_view no_white  = acl::eat_white(white_pos.data());
  REQUIRE(no_white == "here");
  auto u8str = u8"? E?da = Q,  n ? ?, ? f(i) = ? g(i), ?x??: ?x? = ???x?, ? ? ¬? = ¬(¬? ? ?),";
}