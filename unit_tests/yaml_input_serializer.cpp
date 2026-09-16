
#include "catch2/catch_all.hpp"
#include "ouly/reflection/detail/base_concepts.hpp"
#include "ouly/reflection/visitor.hpp"
#include "ouly/serializers/lite_yml.hpp"
#include <array>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <variant>
#include <vector>

// NOLINTBEGIN
struct TestStruct
{
  int         a;
  double      b;
  std::string c;

  static constexpr auto reflect() noexcept
  {
    return ouly::bind(ouly::bind<"a", &TestStruct::a>(), ouly::bind<"b", &TestStruct::b>(),
                      ouly::bind<"c", &TestStruct::c>());
  }
};

TEST_CASE("yaml parser: inline comments and scalar whitespace", "[yaml][regression]")
{
  struct Document
  {
    std::string text;
    int         number = 0;
    std::string hash;
  };
  for (auto const* text : {"hello", "\"hello\"", "'hello'"})
  {
    Document doc;
    auto     yaml = std::string("text : ") + text + "   # comment\r\nnumber: 42   # number\r\nhash: hello#world   \r\n";
    CAPTURE(yaml);
    ouly::yml::from_string(doc, yaml);
    REQUIRE(doc.text == "hello");
    REQUIRE(doc.number == 42);
    REQUIRE(doc.hash == "hello#world");
  }

  Document doc;
  ouly::yml::from_string(doc, "text: hello   \nnumber : 42   \nhash: '# literal' # trailing comment");
  REQUIRE(doc.text == "hello");
  REQUIRE(doc.number == 42);
  REQUIRE(doc.hash == "# literal");
}

TEST_CASE("yaml parser: quoted keys and values", "[yaml][regression]")
{
  struct Document
  {
    std::string text;
    std::string other;
  };
  Document doc;
  ouly::yml::from_string(doc, R"("text" : "say \"hi\"" # comment
'other': 'it''s fine'
)");
  REQUIRE(doc.text == "say \"hi\"");
  REQUIRE(doc.other == "it's fine");

  ouly::yml::from_string(doc, R"(text: "line\nnext\tcolumn\\path"
other: 'literal\nvalue'
)");
  REQUIRE(doc.text == "line\nnext\tcolumn\\path");
  REQUIRE(doc.other == "literal\\nvalue");

  ouly::yml::from_string(doc, "text: '' # empty\nother: \"\" # empty");
  REQUIRE(doc.text.empty());
  REQUIRE(doc.other.empty());
}

TEST_CASE("yaml parser: flow strings and comments", "[yaml][regression]")
{
  struct Document
  {
    std::vector<std::string> values;
    int                      after = 0;
  };
  Document doc;
  ouly::yml::from_string(doc,
                         "values: [hello world, # first\n  goodbye moon, 'a,b', \"x]y\", '', hello#world]\nafter: 7\n");
  REQUIRE(doc.values == std::vector<std::string>{"hello world", "goodbye moon", "a,b", "x]y", "", "hello#world"});
  REQUIRE(doc.after == 7);
}

TEST_CASE("yaml parser: dash spacing does not change sequence nesting", "[yaml][regression]")
{
  struct Document
  {
    std::vector<std::string> values;
    int                      after = 0;
  };
  Document doc;
  ouly::yml::from_string(doc, "values:\n  - one\n  -   two\n  - three\nafter: 7\n");
  REQUIRE(doc.values == std::vector<std::string>{"one", "two", "three"});
  REQUIRE(doc.after == 7);

  std::vector<int> root;
  ouly::yml::from_string(root, "- -1\n-   -2\n- -3");
  REQUIRE(root == std::vector<int>{-1, -2, -3});
}

TEST_CASE("yaml parser: nested sequence forms", "[yaml][regression]")
{
  struct Document
  {
    std::vector<std::vector<int>> values;
    int                           after = 0;
  };
  for (auto const* yaml :
       {"values:\n  - - 1\n    - 2\n  - - 3\n    - 4\nafter: 7\n",
        "values:\n  -\n    - 1\n    - 2\n  -\n    - 3\n    - 4\nafter: 7\n", "values: [[1, 2], [3, 4]]\nafter: 7\n"})
  {
    CAPTURE(yaml);
    Document doc;
    ouly::yml::from_string(doc, yaml);
    REQUIRE(doc.values == std::vector<std::vector<int>>{
                           {1, 2},
                           {3, 4}
    });
    REQUIRE(doc.after == 7);
  }
}

TEST_CASE("yaml parser: sequence object fields align with content", "[yaml][regression]")
{
  struct Item
  {
    std::string name;
    int         value = 0;
  };
  std::vector<Item> items;
  ouly::yml::from_string(items, "- name: one\n  value: 1\n-   name: two\n    value: 2\n- name: three\n  value: 3\n");
  REQUIRE(items.size() == 3);
  REQUIRE(items[0].name == "one");
  REQUIRE(items[0].value == 1);
  REQUIRE(items[1].name == "two");
  REQUIRE(items[1].value == 2);
  REQUIRE(items[2].name == "three");
  REQUIRE(items[2].value == 3);
}

TEST_CASE("yaml parser: block scalars stop at dedentation", "[yaml][regression]")
{
  struct Document
  {
    std::string text;
    int         after = 0;
  };
  for (auto const* yaml :
       {"text: |\n  hello\nafter: 7\n", "text: > # folded\n  hello\nafter: 7\n", "text: |\r\n  hello\r\nafter: 7\r\n"})
  {
    CAPTURE(yaml);
    Document doc;
    ouly::yml::from_string(doc, yaml);
    REQUIRE(doc.text == "hello");
    REQUIRE(doc.after == 7);
  }
  Document doc;
  ouly::yml::from_string(doc, "text: |\n  x\n  y\n\nafter: 7\n");
  REQUIRE(doc.text == "x\ny");
  REQUIRE(doc.after == 7);
}

TEST_CASE("yaml parser: block scalars retain content at EOF", "[yaml][regression]")
{
  struct Document
  {
    std::string text;
  };
  for (auto const* yaml : {"text: |\n  hello", "text: |\n  hello\n", "text: >\n  hello", "text: >\n  hello\n"})
  {
    CAPTURE(yaml);
    Document doc;
    ouly::yml::from_string(doc, yaml);
    REQUIRE(doc.text == "hello");
  }
  for (auto const* yaml : {"text: |", "text: > # empty", "text: |\n\n"})
  {
    CAPTURE(yaml);
    Document doc{"original"};
    ouly::yml::from_string(doc, yaml);
    REQUIRE(doc.text.empty());
  }
}

TEST_CASE("yaml parser: block scalar blank lines and extra indentation", "[yaml][regression]")
{
  struct Document
  {
    std::string text;
    int         after = 0;
  };
  Document doc;
  ouly::yml::from_string(doc, "text: |\n  first\n\n    indented\n  # literal\nafter: 7\n");
  REQUIRE(doc.text == "first\n\n  indented\n# literal");
  REQUIRE(doc.after == 7);
  ouly::yml::from_string(doc, "text: >\n  first\n  second\n\n  paragraph\nafter: 8\n");
  REQUIRE(doc.text == "first second\nparagraph");
  REQUIRE(doc.after == 8);
}

TEST_CASE("yaml parser: block scalars inside sequences", "[yaml][regression]")
{
  struct Document
  {
    std::vector<std::string> values;
    int                      after = 0;
  };
  Document doc;
  ouly::yml::from_string(doc,
                         "values:\n  - |\n    first\n    second\n  - >\n    third\n    fourth\n  - last\nafter: 7\n");
  REQUIRE(doc.values == std::vector<std::string>{"first\nsecond", "third fourth", "last"});
  REQUIRE(doc.after == 7);
}

TEST_CASE("yaml parser: nested empty sequences are values", "[yaml][regression]")
{
  struct Document
  {
    std::vector<std::vector<int>> values;
    int                           after = 0;
  };
  for (auto const* yaml : {"values: [[], [1], []]\nafter: 7\n", "values:\n  - []\n  - [1]\n  - []\nafter: 7\n"})
  {
    CAPTURE(yaml);
    Document doc;
    ouly::yml::from_string(doc, yaml);
    REQUIRE(doc.values == std::vector<std::vector<int>>{{}, {1}, {}});
    REQUIRE(doc.after == 7);
  }
}

TEST_CASE("yaml parser: extra content after a root scalar is rejected", "[yaml][regression]")
{
  for (auto const* yaml : {"'hello'\nother: value\n", "'hello'\n'world'\n", "'hello'\n- item\n", "'hello'\n[]\n"})
  {
    CAPTURE(yaml);
    std::string value;
    REQUIRE_THROWS_AS(ouly::yml::from_string(value, yaml), std::runtime_error);
  }
}

TEST_CASE("yaml parser: truncated quoted scalars are rejected", "[yaml][regression]")
{
  struct Document
  {
    std::string text;
  };
  for (auto const* yaml : {"text: \"hello", "text: 'hello", "text: \"", "text: '\n", "text: \"hello\\",
                           "text: \"hello\\q\"", "text: \"hello\" garbage", "text: | trailing\n  hello\n"})
  {
    CAPTURE(yaml);
    Document doc;
    REQUIRE_THROWS_AS(ouly::yml::from_string(doc, yaml), std::runtime_error);
  }
}

TEST_CASE("yaml parser: incomplete and malformed flow sequences are rejected", "[yaml][regression]")
{
  struct Document
  {
    std::vector<std::vector<int>> values;
  };
  for (auto const* yaml : {"values: [", "values: [[1, 2]", "values: [[1, 2", "values: [[1],\n", "values: [[1] [2]]",
                           "values: [,]", "values: [[1],, [2]]"})
  {
    CAPTURE(yaml);
    Document doc;
    REQUIRE_THROWS_AS(ouly::yml::from_string(doc, yaml), std::runtime_error);
  }
}

TEST_CASE("yaml_object: Test read")
{
  std::string yml = R"(
a: 100
b: 200.0
c: "value"
)";

  TestStruct ts;
  ouly::yml::from_string(ts, yml);

  REQUIRE(ts.a == 100);
  REQUIRE(ts.b == 200.0);
  REQUIRE(ts.c == "value");
}

struct TestStruct2
{
  int                   a;
  double                b;
  std::string           c;
  TestStruct            d;
  static constexpr auto reflect() noexcept
  {
    return ouly::bind(ouly::bind<"a", &TestStruct2::a>(), ouly::bind<"b", &TestStruct2::b>(),
                      ouly::bind<"c", &TestStruct2::c>(), ouly::bind<"d", &TestStruct2::d>());
  }
};

TEST_CASE("yaml_object: Test simple nested")
{
  struct aggregate
  {
    int value = -1;
  };
  struct instance
  {
    int       parent = -1;
    aggregate child;
  };
  std::string yml = R"(
parent: 100
child:
  value: 300
)";
  instance    ts;
  ouly::yml::from_string(ts, yml);
  REQUIRE(ts.parent == 100);
  REQUIRE(ts.child.value == 300);
}

TEST_CASE("yaml_object: Test read nested")
{
  std::string yml = R"(
a: 100
b: 200.0
c: "value"
d:
  a: 300
  b: 400.0
  c: "value2"
)";
  TestStruct2 ts;
  ouly::yml::from_string(ts, yml);
  REQUIRE(ts.a == 100);
  REQUIRE(ts.b == 200.0);
  REQUIRE(ts.c == "value");
  REQUIRE(ts.d.a == 300);
  REQUIRE(ts.d.b == 400.0);
  REQUIRE(ts.d.c == "value2");
}

TEST_CASE("yaml_object: Test read vector")
{
  std::string yml = R"(
numbers:
  - 1
  - 2
  - 3
)";
  struct TestStructVector
  {
    std::vector<int>      numbers;
    static constexpr auto reflect() noexcept
    {
      return ouly::bind(ouly::bind<"numbers", &TestStructVector::numbers>());
    }
  };

  TestStructVector ts;
  ouly::yml::from_string(ts, yml);

  REQUIRE(ts.numbers.size() == 3);
  REQUIRE(ts.numbers[0] == 1);
  REQUIRE(ts.numbers[1] == 2);
  REQUIRE(ts.numbers[2] == 3);
}

TEST_CASE("yaml_object: Test read aggregate array items containing arrays")
{
  struct Item
  {
    int              index = 0;
    std::vector<int> values;
    int              count = 0;
  };
  struct Document
  {
    std::vector<Item> items;
  };
  std::string yml = R"(
items:
  - index: 0
    values:
      - 1
      - 2
    count: 2
  - index: 1
    values: []
    count: 0
)";

  Document document;
  ouly::yml::from_string(document, yml);

  REQUIRE(document.items.size() == 2);
  REQUIRE(document.items[0].index == 0);
  REQUIRE(document.items[0].values == std::vector{1, 2});
  REQUIRE(document.items[0].count == 2);
  REQUIRE(document.items[1].index == 1);
  REQUIRE(document.items[1].values.empty());
  REQUIRE(document.items[1].count == 0);
}

TEST_CASE("yaml_object: Test read optional")
{
  std::string yaml_present = R"(
value: 42
)";

  std::string yaml_absent = R"(
)";

  struct TestStructOptional
  {
    std::optional<int>    value;
    static constexpr auto reflect() noexcept
    {
      return ouly::bind(ouly::bind<"value", &TestStructOptional::value>());
    }
  };

  // Test with value present
  {
    TestStructOptional ts;
    ouly::yml::from_string(ts, yaml_present);
    REQUIRE(ts.value.has_value());
    REQUIRE(ts.value.value() == 42);
  }

  // Test with value absent
  {
    TestStructOptional ts;
    ouly::yml::from_string(ts, yaml_absent);
    REQUIRE(!ts.value.has_value());
  }
}

enum class Color
{
  Red   = 0,
  Green = 1,
  Blue  = 2
};

TEST_CASE("yaml_object: Test read enum")
{
  std::string yml = R"(
color: 1
)";

  struct TestStructEnum
  {
    Color                 color;
    static constexpr auto reflect() noexcept
    {
      return ouly::bind(ouly::bind<"color", &TestStructEnum::color>());
    }
  };

  TestStructEnum ts;
  ouly::yml::from_string(ts, yml);

  REQUIRE(ts.color == Color::Green);
}

TEST_CASE("yaml_object: Test read pointer")
{
  std::string yaml_present = R"(
value: 42
)";

  std::string yaml_null = R"(
value: null
)";

  struct TestStructPointer
  {
    std::unique_ptr<int>  value;
    static constexpr auto reflect() noexcept
    {
      return ouly::bind(ouly::bind<"value", &TestStructPointer::value>());
    }
  };

  // Test with value present
  {
    TestStructPointer ts;
    ouly::yml::from_string(ts, yaml_present);
    REQUIRE(ts.value != nullptr);
    REQUIRE(*ts.value == 42);
  }

  // Test with value null
  {
    TestStructPointer ts;
    ouly::yml::from_string(ts, yaml_null);
    REQUIRE(ts.value == nullptr);
  }
}

TEST_CASE("yaml_object: Test read tuple")
{
  std::string yml = R"(
tuple:
  - 1
  - "string"
  - 3.14
)";

  struct TestStructTuple
  {
    std::tuple<int, std::string, double> tuple;
    static constexpr auto                reflect() noexcept
    {
      return ouly::bind(ouly::bind<"tuple", &TestStructTuple::tuple>());
    }
  };

  TestStructTuple ts;
  ouly::yml::from_string(ts, yml);

  REQUIRE(std::get<0>(ts.tuple) == 1);
  REQUIRE(std::get<1>(ts.tuple) == "string");
  REQUIRE(std::get<2>(ts.tuple) == 3.14);
}

TEST_CASE("yaml_object: Test read array")
{
  std::string yml = R"(
array:
  - 10
  - 20
  - 30
)";

  struct TestStructArray
  {
    std::array<int, 3>    array;
    static constexpr auto reflect() noexcept
    {
      return ouly::bind(ouly::bind<"array", &TestStructArray::array>());
    }
  };

  TestStructArray ts;
  ouly::yml::from_string(ts, yml);

  REQUIRE(ts.array[0] == 10);
  REQUIRE(ts.array[1] == 20);
  REQUIRE(ts.array[2] == 30);
}

TEST_CASE("yaml_object: Test read compact array ")
{
  std::string yml = R"(
array: [10, 20, 30]
)";

  struct TestStructArray
  {
    std::array<int, 3>    array;
    static constexpr auto reflect() noexcept
    {
      return ouly::bind(ouly::bind<"array", &TestStructArray::array>());
    }
  };

  TestStructArray ts;
  ouly::yml::from_string(ts, yml);

  REQUIRE(ts.array[0] == 10);
  REQUIRE(ts.array[1] == 20);
  REQUIRE(ts.array[2] == 30);
}

TEST_CASE("yaml_object: Test read boolean")
{
  std::string yml = R"(
flag1: true
flag2: false
)";

  struct TestStructBool
  {
    bool                  flag1;
    bool                  flag2;
    static constexpr auto reflect() noexcept
    {
      return ouly::bind(ouly::bind<"flag1", &TestStructBool::flag1>(), ouly::bind<"flag2", &TestStructBool::flag2>());
    }
  };

  TestStructBool ts;
  ouly::yml::from_string(ts, yml);

  REQUIRE(ts.flag1 == true);
  REQUIRE(ts.flag2 == false);
}

TEST_CASE("yaml_object: Test read variant")
{
  std::string yaml_int = R"(
var:
  type: 0
  value: 42
)";

  std::string yaml_string = R"(
var:
  type: 1
  value: "hello"
)";

  using VarType = std::variant<int, std::string>;

  struct TestStructVariant
  {
    VarType               var;
    static constexpr auto reflect() noexcept
    {
      return ouly::bind(ouly::bind<"var", &TestStructVariant::var>());
    }
  };

  {
    TestStructVariant ts;
    ouly::yml::from_string(ts, yaml_int);
    REQUIRE(std::holds_alternative<int>(ts.var));
    REQUIRE(std::get<int>(ts.var) == 42);
  }

  {
    TestStructVariant ts;
    ouly::yml::from_string(ts, yaml_string);
    REQUIRE(std::holds_alternative<std::string>(ts.var));
    REQUIRE(std::get<std::string>(ts.var) == "hello");
  }
}

TEST_CASE("yaml_object: Test read invalid YAML")
{
  std::string yml = R"(
a: 100
b: [1, 2, 3
c: "value
)";

  struct TestStructInvalid
  {
    int              a;
    std::vector<int> b;
    std::string      c;

    static constexpr auto reflect() noexcept
    {
      return ouly::bind(ouly::bind<"a", &TestStructInvalid::a>(), ouly::bind<"b", &TestStructInvalid::b>(),
                        ouly::bind<"c", &TestStructInvalid::c>());
    }
  };

  TestStructInvalid ts;
  REQUIRE_THROWS_AS(ouly::yml::from_string(ts, yml), std::runtime_error);
}

TEST_CASE("yaml_object: Test read block scalar literals")
{
  std::string yml = R"(
literal_block: |
  This is a block of text
  that spans multiple lines.

folded_block: >
  This is another block
  that folds newlines
  into spaces.
)";

  struct TestStructBlockScalar
  {
    std::string literal_block;
    std::string folded_block;

    static constexpr auto reflect() noexcept
    {
      return ouly::bind(ouly::bind<"literal_block", &TestStructBlockScalar::literal_block>(),
                        ouly::bind<"folded_block", &TestStructBlockScalar::folded_block>());
    }
  };

  TestStructBlockScalar ts;
  ouly::yml::from_string(ts, yml);

  REQUIRE(ts.literal_block == "This is a block of text\nthat spans multiple lines.");
  REQUIRE(ts.folded_block == "This is another block that folds newlines into spaces.");
}

TEST_CASE("yaml_object: Test read with unexpected token")
{
  std::string yml = R"(
list:
  - 1
  - 2
  - x3
)";

  struct TestStructUnexpectedToken
  {
    std::vector<int> list;

    static constexpr auto reflect() noexcept
    {
      return ouly::bind(ouly::bind<"list", &TestStructUnexpectedToken::list>());
    }
  };

  TestStructUnexpectedToken ts;
  REQUIRE_THROWS_AS(ouly::yml::from_string(ts, yml), std::runtime_error);
}

TEST_CASE("yaml_object: Test read with missing key")
{
  std::string yml = R"(
a: 100
c: "value"
)";

  struct TestStructMissingKey
  {
    int         a;
    int         b = -100; // Missing in YAML
    std::string c;

    static constexpr auto reflect() noexcept
    {
      return ouly::bind(ouly::bind<"a", &TestStructMissingKey::a>(), ouly::bind<"b", &TestStructMissingKey::b>(),
                        ouly::bind<"c", &TestStructMissingKey::c>());
    }
  };

  TestStructMissingKey ts;
  ouly::yml::from_string(ts, yml);

  REQUIRE(ts.a == 100);
  REQUIRE(ts.b == -100); // value untouched
  REQUIRE(ts.c == "value");
}

TEST_CASE("yaml_object: Test read with extra fields")
{
  std::string yml = R"(
a: 100
b: 200
c: "value"
extra_field: "cause a crash"
)";

  struct TestStructExtraField
  {
    int         a;
    int         b;
    std::string c;

    static constexpr auto reflect() noexcept
    {
      return ouly::bind(ouly::bind<"a", &TestStructExtraField::a>(), ouly::bind<"b", &TestStructExtraField::b>(),
                        ouly::bind<"c", &TestStructExtraField::c>());
    }
  };

  TestStructExtraField ts;
  REQUIRE_THROWS_AS(ouly::yml::from_string(ts, yml), ouly::visitor_error);
}

TEST_CASE("yaml_object: Test read of unexpected type")
{
  std::string yml = R"(
number: "not_a_number"
)";

  struct TestStructUnexpectedType
  {
    int number;

    static constexpr auto reflect() noexcept
    {
      return ouly::bind(ouly::bind<"number", &TestStructUnexpectedType::number>());
    }
  };

  TestStructUnexpectedType ts;
  REQUIRE_THROWS_AS(ouly::yml::from_string(ts, yml), std::runtime_error);
}

TEST_CASE("yaml_object: Test read recursive structures")
{
  std::string yml = R"(
node:
  value: 1
  next:
    value: 2
    next:
      value: 3
)";

  struct Node
  {
    int                   value;
    std::unique_ptr<Node> next;

    static constexpr auto reflect() noexcept
    {
      return ouly::bind(ouly::bind<"value", &Node::value>(), ouly::bind<"next", &Node::next>());
    }
  };

  struct TestStructRecursive
  {
    Node node;

    static constexpr auto reflect() noexcept
    {
      return ouly::bind(ouly::bind<"node", &TestStructRecursive::node>());
    }
  };

  TestStructRecursive ts;
  ouly::yml::from_string(ts, yml);

  REQUIRE(ts.node.value == 1);
  REQUIRE(ts.node.next->value == 2);
  REQUIRE(ts.node.next->next->value == 3);
  REQUIRE(ts.node.next->next->next == nullptr);
}

TEST_CASE("yaml_object: Test read with incorrect type casting")
{
  std::string yml = R"(
value: "string_instead_of_int"
)";

  struct TestStructTypeCast
  {
    int value;

    static constexpr auto reflect() noexcept
    {
      return ouly::bind(ouly::bind<"value", &TestStructTypeCast::value>());
    }
  };

  TestStructTypeCast ts;
  REQUIRE_THROWS_AS(ouly::yml::from_string(ts, yml), std::runtime_error);
}

TEST_CASE("yaml_object: Test read with empty YAML")
{
  std::string yml = "";

  struct TestStructEmpty
  {
    int a = -1;

    static constexpr auto reflect() noexcept
    {
      return ouly::bind(ouly::bind<"a", &TestStructEmpty::a>());
    }
  };

  TestStructEmpty ts;
  ouly::yml::from_string(ts, yml);

  // Since YAML is empty, the value should remain default-initialized
  REQUIRE(ts.a == -1);
}

TEST_CASE("yaml_object: Test read with null value")
{
  std::string yml = R"(
value: null
)";

  struct TestStructNull
  {
    std::optional<int> value = 10;

    static constexpr auto reflect() noexcept
    {
      return ouly::bind(ouly::bind<"value", &TestStructNull::value>());
    }
  };

  TestStructNull ts;
  ouly::yml::from_string(ts, yml);

  // The optional should not be set
  REQUIRE(!ts.value.has_value());
}

TEST_CASE("yaml_object: Test read large numbers")
{
  std::string yml = R"(
int_max: 9223372036854775807
int_min: -9223372036854775808
uint_max: 18446744073709551615
)";

  struct TestStructLargeNumbers
  {
    int64_t  int_max;
    int64_t  int_min;
    uint64_t uint_max;

    static constexpr auto reflect() noexcept
    {
      return ouly::bind(ouly::bind<"int_max", &TestStructLargeNumbers::int_max>(),
                        ouly::bind<"int_min", &TestStructLargeNumbers::int_min>(),
                        ouly::bind<"uint_max", &TestStructLargeNumbers::uint_max>());
    }
  };

  TestStructLargeNumbers ts;
  ouly::yml::from_string(ts, yml);

  REQUIRE(ts.int_max == INT64_MAX);
  REQUIRE(ts.int_min == INT64_MIN);
  REQUIRE(ts.uint_max == UINT64_MAX);
}

TEST_CASE("yaml_object: Test read floating-point edge cases")
{
  std::string yml = R"(
positive_infinity: .inf
negative_infinity: -.inf
not_a_number: .nan
)";

  struct TestStructFloatEdgeCases
  {
    double positive_infinity;
    double negative_infinity;
    double not_a_number;

    static constexpr auto reflect() noexcept
    {
      return ouly::bind(ouly::bind<"positive_infinity", &TestStructFloatEdgeCases::positive_infinity>(),
                        ouly::bind<"negative_infinity", &TestStructFloatEdgeCases::negative_infinity>(),
                        ouly::bind<"not_a_number", &TestStructFloatEdgeCases::not_a_number>());
    }
  };

  TestStructFloatEdgeCases ts;
  ouly::yml::from_string(ts, yml);

  REQUIRE(std::isinf(ts.positive_infinity));
  REQUIRE(ts.positive_infinity > 0);

  REQUIRE(std::isinf(ts.negative_infinity));
  REQUIRE(ts.negative_infinity < 0);

  REQUIRE(std::isnan(ts.not_a_number));
}

TEST_CASE("yaml_object: Test read deeply nested structures")
{
  std::string yml = R"(
level1:
  level2:
    level3:
      level4:
        value: 42
)";

  struct Level4
  {
    int value;

    static constexpr auto reflect() noexcept
    {
      return ouly::bind(ouly::bind<"value", &Level4::value>());
    }
  };

  struct Level3
  {
    Level4 level4;

    static constexpr auto reflect() noexcept
    {
      return ouly::bind(ouly::bind<"level4", &Level3::level4>());
    }
  };

  struct Level2
  {
    Level3 level3;

    static constexpr auto reflect() noexcept
    {
      return ouly::bind(ouly::bind<"level3", &Level2::level3>());
    }
  };

  struct Level1
  {
    Level2 level2;

    static constexpr auto reflect() noexcept
    {
      return ouly::bind(ouly::bind<"level2", &Level1::level2>());
    }
  };

  struct Level0
  {
    Level1 level1;
  };

  Level0 ts;
  ouly::yml::from_string(ts, yml);

  REQUIRE(ts.level1.level2.level3.level4.value == 42);
}

TEST_CASE("yaml_object: Test read sequence of maps with single key")
{
  std::string yml = R"(
- name: "Item1"
- name: "Item2"
- name: "Item3"
)";

  struct Item
  {
    std::string name;
  };

  using ItemsList = std::vector<Item>;
  ItemsList items;

  ouly::yml::from_string(items, yml);

  REQUIRE(items.size() == 3);
  REQUIRE(items[0].name == "Item1");
  REQUIRE(items[1].name == "Item2");
  REQUIRE(items[2].name == "Item3");
}

TEST_CASE("yaml_object: Test read sequence of maps")
{
  std::string yml = R"(
- name: "Item1"
  value: 10
- name: "Item2"
  value: 20
- name: "Item3"
  value: 30
)";

  struct Item
  {
    std::string name;
    int         value;

    static constexpr auto reflect() noexcept
    {
      return ouly::bind(ouly::bind<"name", &Item::name>(), ouly::bind<"value", &Item::value>());
    }
  };

  using ItemsList = std::vector<Item>;
  ItemsList items;

  ouly::yml::from_string(items, yml);

  REQUIRE(items.size() == 3);
  REQUIRE(items[0].name == "Item1");
  REQUIRE(items[0].value == 10);
  REQUIRE(items[1].name == "Item2");
  REQUIRE(items[1].value == 20);
  REQUIRE(items[2].name == "Item3");
  REQUIRE(items[2].value == 30);
}

TEST_CASE("yaml_object: Test read with duplicate keys")
{
  std::string yml = R"(
a: 1
a: 2
)";

  struct TestStructDuplicateKeys
  {
    int a;

    static constexpr auto reflect() noexcept
    {
      return ouly::bind(ouly::bind<"a", &TestStructDuplicateKeys::a>());
    }
  };

  TestStructDuplicateKeys ts;
  ouly::yml::from_string(ts, yml);

  // Expect the last value to override
  REQUIRE(ts.a == 2);
}

TEST_CASE("yaml_object: Test read map types")
{
  std::string yml = R"(
map:
  - [1, "one"]
  - [2, "two"]
  - [3, "three"]
)";

  struct TestStructMap
  {
    std::map<int, std::string> map;
  };

  TestStructMap ts;
  ouly::yml::from_string(ts, yml);

  REQUIRE(ts.map.size() == 3);
  REQUIRE(ts.map[1] == "one");
  REQUIRE(ts.map[2] == "two");
  REQUIRE(ts.map[3] == "three");
}

TEST_CASE("yaml_object: Test read map types with int pair")
{
  std::string yml = R"(
map:
  - [1, 2]
  - [2, 3]
  - [3, 4]
)";

  struct TestStructMap
  {
    std::map<int, int> map;
  };

  TestStructMap ts;
  ouly::yml::from_string(ts, yml);

  REQUIRE(ts.map.size() == 3);
  REQUIRE(ts.map[1] == 2);
  REQUIRE(ts.map[2] == 3);
  REQUIRE(ts.map[3] == 4);
}

TEST_CASE("yaml_object: Test empty array")
{
  std::string yml = R"(
array: []
)";

  struct TestStructEmptyArray
  {
    std::vector<int> array;
  };

  TestStructEmptyArray ts;
  ouly::yml::from_string(ts, yml);

  REQUIRE(ts.array.empty());
}

// NOLINTEND
