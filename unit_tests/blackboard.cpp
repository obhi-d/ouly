#include "ouly/containers/blackboard.hpp"
#include "catch2/catch_all.hpp"
#include <string>
#include <unordered_map>

// NOLINTBEGIN
template <typename V>
using custom_map = std::unordered_map<std::string, V>;

TEST_CASE("blackboard: push_back", "[blackboard][push_back]")
{
  ouly::blackboard board;
  auto&            int_index0 = board.emplace<std::uint32_t>("param1", 50);
  auto&            str_index0 = board.emplace<std::string>("param2", "number 1");
  auto&            str_index1 = board.emplace<std::string>("param3", "number 2");
  auto&            str_index2 = board.emplace<std::string>("param4", "number 3");
  auto&            int_index1 = board.emplace<std::uint32_t>("param5", 100);
  auto&            str_index3 = board.emplace<std::string>("param6", "number 4");
  auto&            int_index2 = board.emplace<std::uint32_t>("param7", 150);

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

  using typeidx_board = ouly::blackboard<ouly::cfg::map<std::unordered_map>>;
  typeidx_board board2;
  board2.emplace<std::pair<int, int>>(10, 10);
  REQUIRE(board2.get<std::pair<int, int>>() == std::pair<int, int>(10, 10));

  // check decl
  ouly::blackboard<ouly::config<ouly::cfg::name_map<custom_map>>> map;
}

TEST_CASE("blackboard: Test swap functionality", "[blackboard][swap]")
{
  ouly::blackboard board1;
  board1.emplace<int>("param1", 100);
  board1.emplace<std::string>("param2", "hello");

  ouly::blackboard board2;
  board2.emplace<int>("param3", 200);
  board2.emplace<std::string>("param4", "world");
  board2.emplace<double>("param5", 3.14);

  // Test member swap
  board1.swap(board2);

  // Verify contents are swapped
  REQUIRE(board1.contains("param3"));
  REQUIRE(board1.contains("param4"));
  REQUIRE(board1.contains("param5"));
  REQUIRE(!board1.contains("param1"));
  REQUIRE(!board1.contains("param2"));

  REQUIRE(board2.contains("param1"));
  REQUIRE(board2.contains("param2"));
  REQUIRE(!board2.contains("param3"));
  REQUIRE(!board2.contains("param4"));
  REQUIRE(!board2.contains("param5"));

  REQUIRE(board1.get<int>("param3") == 200);
  REQUIRE(board1.get<std::string>("param4") == "world");
  REQUIRE(board1.get<double>("param5") == 3.14);

  REQUIRE(board2.get<int>("param1") == 100);
  REQUIRE(board2.get<std::string>("param2") == "hello");

  // Test friend swap function (swap back)
  swap(board1, board2);

  // Verify we're back to original state
  REQUIRE(board1.contains("param1"));
  REQUIRE(board1.contains("param2"));
  REQUIRE(!board1.contains("param3"));
  REQUIRE(!board1.contains("param4"));
  REQUIRE(!board1.contains("param5"));

  REQUIRE(board2.contains("param3"));
  REQUIRE(board2.contains("param4"));
  REQUIRE(board2.contains("param5"));
  REQUIRE(!board2.contains("param1"));
  REQUIRE(!board2.contains("param2"));

  REQUIRE(board1.get<int>("param1") == 100);
  REQUIRE(board1.get<std::string>("param2") == "hello");

  REQUIRE(board2.get<int>("param3") == 200);
  REQUIRE(board2.get<std::string>("param4") == "world");
  REQUIRE(board2.get<double>("param5") == 3.14);
}
// NOLINTEND