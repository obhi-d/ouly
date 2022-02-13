#include <acl/inventory.hpp>
#include <catch2/catch.hpp>

TEST_CASE("inventory: push_back", "[inventory][push_back]")
{
  acl::inventory board;
  auto           index = board.emplace_safe<std::uint32_t>("param1", 50);
  REQUIRE(board.at<std::uint32_t>(index) == 50);
  REQUIRE(board.at<std::uint32_t>("param1") == 50);
  REQUIRE(board.contains("param1"));
  board.emplace_safe<std::uint32_t>("param1", 150);
  REQUIRE(board.at<std::uint32_t>(index) == 150);
  board.erase<std::uint32_t>("param1");
  std::shared_ptr<std::uint32_t> shared_int = std::make_shared<std::uint32_t>(50);
  board.emplace_safe<std::shared_ptr<std::uint32_t>>("shared1", shared_int);
  REQUIRE(board.at<std::shared_ptr<std::uint32_t>>("shared1").get() == shared_int.get());
  REQUIRE(shared_int.use_count() == 2);
  board.erase<std::shared_ptr<std::uint32_t>>("shared1");
  REQUIRE(shared_int.use_count() == 1);
  auto l = board.emplace_safe<std::string>("string1", "string");
  REQUIRE(board.contains("string1"));
  REQUIRE(board.at<std::string>("string1") == "string");
}