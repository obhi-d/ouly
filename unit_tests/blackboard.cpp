#include <acl/containers/blackboard.hpp>
#include <catch2/catch_all.hpp>
#include <string>
#include <unordered_map>

// NOLINTBEGIN
template <typename V>
using custom_map = std::unordered_map<std::string, V>;

TEST_CASE("blackboard: push_back", "[blackboard][push_back]")
{
  acl::blackboard board;
  auto&           int_index0 = board.emplace<std::uint32_t>("param1", 50);
  auto&           str_index0 = board.emplace<std::string>("param2", "number 1");
  auto&           str_index1 = board.emplace<std::string>("param3", "number 2");
  auto&           str_index2 = board.emplace<std::string>("param4", "number 3");
  auto&           int_index1 = board.emplace<std::uint32_t>("param5", 100);
  auto&           str_index3 = board.emplace<std::string>("param6", "number 4");
  auto&           int_index2 = board.emplace<std::uint32_t>("param7", 150);

  REQUIRE((int_index0) == 50);
  REQUIRE((int_index1) == 100);
  REQUIRE((int_index2) == 150);

  REQUIRE((str_index0) == "number 1");
  REQUIRE((str_index1) == "number 2");
  REQUIRE((str_index2) == "number 3");
  REQUIRE((str_index3) == "number 4");

  REQUIRE(*board.get_if<std::uint32_t>("param1") == 50);
  REQUIRE(board.get_if<std::uint32_t>("none") == nullptr);

  REQUIRE(board.get<std::uint32_t>("param1") == 50);
  REQUIRE(board.get<std::uint32_t>("param5") == 100);
  REQUIRE(board.get<std::uint32_t>("param7") == 150);

  REQUIRE(board.get<std::string>("param2") == "number 1");
  REQUIRE(board.get<std::string>("param3") == "number 2");
  REQUIRE(board.get<std::string>("param4") == "number 3");
  REQUIRE(board.get<std::string>("param6") == "number 4");

  REQUIRE(board.contains("param1"));
  REQUIRE(board.contains("param2"));
  REQUIRE(board.contains("param3"));
  REQUIRE(board.contains("param4"));
  REQUIRE(board.contains("param5"));
  REQUIRE(board.contains("param6"));
  REQUIRE(board.contains("param7"));

  board.emplace<std::uint32_t>("param1", 300);
  board.emplace<std::uint32_t>("param5", 1500);

  REQUIRE(board.get<std::uint32_t>("param1") == 300);
  REQUIRE(board.get<std::uint32_t>("param5") == 1500);

  board.erase("param1");
  board.erase("param4");

  REQUIRE(board.get<std::uint32_t>("param5") == 1500);
  REQUIRE(board.get<std::uint32_t>("param7") == 150);

  REQUIRE(board.get<std::string>("param2") == "number 1");
  REQUIRE(board.get<std::string>("param3") == "number 2");
  REQUIRE(board.get<std::string>("param6") == "number 4");

  REQUIRE(board.contains("param2"));
  REQUIRE(board.contains("param3"));
  REQUIRE(board.contains("param5"));
  REQUIRE(board.contains("param6"));
  REQUIRE(board.contains("param7"));

  board.erase("param7");

  REQUIRE(!board.contains("param1"));
  REQUIRE(board.contains("param2"));
  REQUIRE(board.contains("param3"));
  REQUIRE(!board.contains("param4"));
  REQUIRE(board.contains("param5"));
  REQUIRE(board.contains("param6"));
  REQUIRE(!board.contains("param7"));

  using typeidx_board = acl::blackboard<acl::cfg::map<std::unordered_map>>;
  typeidx_board board2;
  board2.emplace<std::pair<int, int>>(10, 10);
  REQUIRE(board2.get<std::pair<int, int>>() == std::pair<int, int>(10, 10));

  // check decl
  acl::blackboard<acl::config<acl::cfg::name_map<custom_map>>> map;
}
// NOLINTEND