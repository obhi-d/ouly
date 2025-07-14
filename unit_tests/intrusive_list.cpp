
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
// NOLINTEND
