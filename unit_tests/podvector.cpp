
#include "test_common.hpp"
#include "acl/containers/podvector.hpp"
#include "catch2/catch_all.hpp"

// NOLINTBEGIN
TEST_CASE("podvector: Validate podvector emplace", "[podvector][emplace]")
{
  acl::podvector<pod> v1, v2;
  v1.emplace_back(pod{45, 66});
  v1.emplace_back(pod{425, 166});
  v2.emplace_back(pod{45, 66});
  v2.emplace_back(pod{425, 166});
  REQUIRE(v1 == v2);
  REQUIRE(v1.back().a == 425);
  REQUIRE(v2.back().b == 166);
}

TEST_CASE("podvector: Validate podvector assign", "[podvector][assign]")
{
  acl::podvector<pod> v1, v2;
  v1.assign({
   pod{std::rand(), std::rand()},
   pod{std::rand(), std::rand()}
  });
  v2.assign(v1.begin(), v1.end());
  REQUIRE(v1 == v2);
  auto saved = pod{std::rand(), std::rand()};
  v1.assign(10, saved);
  v2.assign(10, saved);
  REQUIRE(v1.size() == 10);
  REQUIRE(v1 == v2);
  REQUIRE(v1.back().a == saved.a);
  REQUIRE(v2.back().b == saved.b);
  REQUIRE(v1.at(0).a == saved.a);
  REQUIRE(v2.at(0).b == saved.b);
  v2.clear();
  REQUIRE(v2.size() == 0);
  REQUIRE(v2.capacity() != 0);
  v2.shrink_to_fit();
  REQUIRE(v2.capacity() == 0);
}

TEST_CASE("podvector: Validate podvector insert", "[podvector][insert]")
{
  acl::podvector<pod> v1;
  v1.insert(v1.end(), pod{100, 200});
  v1.insert(v1.end(), {
                       pod{300, 400},
                       pod{500, 600},
                       pod{255, 111}
  });

  acl::podvector<pod> v2 = {
   pod{100, 200},
   pod{300, 400},
   pod{500, 600},
   pod{255, 111}
  };
  REQUIRE(v1 == v2);
  v1.insert(v1.begin() + 1, pod{10, 20});
  REQUIRE(v1[1].a == 10);
  REQUIRE(v1[1].b == 20);
}

TEST_CASE("podvector: Validate podvector erase", "[podvector][erase]")
{
  acl::podvector<pod> v1;
  v1.insert(v1.end(), {
                       pod{100, 200},
                       pod{300, 400},
                       pod{500, 600},
                       pod{255, 111}
  });
  REQUIRE(v1.size() == 4);
  v1.erase(v1.begin() + 2);
  REQUIRE(v1.size() == 3);
  REQUIRE(v1.back().a == 255);
  REQUIRE(v1.back().b == 111);
  REQUIRE(v1[2].a == 255);
  REQUIRE(v1[2].b == 111);
  v1.insert(v1.end(), {
                       pod{100, 200},
                       pod{300, 400},
                       pod{500, 600},
                       pod{255, 111}
  });
  v1.erase(v1.begin(), v1.begin() + 3);
  REQUIRE(v1.size() == 4);
  acl::podvector other = {
   pod{100, 200},
   pod{300, 400},
   pod{500, 600},
   pod{255, 111}
  };
  REQUIRE(other == v1);
}
// NOLINTEND