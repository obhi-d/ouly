// SPDX-License-Identifier: MIT

#include "ouly/allocators/object_pool.hpp"
#include "test_common.hpp"
#include <catch2/catch_all.hpp>
// NOLINTBEGIN
using namespace Catch::literals;

namespace
{

struct TestObject
{
  int    value = 42;
  double data  = 3.14;

  TestObject() = default;
  TestObject(int v, double d) : value(v), data(d) {}
};

struct AlignedObject
{
  alignas(16) int value = 99;
  char padding[12]      = {};
};

struct HighlyAlignedObject
{
  alignas(64) double value = 1.23;
  char padding[56]         = {};
};

} // anonymous namespace

TEST_CASE("object_pool basic functionality", "[object_pool]")
{
  ouly::object_pool<TestObject> pool;

  SECTION("can allocate and deallocate single object")
  {
    auto* obj = pool.allocate();
    REQUIRE(obj != nullptr);

    // Construct object in place
    new (obj) TestObject(100, 2.71);
    REQUIRE(obj->value == 100);
    REQUIRE(obj->data == 2.71_a);

    // Clean up
    obj->~TestObject();
    pool.deallocate(obj);
  }
}

TEST_CASE("object_pool with aligned objects", "[object_pool]")
{
  ouly::object_pool<AlignedObject> pool;

  auto* obj = pool.allocate();
  REQUIRE(obj != nullptr);

  // Check alignment
  REQUIRE((reinterpret_cast<uintptr_t>(obj) & 15) == 0);

  new (obj) AlignedObject();
  REQUIRE(obj->value == 99);

  obj->~AlignedObject();
  pool.deallocate(obj);
}

TEST_CASE("object_pool with custom configuration", "[object_pool]")
{
  using CustomConfig = ouly::config<ouly::cfg::pool_size<128>>;
  ouly::object_pool<TestObject, CustomConfig> pool;

  SECTION("can allocate many objects with custom pool size")
  {
    std::vector<TestObject*> objects;
    for (int i = 0; i < 200; ++i)
    { // More than default pool size
      auto* obj = pool.allocate();
      REQUIRE(obj != nullptr);
      new (obj) TestObject(i, i * 0.5);
      objects.push_back(obj);
    }

    // Verify objects
    for (size_t i = 0; i < objects.size(); ++i)
    {
      REQUIRE(objects[i]->value == static_cast<int>(i));
    }

    // Clean up
    for (auto* obj : objects)
    {
      obj->~TestObject();
      pool.deallocate(obj);
    }
  }
}

TEST_CASE("object_pool arbitrary alignment support", "[object_pool]")
{
  SECTION("16-byte aligned objects")
  {
    ouly::object_pool<AlignedObject> pool;

    std::vector<AlignedObject*> objects;
    for (int i = 0; i < 20; ++i)
    {
      auto* obj = pool.allocate();
      REQUIRE(obj != nullptr);

      // Check alignment
      auto address = reinterpret_cast<std::uintptr_t>(obj);
      REQUIRE((address & 15) == 0); // 16-byte aligned

      new (obj) AlignedObject();
      REQUIRE(obj->value == 99);
      objects.push_back(obj);
    }

    // Clean up
    for (auto* obj : objects)
    {
      obj->~AlignedObject();
      pool.deallocate(obj);
    }
  }

  SECTION("64-byte aligned objects")
  {
    ouly::object_pool<HighlyAlignedObject> pool;

    std::vector<HighlyAlignedObject*> objects;
    for (int i = 0; i < 10; ++i)
    {
      auto* obj = pool.allocate();
      REQUIRE(obj != nullptr);

      // Check 64-byte alignment
      auto address = reinterpret_cast<std::uintptr_t>(obj);
      REQUIRE((address & 63) == 0); // 64-byte aligned

      new (obj) HighlyAlignedObject();
      REQUIRE(obj->value == 1.23_a);
      objects.push_back(obj);
    }

    // Clean up
    for (auto* obj : objects)
    {
      obj->~HighlyAlignedObject();
      pool.deallocate(obj);
    }
  }
}

TEST_CASE("object_pool move semantics", "[object_pool]")
{
  ouly::object_pool<TestObject> pool1;

  // Allocate some objects
  auto* obj1 = pool1.allocate();
  auto* obj2 = pool1.allocate();
  REQUIRE(obj1 != nullptr);
  REQUIRE(obj2 != nullptr);

  // Initialize objects
  new (obj1) TestObject(10, 1.0);
  new (obj2) TestObject(20, 2.0);

  // Move construct
  ouly::object_pool<TestObject> pool2 = std::move(pool1);

  // pool1 should be empty after move
  REQUIRE(pool1.empty());

  // pool2 should have the objects
  auto* obj3 = pool2.allocate(); // Should work fine
  REQUIRE(obj3 != nullptr);
  new (obj3) TestObject(30, 3.0);

  // Cleanup objects properly
  obj1->~TestObject();
  obj2->~TestObject();
  obj3->~TestObject();

  pool2.deallocate(obj1);
  pool2.deallocate(obj2);
  pool2.deallocate(obj3);

  SECTION("move assignment")
  {
    ouly::object_pool<TestObject> pool3;

    // Allocate and immediately deallocate from pool3 to test it works
    auto* temp_obj = pool3.allocate();
    REQUIRE(temp_obj != nullptr);
    new (temp_obj) TestObject(99, 9.9);
    temp_obj->~TestObject();
    pool3.deallocate(temp_obj);

    // Now move pool2 to pool3
    pool3 = std::move(pool2);

    // pool3 should work fine
    auto* obj5 = pool3.allocate();
    REQUIRE(obj5 != nullptr);
    new (obj5) TestObject(50, 5.0);

    obj5->~TestObject();
    pool3.deallocate(obj5);
  }
}
// NOLINTEND