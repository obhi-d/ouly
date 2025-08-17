
#include "ouly/containers/intrusive_list.hpp"
#include "catch2/catch_all.hpp"
#include <array>
#include <string>

// NOLINTBEGIN
struct sobject
{
  std::string      value;
  ouly::slist_hook hook;

  sobject(std::string val = {}) : value(val) {}
};

struct object
{
  std::string     value;
  ouly::list_hook hook;

  object(std::string val = {}) : value(val) {}
};

TEMPLATE_TEST_CASE(
 "Validate intrusive_list basic", "[intrusive_list][basic]",

 (ouly::intrusive_list<&sobject::hook, true, true>), (ouly::intrusive_list<&sobject::hook, true, false>),
 (ouly::intrusive_list<&sobject::hook, false, true>), (ouly::intrusive_list<&sobject::hook, false, false>),
 (ouly::intrusive_list<&object::hook, true, true>), (ouly::intrusive_list<&object::hook, true, false>),
 (ouly::intrusive_list<&object::hook, false, true>), (ouly::intrusive_list<&object::hook, false, false>)

)
{
  TestType il;
  using value_type = TestType::value_type;
  std::array arr   = {value_type("a"), value_type("b"), value_type("c"), value_type("d")};
  for (auto& v : arr)
    il.push_front(v);
  REQUIRE(&il.front() == &arr.back());
  if constexpr (TestType::has_tail)
    REQUIRE(&il.back() == &arr.front());
  il.clear();
  REQUIRE(il.empty() == true);
  REQUIRE(il.size() == 0);
}

TEMPLATE_TEST_CASE("Validate intrusive_list reverse_iterator", "[intrusive_list][reverse_iterator]",

                   (ouly::intrusive_list<&object::hook, true, true>), (ouly::intrusive_list<&object::hook, false, true>)

)
{
  TestType il;
  using value_type = TestType::value_type;
  std::array arr   = {value_type("a"), value_type("b"), value_type("c"), value_type("d")};
  for (auto& v : arr)
    il.push_front(v);

  auto b = arr.begin();
  for (auto rb = il.rbegin(), en = il.rend(); rb != en; ++rb, ++b)
  {
    REQUIRE(&(*rb) == &(*b));
  }
  REQUIRE(il.size() == 4);
}

TEMPLATE_TEST_CASE("Validate intrusive_list value ctor", "[intrusive_list][ctor]",

                   (ouly::intrusive_list<&object::hook, true, true>), (ouly::intrusive_list<&object::hook, false, true>)

)
{
  using value_type = TestType::value_type;
  std::array arr3  = {value_type("1"), value_type("2"), value_type("3"), value_type("4")};
  TestType   il3;
  for (auto& v : arr3)
    il3.push_front(v);

  il3.clear();
  REQUIRE(il3.empty() == true);

  il3 = TestType(arr3[3], arr3[0], 4);
  REQUIRE(il3.size() == 4);
  auto i = il3.begin();
  for (auto b = arr3.rbegin(), e = arr3.rend(); b != e; ++b, ++i)
  {
    REQUIRE(&(*i) == &(*b));
  }
}

TEMPLATE_TEST_CASE("Validate intrusive_list value ctor", "[intrusive_list][ctor]",

                   (ouly::intrusive_list<&sobject::hook, true, false>),
                   (ouly::intrusive_list<&sobject::hook, false, false>)

)
{
  using value_type = TestType::value_type;
  std::array arr3  = {value_type("1"), value_type("2"), value_type("3"), value_type("4")};
  TestType   il3;
  for (auto& v : arr3)
    il3.push_front(v);

  il3.clear();
  REQUIRE(il3.empty() == true);

  il3 = TestType(arr3[3], 4);
  REQUIRE(il3.size() == 4);
  auto i = il3.begin();
  for (auto b = arr3.rbegin(), e = arr3.rend(); b != e; ++b, ++i)
  {
    REQUIRE(&(*i) == &(*b));
  }
}

TEMPLATE_TEST_CASE("Validate intrusive_list push_back", "[intrusive_list][push_back]",

                   (ouly::intrusive_list<&sobject::hook, true, true>),
                   (ouly::intrusive_list<&sobject::hook, false, true>),
                   (ouly::intrusive_list<&object::hook, true, true>), (ouly::intrusive_list<&object::hook, false, true>)

)
{
  TestType il;
  using value_type = TestType::value_type;
  std::array arr   = {value_type("a"), value_type("b"), value_type("c"), value_type("d")};
  for (auto& v : arr)
    il.push_back(v);

  auto b = arr.begin();
  for (auto& i : il)
  {
    REQUIRE(&i == &(*b));
    b++;
  }
  REQUIRE(il.size() == 4);
}

TEMPLATE_TEST_CASE(
 "Validate intrusive_list push_front", "[intrusive_list][push_front]",

 (ouly::intrusive_list<&sobject::hook, true, true>), (ouly::intrusive_list<&sobject::hook, true, false>),
 (ouly::intrusive_list<&sobject::hook, false, true>), (ouly::intrusive_list<&sobject::hook, false, false>),
 (ouly::intrusive_list<&object::hook, true, true>), (ouly::intrusive_list<&object::hook, true, false>),
 (ouly::intrusive_list<&object::hook, false, true>), (ouly::intrusive_list<&object::hook, false, false>)

)
{
  TestType il;
  using value_type = TestType::value_type;
  std::array arr   = {value_type("a"), value_type("b"), value_type("c"), value_type("d")};
  for (auto& v : arr)
    il.push_front(v);

  auto b = arr.rbegin();
  for (auto& i : il)
  {
    REQUIRE(&i == &(*b));
    b++;
  }
  REQUIRE(il.size() == 4);
}

TEMPLATE_TEST_CASE("Validate intrusive_list append_front", "[intrusive_list][append_front]",

                   (ouly::intrusive_list<&object::hook, true, true>), (ouly::intrusive_list<&object::hook, false, true>)

)
{
  TestType il;
  using value_type = TestType::value_type;
  std::array arr   = {value_type("a"), value_type("b"), value_type("c"), value_type("d")};
  for (auto& v : arr)
    il.push_front(v);

  auto b = arr.rbegin();
  for (auto& i : il)
  {
    REQUIRE(&i == &(*b));
    b++;
  }
  REQUIRE(il.size() == 4);

  TestType il2;

  {
    il2.append_front(std::move(il));

    auto rbeg = arr.rbegin();
    for (auto& i : il2)
    {
      REQUIRE(&i == &(*rbeg));
      rbeg++;
    }
    REQUIRE(il2.size() == 4);
    REQUIRE(il.size() == 0);
    REQUIRE(il.empty() == true);
  }

  std::array arr2 = {value_type("e"), value_type("f"), value_type("g"), value_type("h")};

  for (auto& v : arr2)
    il.push_front(v);

  il2.append_front(std::move(il));
  auto i = il2.begin();
  for (auto rbeg = arr2.rbegin(), e = arr2.rend(); rbeg != e; ++rbeg, ++i)
  {
    REQUIRE(&(*i) == &(*rbeg));
  }
  for (auto rbeg = arr.rbegin(), e = arr.rend(); rbeg != e; ++rbeg, ++i)
  {
    REQUIRE(&(*i) == &(*rbeg));
  }
  REQUIRE(il2.size() == 8);
}

TEMPLATE_TEST_CASE("Validate intrusive_list append_back", "[intrusive_list][append_back]",

                   (ouly::intrusive_list<&object::hook, true, true>), (ouly::intrusive_list<&object::hook, false, true>)

)
{
  TestType il;
  using value_type = TestType::value_type;
  std::array arr   = {value_type("a"), value_type("b"), value_type("c"), value_type("d")};
  for (auto& v : arr)
    il.push_front(v);

  auto b = arr.rbegin();
  for (auto& i : il)
  {
    REQUIRE(&i == &(*b));
    b++;
  }
  REQUIRE(il.size() == 4);

  TestType il2;

  {
    il2.append_back(std::move(il));

    auto rbeg = arr.rbegin();
    for (auto& i : il2)
    {
      REQUIRE(&i == &(*rbeg));
      rbeg++;
    }
    REQUIRE(il2.size() == 4);
    REQUIRE(il.size() == 0);
    REQUIRE(il.empty() == true);
  }

  std::array arr2 = {value_type("e"), value_type("f"), value_type("g"), value_type("h")};

  for (auto& v : arr2)
    il.push_front(v);

  il2.append_back(std::move(il));
  auto i = il2.begin();
  for (auto rbeg = arr.rbegin(), e = arr.rend(); rbeg != e; ++rbeg, ++i)
  {
    REQUIRE(&(*i) == &(*rbeg));
  }
  for (auto rbeg = arr2.rbegin(), e = arr2.rend(); rbeg != e; ++rbeg, ++i)
  {
    REQUIRE(&(*i) == &(*rbeg));
  }
  REQUIRE(il2.size() == 8);
}

TEMPLATE_TEST_CASE(
 "Validate intrusive_list erase_after", "[intrusive_list][erase_after]",

 (ouly::intrusive_list<&sobject::hook, true, true>), (ouly::intrusive_list<&sobject::hook, true, false>),
 (ouly::intrusive_list<&sobject::hook, false, true>), (ouly::intrusive_list<&sobject::hook, false, false>),
 (ouly::intrusive_list<&object::hook, true, true>), (ouly::intrusive_list<&object::hook, true, false>),
 (ouly::intrusive_list<&object::hook, false, true>), (ouly::intrusive_list<&object::hook, false, false>)

)
{
  TestType il;
  using value_type = TestType::value_type;
  std::array arr   = {value_type("d"), value_type("c"), value_type("b"), value_type("a")};
  for (auto& v : arr)
    il.push_front(v);

  il.erase_after(arr[1]);

  REQUIRE(il.size() == 3);

  auto b = il.begin();
  REQUIRE((*b).value == "a");
  b++;
  REQUIRE((*b).value == "b");
  b++;
  REQUIRE((*b).value == "c");
  b++;
  REQUIRE(b == il.end());

  il.erase_after(arr[3]);
  REQUIRE(il.size() == 2);

  b = il.begin();
  REQUIRE((*b).value == "a");
  b++;
  REQUIRE((*b).value == "c");
  b++;
  REQUIRE(b == il.end());

  il.push_front(arr[0]);
  REQUIRE(il.size() == 3);

  il.erase_after(arr[3]);
  REQUIRE(il.size() == 2);

  b = il.begin();
  REQUIRE((*b).value == "d");
  b++;
  REQUIRE((*b).value == "a");
  b++;
  REQUIRE(b == il.end());

  il.push_front(arr[2]);

  b = il.begin();
  REQUIRE((*b).value == "b");
  b++;
  REQUIRE((*b).value == "d");
  b++;
  REQUIRE((*b).value == "a");
  b++;
  REQUIRE(b == il.end());

  il.pop_front();
  REQUIRE(il.size() == 2);
  b = il.begin();
  REQUIRE((*b).value == "d");
  b++;
  REQUIRE((*b).value == "a");
  b++;
  REQUIRE(b == il.end());
}

TEMPLATE_TEST_CASE(
 "Validate intrusive_list insert_after", "[intrusive_list][insert_after]",

 (ouly::intrusive_list<&sobject::hook, true, true>), (ouly::intrusive_list<&sobject::hook, true, false>),
 (ouly::intrusive_list<&sobject::hook, false, true>), (ouly::intrusive_list<&sobject::hook, false, false>),
 (ouly::intrusive_list<&object::hook, true, true>), (ouly::intrusive_list<&object::hook, true, false>),
 (ouly::intrusive_list<&object::hook, false, true>), (ouly::intrusive_list<&object::hook, false, false>)

)
{
  TestType il;
  using value_type = TestType::value_type;
  std::array arr   = {value_type("a"), value_type("b"), value_type("c"), value_type("d")};
  std::array arr2  = {value_type("1"), value_type("2"), value_type("3"), value_type("4")};

  for (auto& v : arr)
    il.push_front(v);

  for (int i = 3; i >= 0; --i)
    il.insert_after(arr[i], arr2[i]);

  REQUIRE(il.size() == 8);

  auto b = il.begin();
  REQUIRE((*b).value == "d");
  b++;
  REQUIRE((*b).value == "4");
  b++;
  REQUIRE((*b).value == "c");
  b++;
  REQUIRE((*b).value == "3");
  b++;
  REQUIRE((*b).value == "b");
  b++;
  REQUIRE((*b).value == "2");
  b++;
  REQUIRE((*b).value == "a");
  b++;
  REQUIRE((*b).value == "1");
  b++;
  REQUIRE(b == il.end());
}

TEMPLATE_TEST_CASE("Validate intrusive_list insert", "[intrusive_list][insert]",

                   (ouly::intrusive_list<&object::hook, true, true>),
                   (ouly::intrusive_list<&object::hook, true, false>),
                   (ouly::intrusive_list<&object::hook, false, true>),
                   (ouly::intrusive_list<&object::hook, false, false>)

)
{
  TestType il;
  using value_type = TestType::value_type;
  std::array arr   = {value_type("a"), value_type("b"), value_type("c"), value_type("d")};
  std::array arr2  = {value_type("1"), value_type("2"), value_type("3"), value_type("4")};

  for (auto& v : arr)
    il.push_front(v);

  for (std::size_t i = 0; i < 4; ++i)
    il.insert(arr[i], arr2[i]);

  REQUIRE(il.size() == 8);

  auto b = il.begin();
  REQUIRE((*b).value == "4");
  b++;
  REQUIRE((*b).value == "d");
  b++;
  REQUIRE((*b).value == "3");
  b++;
  REQUIRE((*b).value == "c");
  b++;
  REQUIRE((*b).value == "2");
  b++;
  REQUIRE((*b).value == "b");
  b++;
  REQUIRE((*b).value == "1");
  b++;
  REQUIRE((*b).value == "a");
  b++;
  REQUIRE(b == il.end());
}

TEMPLATE_TEST_CASE("Validate intrusive_list append", "[intrusive_list][append]",

                   (ouly::intrusive_list<&object::hook, true, true>), (ouly::intrusive_list<&object::hook, false, true>)

)
{
  TestType il;
  using value_type = TestType::value_type;
  std::array arr   = {value_type("a"), value_type("b"), value_type("c"), value_type("d")};
  std::array arr2  = {value_type("1"), value_type("2"), value_type("3"), value_type("4")};

  for (auto& v : arr)
    il.push_front(v);

  TestType il2;
  for (auto& v : arr2)
    il2.push_front(v);

  il.append(il.begin(), std::move(il2));

  REQUIRE(il.size() == 8);

  auto b = il.begin();
  REQUIRE((*b).value == "4");
  b++;
  REQUIRE((*b).value == "3");
  b++;
  REQUIRE((*b).value == "2");
  b++;
  REQUIRE((*b).value == "1");
  b++;
  REQUIRE((*b).value == "d");
  b++;
  REQUIRE((*b).value == "c");
  b++;
  REQUIRE((*b).value == "b");
  b++;
  REQUIRE((*b).value == "a");
  b++;
  REQUIRE(b == il.end());

  il.clear();
  il2.clear();

  for (auto& v : arr)
    il.push_front(v);

  for (auto& v : arr2)
    il2.push_front(v);

  il.append(arr[2], std::move(il2));

  REQUIRE(il.size() == 8);

  b = il.begin();
  REQUIRE((*b).value == "d");
  b++;
  REQUIRE((*b).value == "4");
  b++;
  REQUIRE((*b).value == "3");
  b++;
  REQUIRE((*b).value == "2");
  b++;
  REQUIRE((*b).value == "1");
  b++;
  REQUIRE((*b).value == "c");
  b++;
  REQUIRE((*b).value == "b");
  b++;
  REQUIRE((*b).value == "a");
  b++;
  REQUIRE(b == il.end());
}

TEMPLATE_TEST_CASE("Validate intrusive_list erase", "[intrusive_list][erase]",

                   (ouly::intrusive_list<&object::hook, true, true>), (ouly::intrusive_list<&object::hook, false, true>)

)
{
  TestType il;
  using value_type = TestType::value_type;
  std::array arr   = {value_type("a"), value_type("b"), value_type("c"), value_type("d")};

  for (auto& v : arr)
    il.push_front(v);

  il.erase(arr[3]);
  REQUIRE(il.size() == 3);
  auto b = il.begin();
  REQUIRE((*b).value == "c");
  b++;
  REQUIRE((*b).value == "b");
  b++;
  REQUIRE((*b).value == "a");
  b++;
  REQUIRE(b == il.end());

  il.clear();

  for (auto& v : arr)
    il.push_front(v);

  il.erase(arr[0]);
  REQUIRE(il.size() == 3);
  b = il.begin();
  REQUIRE((*b).value == "d");
  b++;
  REQUIRE((*b).value == "c");
  b++;
  REQUIRE((*b).value == "b");
  b++;
  REQUIRE(b == il.end());

  il.clear();

  for (auto& v : arr)
    il.push_front(v);

  il.erase(arr[1]);
  REQUIRE(il.size() == 3);
  b = il.begin();
  REQUIRE((*b).value == "d");
  b++;
  REQUIRE((*b).value == "c");
  b++;
  REQUIRE((*b).value == "a");
  b++;
  REQUIRE(b == il.end());
}

// Comprehensive edge case testing
TEMPLATE_TEST_CASE(
 "Intrusive list - Empty list operations", "[intrusive_list][edge_cases][empty]",
 (ouly::intrusive_list<&sobject::hook, true, true>), (ouly::intrusive_list<&sobject::hook, true, false>),
 (ouly::intrusive_list<&sobject::hook, false, true>), (ouly::intrusive_list<&sobject::hook, false, false>),
 (ouly::intrusive_list<&object::hook, true, true>), (ouly::intrusive_list<&object::hook, true, false>),
 (ouly::intrusive_list<&object::hook, false, true>), (ouly::intrusive_list<&object::hook, false, false>))
{
  TestType il;

  // Test empty state
  REQUIRE(il.empty());
  REQUIRE(il.size() == 0);
  REQUIRE(il.begin() == il.end());

  // Test clearing already empty list
  il.clear();
  REQUIRE(il.empty());
  REQUIRE(il.size() == 0);
}

TEMPLATE_TEST_CASE(
 "Intrusive list - Single element operations", "[intrusive_list][edge_cases][single]",
 (ouly::intrusive_list<&sobject::hook, true, true>), (ouly::intrusive_list<&sobject::hook, true, false>),
 (ouly::intrusive_list<&sobject::hook, false, true>), (ouly::intrusive_list<&sobject::hook, false, false>),
 (ouly::intrusive_list<&object::hook, true, true>), (ouly::intrusive_list<&object::hook, true, false>),
 (ouly::intrusive_list<&object::hook, false, true>), (ouly::intrusive_list<&object::hook, false, false>))
{
  TestType il;
  using value_type = TestType::value_type;
  value_type node("single");

  // Test single element list
  il.push_front(node);
  REQUIRE(!il.empty());
  REQUIRE(il.size() == 1);
  REQUIRE(&il.front() == &node);

  if constexpr (TestType::has_tail)
  {
    REQUIRE(&il.back() == &node);
  }

  // Test iterator with single element
  auto it = il.begin();
  REQUIRE(&(*it) == &node);
  ++it;
  REQUIRE(it == il.end());

  // Test pop_front on single element
  il.pop_front();
  REQUIRE(il.empty());
  REQUIRE(il.size() == 0);
}

TEMPLATE_TEST_CASE(
 "Intrusive list - Move semantics", "[intrusive_list][edge_cases][move]",
 (ouly::intrusive_list<&sobject::hook, true, true>), (ouly::intrusive_list<&sobject::hook, true, false>),
 (ouly::intrusive_list<&sobject::hook, false, true>), (ouly::intrusive_list<&sobject::hook, false, false>),
 (ouly::intrusive_list<&object::hook, true, true>), (ouly::intrusive_list<&object::hook, true, false>),
 (ouly::intrusive_list<&object::hook, false, true>), (ouly::intrusive_list<&object::hook, false, false>))
{
  using value_type = TestType::value_type;
  std::array arr   = {value_type("a"), value_type("b"), value_type("c")};

  // Test move constructor
  TestType source_list;
  for (auto& v : arr)
  {
    source_list.push_front(v);
  }
  REQUIRE(source_list.size() == 3);

  TestType moved_list(std::move(source_list));
  REQUIRE(moved_list.size() == 3);
  REQUIRE(source_list.empty());
  REQUIRE(source_list.size() == 0);

  // Test move assignment
  TestType another_list;
  another_list = std::move(moved_list);
  REQUIRE(another_list.size() == 3);
  REQUIRE(moved_list.empty());

  // Test self-assignment
  TestType& ref = another_list;
  another_list  = std::move(ref);
  REQUIRE(another_list.size() == 3);
}

TEMPLATE_TEST_CASE(
 "Intrusive list - Pop operations edge cases", "[intrusive_list][edge_cases][pop]",
 (ouly::intrusive_list<&sobject::hook, true, true>), (ouly::intrusive_list<&sobject::hook, true, false>),
 (ouly::intrusive_list<&sobject::hook, false, true>), (ouly::intrusive_list<&sobject::hook, false, false>),
 (ouly::intrusive_list<&object::hook, true, true>), (ouly::intrusive_list<&object::hook, true, false>),
 (ouly::intrusive_list<&object::hook, false, true>), (ouly::intrusive_list<&object::hook, false, false>))
{
  TestType il;
  using value_type = TestType::value_type;
  std::array arr   = {value_type("a"), value_type("b")};

  // Test pop_front until empty
  for (auto& v : arr)
  {
    il.push_front(v);
  }

  while (!il.empty())
  {
    auto old_size = il.size();
    il.pop_front();
    REQUIRE(il.size() == old_size - 1);
  }
  REQUIRE(il.empty());
  REQUIRE(il.size() == 0);
}

TEMPLATE_TEST_CASE("Intrusive list - Pop back operations", "[intrusive_list][edge_cases][pop_back]",
                   (ouly::intrusive_list<&object::hook, true, true>),
                   (ouly::intrusive_list<&object::hook, false, true>))
{
  TestType il;
  using value_type = TestType::value_type;
  std::array arr   = {value_type("a"), value_type("b"), value_type("c")};

  for (auto& v : arr)
  {
    il.push_back(v);
  }

  // Test pop_back until empty
  REQUIRE(&il.back() == &arr[2]);
  il.pop_back();
  REQUIRE(il.size() == 2);
  REQUIRE(&il.back() == &arr[1]);

  il.pop_back();
  REQUIRE(il.size() == 1);
  REQUIRE(&il.back() == &arr[0]);
  REQUIRE(&il.front() == &arr[0]);

  il.pop_back();
  REQUIRE(il.empty());
  REQUIRE(il.size() == 0);
}

TEMPLATE_TEST_CASE("Intrusive list - Erase edge cases", "[intrusive_list][edge_cases][erase]",
                   (ouly::intrusive_list<&object::hook, true, true>),
                   (ouly::intrusive_list<&object::hook, true, false>),
                   (ouly::intrusive_list<&object::hook, false, true>),
                   (ouly::intrusive_list<&object::hook, false, false>))
{
  TestType il;
  using value_type = TestType::value_type;
  std::array arr   = {value_type("a"), value_type("b"), value_type("c"), value_type("d")};

  // Test erase all elements one by one
  for (auto& v : arr)
  {
    il.push_front(v);
  }

  // Erase from middle
  il.erase(arr[1]); // Remove "b"
  REQUIRE(il.size() == 3);
  auto it = il.begin();
  REQUIRE((*it).value == "d");
  ++it;
  REQUIRE((*it).value == "c");
  ++it;
  REQUIRE((*it).value == "a");

  // Erase head
  il.erase(arr[3]); // Remove "d" (head)
  REQUIRE(il.size() == 2);
  it = il.begin();
  REQUIRE((*it).value == "c");

  // Erase tail
  il.erase(arr[0]); // Remove "a" (tail)
  REQUIRE(il.size() == 1);
  it = il.begin();
  REQUIRE((*it).value == "c");

  // Erase last element
  il.erase(arr[2]);
  REQUIRE(il.empty());
  REQUIRE(il.size() == 0);
}

TEMPLATE_TEST_CASE(
 "Intrusive list - Erase after edge cases", "[intrusive_list][edge_cases][erase_after]",
 (ouly::intrusive_list<&sobject::hook, true, true>), (ouly::intrusive_list<&sobject::hook, true, false>),
 (ouly::intrusive_list<&sobject::hook, false, true>), (ouly::intrusive_list<&sobject::hook, false, false>),
 (ouly::intrusive_list<&object::hook, true, true>), (ouly::intrusive_list<&object::hook, true, false>),
 (ouly::intrusive_list<&object::hook, false, true>), (ouly::intrusive_list<&object::hook, false, false>))
{
  TestType il;
  using value_type = TestType::value_type;
  std::array arr   = {value_type("a"), value_type("b"), value_type("c")};

  // Test erase_after when target is the last element (no-op)
  for (auto& v : arr)
  {
    il.push_front(v);
  }

  auto initial_size = il.size();
  il.erase_after(arr[0]);             // Try to erase after last element
  REQUIRE(il.size() == initial_size); // Size unchanged

  // Test erase_after removing the last element
  il.erase_after(arr[1]); // Should remove arr[0]
  REQUIRE(il.size() == initial_size - 1);

  // Verify the tail is updated correctly when removing last element
  if constexpr (TestType::has_tail)
  {
    REQUIRE(&il.back() == &arr[1]);
  }
}

TEMPLATE_TEST_CASE(
 "Intrusive list - Insert operations comprehensive", "[intrusive_list][edge_cases][insert]",
 (ouly::intrusive_list<&sobject::hook, true, true>), (ouly::intrusive_list<&sobject::hook, true, false>),
 (ouly::intrusive_list<&sobject::hook, false, true>), (ouly::intrusive_list<&sobject::hook, false, false>),
 (ouly::intrusive_list<&object::hook, true, true>), (ouly::intrusive_list<&object::hook, true, false>),
 (ouly::intrusive_list<&object::hook, false, true>), (ouly::intrusive_list<&object::hook, false, false>))
{
  TestType il;
  using value_type = TestType::value_type;
  std::array arr   = {value_type("a"), value_type("b"), value_type("c")};
  value_type new_node("x");

  // Test insert_after on empty list (push_front equivalent)
  il.push_front(arr[0]);
  il.insert_after(arr[0], new_node);
  REQUIRE(il.size() == 2);

  if constexpr (TestType::has_tail)
  {
    REQUIRE(&il.back() == &new_node);
  }

  // Test insert_after at end updates tail correctly
  value_type end_node("end");
  il.insert_after(new_node, end_node);
  REQUIRE(il.size() == 3);

  if constexpr (TestType::has_tail)
  {
    REQUIRE(&il.back() == &end_node);
  }
}

TEMPLATE_TEST_CASE("Intrusive list - Insert before operations", "[intrusive_list][edge_cases][insert_before]",
                   (ouly::intrusive_list<&object::hook, true, true>),
                   (ouly::intrusive_list<&object::hook, true, false>),
                   (ouly::intrusive_list<&object::hook, false, true>),
                   (ouly::intrusive_list<&object::hook, false, false>))
{
  TestType il;
  using value_type = TestType::value_type;
  std::array arr   = {value_type("a"), value_type("b"), value_type("c")};
  value_type new_node("x");

  // Build list: c -> b -> a
  for (auto& v : arr)
  {
    il.push_front(v);
  }

  // Test insert before head (should become new head)
  il.insert(arr[2], new_node); // Insert before "c"
  REQUIRE(il.size() == 4);
  REQUIRE(&il.front() == &new_node);

  // Verify order: x -> c -> b -> a
  auto it = il.begin();
  REQUIRE((*it).value == "x");
  ++it;
  REQUIRE((*it).value == "c");
  ++it;
  REQUIRE((*it).value == "b");
  ++it;
  REQUIRE((*it).value == "a");
}

TEMPLATE_TEST_CASE("Intrusive list - Append operations comprehensive", "[intrusive_list][edge_cases][append]",
                   (ouly::intrusive_list<&object::hook, true, true>),
                   (ouly::intrusive_list<&object::hook, false, true>))
{
  TestType il1, il2, il3;
  using value_type = TestType::value_type;
  std::array arr1  = {value_type("a"), value_type("b")};
  std::array arr2  = {value_type("1"), value_type("2")};
  std::array arr3  = {value_type("x"), value_type("y")};

  // Test append_front with empty source
  for (auto& v : arr1)
  {
    il1.push_front(v);
  }

  il1.append_front(std::move(il3)); // il3 is empty
  REQUIRE(il1.size() == 2);
  REQUIRE(il3.empty());

  // Test append_back with empty source
  il1.append_back(std::move(il3)); // il3 is still empty
  REQUIRE(il1.size() == 2);
  REQUIRE(il3.empty());

  // Test append_front with empty destination
  for (auto& v : arr2)
  {
    il2.push_front(v);
  }

  il3.append_front(std::move(il2));
  REQUIRE(il3.size() == 2);
  REQUIRE(il2.empty());

  // Test append_back with empty destination
  for (auto& v : arr2)
  {
    il2.push_front(v);
  }

  TestType il4;
  il4.append_back(std::move(il2));
  REQUIRE(il4.size() == 2);
  REQUIRE(il2.empty());
}

TEMPLATE_TEST_CASE("Intrusive list - Large scale operations", "[intrusive_list][edge_cases][large_scale]",
                   (ouly::intrusive_list<&sobject::hook, true, true>),
                   (ouly::intrusive_list<&object::hook, true, true>))
{
  TestType il;
  using value_type = TestType::value_type;

  // Test with larger number of elements
  const size_t            N = 1000;
  std::vector<value_type> nodes;
  nodes.reserve(N);

  for (size_t i = 0; i < N; ++i)
  {
    nodes.emplace_back(std::to_string(i));
    il.push_front(nodes.back());
  }

  REQUIRE(il.size() == N);

  // Test iterator traversal
  size_t count = 0;
  for (auto& node : il)
  {
    (void)node; // Suppress unused variable warning
    ++count;
  }
  REQUIRE(count == N);

  // Test clearing large list
  il.clear();
  REQUIRE(il.empty());
  REQUIRE(il.size() == 0);
}

TEMPLATE_TEST_CASE("Intrusive list - Hook state verification", "[intrusive_list][edge_cases][hook_state]",
                   (ouly::intrusive_list<&object::hook, true, true>)) // Only test doubly-linked lists for erase
{
  TestType il;
  using value_type = typename TestType::value_type;
  std::array arr   = {value_type("a"), value_type("b"), value_type("c")};

  // Verify hooks are properly cleared after removal
  for (auto& v : arr)
  {
    il.push_front(v);
  }

  // Remove middle element and verify its hooks are cleared
  il.erase(arr[1]);

  // The hook should be cleaned up
  using traits = ouly::detail::intrusive_list_type_traits<&value_type::hook>;
  REQUIRE(traits::next(arr[1]) == nullptr);

  if constexpr (std::is_same_v<typename traits::hook_type, ouly::list_hook>)
  {
    REQUIRE(traits::prev(arr[1]) == nullptr);
  }
}

TEMPLATE_TEST_CASE(
 "Intrusive list - Iterator stability", "[intrusive_list][edge_cases][iterator_stability]",
 (ouly::intrusive_list<&sobject::hook, true, true>), (ouly::intrusive_list<&sobject::hook, true, false>),
 (ouly::intrusive_list<&sobject::hook, false, true>), (ouly::intrusive_list<&sobject::hook, false, false>),
 (ouly::intrusive_list<&object::hook, true, true>), (ouly::intrusive_list<&object::hook, true, false>),
 (ouly::intrusive_list<&object::hook, false, true>), (ouly::intrusive_list<&object::hook, false, false>))
{
  TestType il;
  using value_type = TestType::value_type;
  std::array arr   = {value_type("a"), value_type("b"), value_type("c"), value_type("d")};

  for (auto& v : arr)
  {
    il.push_front(v);
  }

  // Get iterator to middle element
  auto it = il.begin();
  ++it; // Point to second element
  value_type* target = &(*it);

  // Remove a different element
  il.erase_after(arr[3]); // Remove first element after head

  // Iterator should still be valid and point to same element
  REQUIRE(&(*it) == target);
}

TEMPLATE_TEST_CASE("Intrusive list - Const correctness", "[intrusive_list][edge_cases][const]",
                   (ouly::intrusive_list<&sobject::hook, true, true>),
                   (ouly::intrusive_list<&object::hook, true, true>))
{
  TestType il;
  using value_type = TestType::value_type;
  std::array arr   = {value_type("a"), value_type("b"), value_type("c")};

  for (auto& v : arr)
  {
    il.push_front(v);
  }

  const TestType& const_il = il;

  // Test const operations
  REQUIRE(!const_il.empty());
  REQUIRE(const_il.size() == 3);
  REQUIRE(&const_il.front() == &arr[2]);

  if constexpr (TestType::has_tail)
  {
    REQUIRE(&const_il.back() == &arr[0]);
  }

  // Test const iterators
  auto const_it = const_il.begin();
  REQUIRE(&(*const_it) == &arr[2]);

  auto const_cit = const_il.cbegin();
  REQUIRE(&(*const_cit) == &arr[2]);

  // Test const iterator traversal
  size_t count = 0;
  for (auto cit = const_il.cbegin(); cit != const_il.cend(); ++cit)
  {
    ++count;
  }
  REQUIRE(count == 3);
}

TEMPLATE_TEST_CASE("Intrusive list - Reverse iterator comprehensive", "[intrusive_list][edge_cases][reverse_iterator]",
                   (ouly::intrusive_list<&object::hook, true, true>),
                   (ouly::intrusive_list<&object::hook, false, true>))
{
  TestType il;
  using value_type = TestType::value_type;
  std::array arr   = {value_type("a"), value_type("b"), value_type("c"), value_type("d")};

  for (auto& v : arr)
  {
    il.push_back(v); // Use push_back to maintain order
  }

  // Test reverse iteration
  auto rit = il.rbegin();
  for (auto it = arr.rbegin(); it != arr.rend(); ++it, ++rit)
  {
    REQUIRE(&(*rit) == &(*it));
  }
  REQUIRE(rit == il.rend());

  // Test const reverse iteration
  const TestType& const_il = il;
  auto            crit     = const_il.rbegin();
  for (auto it = arr.rbegin(); it != arr.rend(); ++it, ++crit)
  {
    REQUIRE(&(*crit) == &(*it));
  }
  REQUIRE(crit == const_il.rend());
}

TEMPLATE_TEST_CASE("Intrusive list - Size tracking verification", "[intrusive_list][edge_cases][size_tracking]",
                   (ouly::intrusive_list<&sobject::hook, true, true>),  // Size cached
                   (ouly::intrusive_list<&sobject::hook, false, true>), // Size not cached
                   (ouly::intrusive_list<&object::hook, true, true>),   // Size cached
                   (ouly::intrusive_list<&object::hook, false, true>))  // Size not cached
{
  TestType il;
  using value_type = TestType::value_type;
  std::array arr   = {value_type("a"), value_type("b"), value_type("c"), value_type("d"), value_type("e")};

  // Test size tracking through various operations
  REQUIRE(il.size() == 0);

  il.push_front(arr[0]);
  REQUIRE(il.size() == 1);

  il.push_back(arr[1]);
  REQUIRE(il.size() == 2);

  il.insert_after(arr[0], arr[2]);
  REQUIRE(il.size() == 3);

  if constexpr (std::is_same_v<typename TestType::value_type, object>)
  {
    il.insert(arr[1], arr[3]);
    REQUIRE(il.size() == 4);

    il.erase(arr[3]);
    REQUIRE(il.size() == 3);
  }

  il.erase_after(arr[0]);
  REQUIRE(il.size() == 2);

  il.pop_front();
  REQUIRE(il.size() == 1);

  if constexpr (TestType::has_tail && std::is_same_v<typename TestType::value_type, object>)
  {
    il.pop_back();
    REQUIRE(il.size() == 0);
  }
  else
  {
    il.pop_front();
    REQUIRE(il.size() == 0);
  }

  REQUIRE(il.empty());
}

TEMPLATE_TEST_CASE("Intrusive list - Constructor edge cases", "[intrusive_list][edge_cases][constructor]",
                   (ouly::intrusive_list<&sobject::hook, true, false>),  // Size cached, no tail
                   (ouly::intrusive_list<&sobject::hook, false, false>), // No size cache, no tail
                   (ouly::intrusive_list<&object::hook, true, true>),    // Size cached, tail cached
                   (ouly::intrusive_list<&object::hook, true, false>))   // Size cached, no tail
{
  using value_type = TestType::value_type;
  std::array arr   = {value_type("a"), value_type("b"), value_type("c"), value_type("d")};

  // Test constructor variants
  if constexpr (!TestType::has_tail)
  {
    // Test range constructor without tail caching
    // Connect the nodes manually for testing
    ouly::detail::intrusive_list_type_traits<&value_type::hook>::next(arr[0], &arr[1]);
    ouly::detail::intrusive_list_type_traits<&value_type::hook>::next(arr[1], &arr[2]);
    ouly::detail::intrusive_list_type_traits<&value_type::hook>::next(arr[2], &arr[3]);
    ouly::detail::intrusive_list_type_traits<&value_type::hook>::next(arr[3], nullptr);

    TestType il(arr[0], 4);
    REQUIRE(il.size() == 4);
    REQUIRE(&il.front() == &arr[0]);

    // Clean up for next test
    for (auto& v : arr)
    {
      ouly::detail::intrusive_list_type_traits<&value_type::hook>::next(v, nullptr);
      if constexpr (std::is_same_v<typename TestType::value_type, object>)
      {
        ouly::detail::intrusive_list_type_traits<&value_type::hook>::prev(v, nullptr);
      }
    }
  }
  else
  {
    // Test range constructor with tail caching
    using traits = ouly::detail::intrusive_list_type_traits<&value_type::hook>;
    traits::next(arr[0], &arr[1]);
    traits::next(arr[1], &arr[2]);
    traits::next(arr[2], &arr[3]);
    traits::next(arr[3], nullptr);

    if constexpr (std::is_same_v<typename TestType::value_type, object>)
    {
      traits::prev(arr[0], nullptr);
      traits::prev(arr[1], &arr[0]);
      traits::prev(arr[2], &arr[1]);
      traits::prev(arr[3], &arr[2]);
    }

    TestType il(arr[0], arr[3], 4);
    REQUIRE(il.size() == 4);
    REQUIRE(&il.front() == &arr[0]);
    REQUIRE(&il.back() == &arr[3]);

    // Clean up
    for (auto& v : arr)
    {
      traits::next(v, nullptr);
      if constexpr (std::is_same_v<typename TestType::value_type, object>)
      {
        traits::prev(v, nullptr);
      }
    }
  }
}

TEMPLATE_TEST_CASE("Intrusive list - Advanced iterator operations", "[intrusive_list][edge_cases][advanced_iterators]",
                   (ouly::intrusive_list<&sobject::hook, true, true>),
                   (ouly::intrusive_list<&object::hook, true, true>))
{
  TestType il;
  using value_type = TestType::value_type;
  std::array arr   = {value_type("a"), value_type("b"), value_type("c"), value_type("d"), value_type("e")};

  for (auto& v : arr)
  {
    il.push_back(v);
  }

  // Test iterator equality and comparison
  auto it1 = il.begin();
  auto it2 = il.begin();
  auto it3 = il.end();

  REQUIRE(it1 == it2);
  REQUIRE(it1 != it3);
  REQUIRE(it2 != it3);

  // Test post-increment vs pre-increment
  auto it_pre  = il.begin();
  auto it_post = il.begin();

  auto result_pre  = ++it_pre;
  auto result_post = it_post++;

  REQUIRE(&(*result_pre) == &arr[1]);
  REQUIRE(&(*result_post) == &arr[0]);
  REQUIRE(&(*it_pre) == &arr[1]);
  REQUIRE(&(*it_post) == &arr[1]);

  // Test iterator arithmetic consistency
  auto it = il.begin();
  for (size_t i = 0; i < arr.size(); ++i)
  {
    REQUIRE(&(*it) == &arr[i]);
    if (i < arr.size() - 1)
    {
      ++it;
    }
  }
  ++it;
  REQUIRE(it == il.end());
}

TEMPLATE_TEST_CASE("Intrusive list - Memory layout and alignment", "[intrusive_list][edge_cases][memory_layout]",
                   (ouly::intrusive_list<&sobject::hook, true, true>),
                   (ouly::intrusive_list<&object::hook, true, true>))
{
  TestType il;

  // Test that the intrusive_list itself has reasonable size
  // (should be small since it only stores pointers and counters)
  REQUIRE(sizeof(TestType) <= 64); // Reasonable upper bound

  // Test that we can create many lists without excessive memory usage
  std::vector<TestType> many_lists(100);
  REQUIRE(many_lists.size() == 100);

  // All lists should be empty initially
  for (auto& list : many_lists)
  {
    REQUIRE(list.empty());
    REQUIRE(list.size() == 0);
  }
}

TEMPLATE_TEST_CASE("Intrusive list - Exception safety simulation", "[intrusive_list][edge_cases][exception_safety]",
                   (ouly::intrusive_list<&sobject::hook, true, true>),
                   (ouly::intrusive_list<&object::hook, true, true>))
{
  TestType il;
  using value_type = TestType::value_type;
  std::array arr   = {value_type("a"), value_type("b"), value_type("c")};

  // Test that operations maintain list integrity even with "exceptions"
  // (simulated by early returns or conditional operations)

  for (auto& v : arr)
  {
    il.push_front(v);
  }

  // Simulate partial operation that might be interrupted
  auto initial_size = il.size();
  auto it           = il.begin();
  ++it; // Point to middle element

  // "Exception" occurs here - verify list is still valid
  REQUIRE(il.size() == initial_size);
  REQUIRE(!il.empty());

  // List should still be traversable
  size_t count = 0;
  for (auto& node : il)
  {
    (void)node;
    ++count;
  }
  REQUIRE(count == initial_size);
}

TEMPLATE_TEST_CASE("Intrusive list - Stress test operations", "[intrusive_list][edge_cases][stress_test]",
                   (ouly::intrusive_list<&sobject::hook, true, true>),
                   (ouly::intrusive_list<&object::hook, true, true>))
{
  TestType il;
  using value_type = TestType::value_type;

  // Test repeated insertions and deletions
  const size_t            iterations = 100;
  std::vector<value_type> nodes;
  nodes.reserve(iterations);

  // Add many nodes
  for (size_t i = 0; i < iterations; ++i)
  {
    nodes.emplace_back(std::to_string(i));
    il.push_front(nodes.back());
  }
  REQUIRE(il.size() == iterations);

  // Remove every other node
  for (size_t i = 0; i < iterations; i += 2)
  {
    if constexpr (std::is_same_v<typename TestType::value_type, object>)
    {
      il.erase(nodes[i]);
    }
    else
    {
      // For singly-linked lists, we need to use erase_after
      if (i > 0)
      {
        il.erase_after(nodes[i - 1]);
      }
      else
      {
        il.pop_front();
      }
    }
  }

  // Verify list integrity
  size_t count = 0;
  for (auto& node : il)
  {
    (void)node;
    ++count;
  }
  REQUIRE(count == il.size());

  // Clear and verify
  il.clear();
  REQUIRE(il.empty());
  REQUIRE(il.size() == 0);
}

TEMPLATE_TEST_CASE("Intrusive list - Multiple lists with same nodes", "[intrusive_list][edge_cases][multi_list]",
                   (ouly::intrusive_list<&sobject::hook, true, true>),
                   (ouly::intrusive_list<&object::hook, true, true>))
{
  // Test that the same node type can be used in different lists
  // (though not simultaneously in the same hook)
  TestType il1, il2;
  using value_type = TestType::value_type;
  std::array arr1  = {value_type("1a"), value_type("1b"), value_type("1c")};
  std::array arr2  = {value_type("2a"), value_type("2b"), value_type("2c")};

  // Add to first list
  for (auto& v : arr1)
  {
    il1.push_front(v);
  }

  // Add to second list
  for (auto& v : arr2)
  {
    il2.push_front(v);
  }

  REQUIRE(il1.size() == 3);
  REQUIRE(il2.size() == 3);

  // Verify lists are independent
  il1.clear();
  REQUIRE(il1.empty());
  REQUIRE(il2.size() == 3); // il2 should be unaffected

  // Move nodes from il2 to il1
  while (!il2.empty())
  {
    auto& front = il2.front();
    il2.pop_front();
    il1.push_front(front);
  }

  REQUIRE(il1.size() == 3);
  REQUIRE(il2.empty());
}

TEMPLATE_TEST_CASE("Intrusive list - Boundary value testing", "[intrusive_list][edge_cases][boundary]",
                   (ouly::intrusive_list<&sobject::hook, true, true>),
                   (ouly::intrusive_list<&object::hook, true, true>))
{
  TestType il;
  using value_type = TestType::value_type;
  using size_type  = typename TestType::size_type;

  // Test with maximum reasonable size (limited by available memory)
  const size_t            max_test_size = 10000;
  std::vector<value_type> nodes;
  nodes.reserve(max_test_size);

  // Add nodes up to test limit
  for (size_t i = 0; i < max_test_size; ++i)
  {
    nodes.emplace_back(std::to_string(i));
    il.push_front(nodes.back());

    // Verify size tracking at key boundaries
    if (i == 0 || i == 1 || i == 10 || i == 100 || i == 1000 || i == max_test_size - 1)
    {
      REQUIRE(il.size() == static_cast<size_type>(i + 1));
    }
  }

  REQUIRE(il.size() == max_test_size);

  // Test rapid clear
  il.clear();
  REQUIRE(il.empty());
  REQUIRE(il.size() == 0);
}

TEMPLATE_TEST_CASE("Intrusive list - Hook cleanup verification", "[intrusive_list][edge_cases][hook_cleanup]",
                   (ouly::intrusive_list<&sobject::hook, true, true>),
                   (ouly::intrusive_list<&object::hook, true, true>))
{
  TestType il;
  using value_type = TestType::value_type;
  using traits     = ouly::detail::intrusive_list_type_traits<&value_type::hook>;
  std::array arr   = {value_type("a"), value_type("b"), value_type("c"), value_type("d")};

  // Add all nodes
  for (auto& v : arr)
  {
    il.push_front(v);
  }

  // Remove nodes one by one and verify hook cleanup
  while (!il.empty())
  {
    auto& front = il.front();
    il.pop_front();

    // Verify hook is cleaned up
    REQUIRE(traits::next(front) == nullptr);
    if constexpr (std::is_same_v<typename TestType::value_type, object>)
    {
      REQUIRE(traits::prev(front) == nullptr);
    }
  }

  // All nodes should have clean hooks
  for (auto& v : arr)
  {
    REQUIRE(traits::next(v) == nullptr);
    if constexpr (std::is_same_v<typename TestType::value_type, object>)
    {
      REQUIRE(traits::prev(v) == nullptr);
    }
  }
}

TEMPLATE_TEST_CASE("Intrusive list - Complex interleaved operations", "[intrusive_list][edge_cases][complex_ops]",
                   (ouly::intrusive_list<&object::hook, true, true>))
{
  TestType il;
  using value_type = TestType::value_type;
  std::array arr   = {value_type("a"), value_type("b"), value_type("c"),
                      value_type("d"), value_type("e"), value_type("f")};

  // Complex sequence of operations
  il.push_front(arr[0]);           // [a]
  il.push_back(arr[1]);            // [a, b]
  il.insert_after(arr[0], arr[2]); // [a, c, b]
  il.insert(arr[1], arr[3]);       // [a, c, d, b]
  il.push_front(arr[4]);           // [e, a, c, d, b]
  il.insert_after(arr[2], arr[5]); // [e, a, c, f, d, b]

  REQUIRE(il.size() == 6);

  // Verify the complex order
  auto it = il.begin();
  REQUIRE((*it).value == "e");
  ++it;
  REQUIRE((*it).value == "a");
  ++it;
  REQUIRE((*it).value == "c");
  ++it;
  REQUIRE((*it).value == "f");
  ++it;
  REQUIRE((*it).value == "d");
  ++it;
  REQUIRE((*it).value == "b");
  ++it;
  REQUIRE(it == il.end());

  // Complex removal sequence
  il.erase(arr[4]);       // Remove 'e': [a, c, f, d, b]
  il.erase_after(arr[2]); // Remove 'f': [a, c, d, b]
  il.pop_back();          // Remove 'b': [a, c, d]
  il.pop_front();         // Remove 'a': [c, d]

  REQUIRE(il.size() == 2);
  it = il.begin();
  REQUIRE((*it).value == "c");
  ++it;
  REQUIRE((*it).value == "d");
  ++it;
  REQUIRE(it == il.end());
}

// Test that intrusive lists work with simple operations
TEST_CASE("Simple intrusive list test", "[intrusive_list][simple]")
{
  ouly::intrusive_list<&sobject::hook, true, true> il;
  sobject                                          obj("test");

  il.push_front(obj);
  REQUIRE(!il.empty());
  REQUIRE(il.size() == 1);
  REQUIRE(&il.front() == &obj);

  il.clear();
  REQUIRE(il.empty());
}

// NOLINTEND
