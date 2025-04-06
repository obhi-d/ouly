#include "ouly/containers/soavector.hpp"
#include "catch2/catch_all.hpp"
#include "test_common.hpp"
#include <string>

// NOLINTBEGIN
TEST_CASE("soavector: Validate soavector emplace", "[soavector][emplace]")
{
  struct pack
  {
    int         i;
    bool        b;
    std::string s;
  };

  ouly::soavector<pack> v1;
  v1.emplace_back(100, true, "first");
  v1.emplace_back(200, false, "second");
  v1.emplace_back(300, false, "third");
  ouly::soavector<pack> v2 = {
   {100,  true,  "first"},
   {200, false, "second"},
   {300, false,  "third"}
  };
  REQUIRE(v1 == v2);
}

TEST_CASE("soavector: Validate soavector assign", "[soavector][assign]")
{

  struct pack
  {
    int         i;
    bool        b;
    std::string s;
  };

  ouly::soavector<pack> v1, v2;

  for (std::uint32_t i = 0; i < 1000; ++i)
    v1.emplace_back(std::rand(), std::rand() > 10000, std::to_string(std::rand()));

  v2.assign(v1.begin(), v1.end());
  REQUIRE(v1 == v2);
  auto saved = pack{std::rand(), std::rand() > 10000, std::to_string(std::rand())};
  v1.assign(10, saved);
  v2.assign(10, saved);
  REQUIRE(v1.size() == 10);
  REQUIRE(v1 == v2);
  REQUIRE(v1.back<0>() == saved.i);
  REQUIRE(v1.back<1>() == saved.b);
  REQUIRE(v2.back<2>() == saved.s);
  v2.clear();
  REQUIRE(v2.size() == 0);
  REQUIRE(v2.capacity() != 0);
  v2.shrink_to_fit();
  REQUIRE(v2.capacity() == 0);
}

TEST_CASE("soavector: Validate soavector insert", "[soavector][insert]")
{

  struct pack
  {
    int         i;
    bool        b;
    std::string s;
  };

  ouly::soavector<pack> v1;

  auto saved = pack{std::rand(), std::rand() > 10000, std::to_string(std::rand())};
  v1.insert(v1.size(), saved);
  v1.insert(v1.size(), pack{std::rand(), std::rand() > 10000, std::to_string(std::rand())});
  v1.insert(v1.size(), pack{std::rand(), std::rand() > 10000, std::to_string(std::rand())});
  v1.insert(2, saved);

  REQUIRE(v1.at<0>(2) == saved.i);
  REQUIRE(v1.at<1>(2) == saved.b);
  REQUIRE(v1.at<2>(2) == saved.s);
  REQUIRE(v1.size() == 4);

  auto saved2 = pack{std::rand(), std::rand() > 10000, std::to_string(std::rand())};
  v1.insert(1, 10, saved);
  for (std::uint32_t i = 0; i < 10; ++i)
  {
    REQUIRE(v1.at<0>(i + 1) == saved.i);
    REQUIRE(v1.at<1>(i + 1) == saved.b);
    REQUIRE(v1.at<2>(i + 1) == saved.s);
  }
}

TEST_CASE("soavector: Validate soavector erase", "[soavector][erase]")
{

  struct pack
  {
    int         i;
    bool        b;
    std::string s;
  };

  ouly::soavector<pack> v1;

  std::array<pack, 10> saved;
  for (std::uint32_t i = 0; i < 10; ++i)
    saved[i] = pack{std::rand(), std::rand() > 10000, std::to_string(std::rand())};

  v1.insert(0, saved.begin(), saved.end());
  REQUIRE(v1.size() == 10);
  v1.erase(2);
  REQUIRE(v1.size() == 9);
  REQUIRE(v1.at<0>(2) == saved[3].i);
  REQUIRE(v1.at<1>(2) == saved[3].b);
  REQUIRE(v1.at<2>(2) == saved[3].s);
  auto                 v2 = v1;
  std::array<pack, 10> saved2;
  for (std::uint32_t i = 0; i < 10; ++i)
    saved2[i] = pack{std::rand(), std::rand() > 10000, std::to_string(std::rand())};

  v1.insert(1, saved2.begin(), saved2.end());
  v1.erase(1, 11);
  REQUIRE(v1.size() == 9);
  REQUIRE(v2 == v1);
}

// Basic struct for testing
struct TestStruct
{
  int         x;
  float       y;
  std::string s;
  auto        operator<=>(const TestStruct&) const = default;
};

TEST_CASE("soavector: Basic operations", "[soavector]")
{
  ouly::soavector<TestStruct> vec;

  SECTION("Constructor and size")
  {
    REQUIRE(vec.empty());
    REQUIRE(vec.size() == 0);

    ouly::soavector<TestStruct> vec2(5);
    REQUIRE(vec2.size() == 5);

    TestStruct                  val{1, 2.0f, "test"};
    ouly::soavector<TestStruct> vec3(3, val);
    REQUIRE(vec3.size() == 3);
    REQUIRE(vec3[0].get().x == 1);
  }

  SECTION("Push back and access")
  {
    vec.push_back({1, 1.0f, "one"});
    vec.push_back({2, 2.0f, "two"});

    REQUIRE(vec.size() == 2);
    auto value_0 = (TestStruct)vec[0];
    auto value_1 = (TestStruct)vec[1];
    REQUIRE(value_0.x == 1);
    REQUIRE(value_1.x == 2);
    REQUIRE(vec.front().get().x == 1);
    REQUIRE(vec.back().get().x == 2);
  }

  SECTION("Element access")
  {
    vec.push_back({1, 1.0f, "one"});
    vec.push_back({2, 2.0f, "two"});

    REQUIRE(vec.at<0>(0) == 1);
    REQUIRE(vec.at<1>(0) == 1.0f);
    REQUIRE(vec.at<2>(0) == "one");
  }

  SECTION("Iterator operations")
  {
    vec.push_back({1, 1.0f, "one"});
    vec.push_back({2, 2.0f, "two"});

    auto it = vec.begin();
    REQUIRE(std::get<0>((*it)) == 1);
    ++it;
    REQUIRE(std::get<0>((*it)) == 2);

    int sum = 0;
    for (const auto& v : vec)
    {
      sum += std::get<0>(v);
    }
    REQUIRE(sum == 3);
  }
}

TEST_CASE("soavector: Memory operations", "[soavector]")
{
  ouly::soavector<TestStruct> vec;

  SECTION("Reserve and capacity")
  {
    vec.reserve(10);
    REQUIRE(vec.capacity() >= 10);
    REQUIRE(vec.size() == 0);

    vec.push_back({1, 1.0f, "test"});
    REQUIRE(vec.capacity() >= 10);
  }

  SECTION("Resize")
  {
    vec.resize(5);
    REQUIRE(vec.size() == 5);

    vec.resize(3);
    REQUIRE(vec.size() == 3);

    vec.resize(6, {1, 1.0f, "test"});
    REQUIRE(vec.size() == 6);
    REQUIRE(vec[5].get().x == 1);
  }

  SECTION("Shrink to fit")
  {
    vec.reserve(100);
    vec.push_back({1, 1.0f, "test"});
    auto oldCapacity = vec.capacity();
    vec.shrink_to_fit();
    REQUIRE(vec.capacity() <= oldCapacity);
  }
}

TEST_CASE("soavector: Copy and move operations", "[soavector]")
{
  ouly::soavector<TestStruct> vec;
  vec.push_back({1, 1.0f, "one"});
  vec.push_back({2, 2.0f, "two"});

  SECTION("Copy constructor")
  {
    auto vec2(vec);
    REQUIRE(vec2 == vec);
  }

  SECTION("Move constructor")
  {
    auto vec2(std::move(vec));
    REQUIRE(vec2.size() == 2);
    REQUIRE(vec.empty());
  }

  SECTION("Copy assignment")
  {
    ouly::soavector<TestStruct> vec2;
    vec2 = vec;
    REQUIRE(vec2 == vec);
  }

  SECTION("Move assignment")
  {
    ouly::soavector<TestStruct> vec2;
    vec2 = std::move(vec);
    REQUIRE(vec2.size() == 2);
    REQUIRE(vec.empty());
  }
}

TEST_CASE("soavector: Modifiers", "[soavector]")
{
  ouly::soavector<TestStruct> vec;

  SECTION("Emplace operations")
  {
    vec.emplace_back(1, 1.0f, "one");
    REQUIRE(vec.size() == 1);
    REQUIRE(vec.back().get().x == 1);

    vec.emplace(0, 0, 0.0f, "zero");
    REQUIRE(vec.size() == 2);
    REQUIRE(vec.front().get().x == 0);
  }

  SECTION("Erase operations")
  {
    vec.push_back({1, 1.0f, "one"});
    vec.push_back({2, 2.0f, "two"});
    vec.push_back({3, 3.0f, "three"});

    vec.erase(1);
    REQUIRE(vec.size() == 2);
    REQUIRE(vec[1].get().x == 3);

    vec.clear();
    REQUIRE(vec.empty());
  }

  SECTION("Insert operations")
  {
    TestStruct val{1, 1.0f, "test"};
    vec.insert(0, val);
    REQUIRE(vec.size() == 1);

    std::vector<TestStruct> values{
     {2, 2.0f,   "two"},
     {3, 3.0f, "three"}
    };
    vec.insert(vec.size(), values.begin(), values.end());
    REQUIRE(vec.size() == 3);
  }
}

TEST_CASE("soavector: Comparison operators", "[soavector]")
{
  ouly::soavector<TestStruct> vec1;
  ouly::soavector<TestStruct> vec2;

  vec1.push_back({1, 1.0f, "one"});
  vec2.push_back({1, 1.0f, "one"});

  REQUIRE(vec1 == vec2);

  vec2.push_back({2, 2.0f, "two"});
  REQUIRE(vec1 != vec2);
  REQUIRE(vec1 < vec2);
  REQUIRE(vec2 > vec1);
  REQUIRE(vec1 <= vec2);
  REQUIRE(vec2 >= vec1);
}

// NOLINTEND