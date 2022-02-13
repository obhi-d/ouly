#include <acl/blackboard.hpp>
#include <catch2/catch.hpp>

TEST_CASE("blackboard: push_back", "[blackboard][push_back]")
{
  acl::blackboard board;
  auto            index = board.emplace_safe<std::uint32_t>("param1", 50);
  REQUIRE(board.at<std::uint32_t>(index) == 50);
  REQUIRE(board.at<std::uint32_t>("param1") == 50);
  REQUIRE(board.contains("param1"));
  board.emplace_safe<std::uint32_t>("param1", 150);
  REQUIRE(board.at<std::uint32_t>(index) == 150);
  board.erase<std::uint32_t>("param1");
  REQUIRE(board.contains("param1") == false);
}