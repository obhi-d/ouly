
#include <acl/dsl/macroxpr.hpp>
#include <catch2/catch_all.hpp>
#include <unordered_map>

TEST_CASE("Validate basic expressions", "[macroxpr]")
{
    std::unordered_map<std::string_view, int> table =
    {
        {"FIRST", 1},
        {"SECOND", 10}
    };

    acl::macroxpr expr([&table](std::string_view v) -> std::optional<int> 
    {
        auto it = table.find(v);
        if (it != table.end())
            return {it->second};
        return {};
    });

    REQUIRE(expr.evaluate("$FIRST && $SECOND") == true);
    REQUIRE(expr.evaluate("FIRST > 1") == false);
    REQUIRE(expr.evaluate("FIRST == 1") == true);
}