#include "ouly/containers/block_vector.hpp"
#include "catch2/catch_all.hpp"
#include "ouly/containers/soavector.hpp"
#include "ouly/ecs/entity.hpp"
#include "ouly/ecs/map.hpp"
#include <string>
#include <vector>

// NOLINTBEGIN
namespace
{
struct pack
{
  int         id    = 0;
  float       value = 0.0F;
  std::string name;

  auto operator==(pack const&) const -> bool = default;
};
} // namespace

TEST_CASE("block_vector: std::vector backing keeps block-sized storage", "[block_vector]")
{
  ouly::block_vector<int, 4> values;

  REQUIRE(values.empty());
  REQUIRE(values.size() == 0);
  REQUIRE(values.storage_size() == 0);

  for (int i = 0; i < 5; ++i)
  {
    values.emplace_back(i + 1);
  }

  REQUIRE(values.size() == 5);
  REQUIRE(values.storage_size() == 8);
  REQUIRE(values.block_count() == 2);
  REQUIRE(values[4] == 5);

  values.pop_back();

  REQUIRE(values.size() == 4);
  REQUIRE(values.storage_size() == 4);
  REQUIRE(values.back() == 4);
}

TEST_CASE("block_vector: resize resets logical tail lanes", "[block_vector]")
{
  ouly::block_vector<int, 4> values;

  values.resize(3);
  values[2] = 42;
  values.resize(1);
  values.resize(3);

  REQUIRE(values.size() == 3);
  REQUIRE(values.storage_size() == 4);
  REQUIRE(values[2] == 0);
}

TEST_CASE("block_vector: soavector backing exposes single elements and blocks", "[block_vector][soavector]")
{
  using storage_type = ouly::soavector<pack>;
  ouly::block_vector<pack, 4, storage_type> values;

  values.emplace_back(1, 10.0F, "one");
  values.emplace_back(2, 20.0F, "two");

  REQUIRE(values.size() == 2);
  REQUIRE(values.storage_size() == 4);
  REQUIRE(values.block_count() == 1);
  REQUIRE(values[0].get() == pack{1, 10.0F, "one"});
  REQUIRE(values[1].get() == pack{2, 20.0F, "two"});
  REQUIRE(values.data<0>() != nullptr);
  REQUIRE(values.data<1>() != nullptr);
  REQUIRE(values.data<2>() != nullptr);
}

TEST_CASE("block_vector: ecs map erase_and_swap_values keeps packed soavector blocks", "[block_vector][ecs]")
{
  using entity_type  = ouly::ecs::entity<>;
  using storage_type = ouly::soavector<pack>;

  ouly::ecs::map<entity_type>               entity_map;
  ouly::block_vector<pack, 4, storage_type> values;
  std::vector<int>                          side_band;
  entity_type                               e1{10};
  entity_type                               e2{20};
  entity_type                               e3{30};

  auto idx1 = entity_map.emplace(e1);
  values.resize(entity_map.size());
  side_band.resize(entity_map.size());
  values[idx1]    = pack{1, 10.0F, "one"};
  side_band[idx1] = 100;

  auto idx2 = entity_map.emplace(e2);
  values.resize(entity_map.size());
  side_band.resize(entity_map.size());
  values[idx2]    = pack{2, 20.0F, "two"};
  side_band[idx2] = 200;

  auto idx3 = entity_map.emplace(e3);
  values.resize(entity_map.size());
  side_band.resize(entity_map.size());
  values[idx3]    = pack{3, 30.0F, "three"};
  side_band[idx3] = 300;

  entity_map.erase_and_swap_values(e2, values, side_band);

  REQUIRE(entity_map.size() == 2);
  REQUIRE(values.size() == 2);
  REQUIRE(values.storage_size() == 4);
  REQUIRE_FALSE(entity_map.contains(e2));
  REQUIRE(values[entity_map.key(e1)].get() == pack{1, 10.0F, "one"});
  REQUIRE(values[entity_map.key(e3)].get() == pack{3, 30.0F, "three"});
  REQUIRE(side_band[entity_map.key(e1)] == 100);
  REQUIRE(side_band[entity_map.key(e3)] == 300);
}
// NOLINTEND
