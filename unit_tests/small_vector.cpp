
#include "ouly/containers/small_vector.hpp"
#include "catch2/catch_all.hpp"
#include "ouly/allocators/linear_allocator.hpp"
#include "test_common.hpp"
#include <compare>
#include <utility>

// NOLINTBEGIN
TEST_CASE("small_vector: Validate small_vector emplace", "[small_vector][emplace]")
{
  ouly::small_vector<pod> v1, v2;
  v1.emplace_back(pod{45, 66});
  v1.emplace_back(pod{425, 166});
  v2.emplace_back(pod{45, 66});
  v2.emplace_back(pod{425, 166});
  REQUIRE(v1 == v2);
  REQUIRE(v1.back().a == 425);
  REQUIRE(v2.back().b == 166);
}

TEST_CASE("small_vector: Validate small_vector assign", "[small_vector][assign]")
{
  ouly::small_vector<pod> v1, v2;
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
  REQUIRE(v2.capacity() == v2.get_inlined_capacity());
}

TEST_CASE("small_vector: Validate small_vector insert", "[small_vector][insert]")
{
  ouly::small_vector<pod> v1;
  v1.insert(v1.end(), pod{100, 200});
  v1.insert(v1.end(), {
                       pod{300, 400},
                       pod{500, 600},
                       pod{255, 111}
  });

  ouly::small_vector<pod> v2 = {
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

TEST_CASE("small_vector: Validate small_vector erase", "[small_vector][erase]")
{
  ouly::small_vector<pod> v1;
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
  ouly::small_vector other = {
   pod{100, 200},
   pod{300, 400},
   pod{500, 600},
   pod{255, 111}
  };
  REQUIRE(other == v1);
}

TEST_CASE("small_vector: size construct", "[small_vector][construct]")
{
  auto test_size = [](uint32_t size, bool inlined)
  {
    auto v1 = ouly::small_vector<std::string, 5>(size);
    REQUIRE(v1.size() == size);
    for (uint32_t i = 0; i < size; ++i)
      v1[i] = to_lstring(i);
    REQUIRE(v1.is_inlined() == inlined);
    for (uint32_t i = 0; i < size; ++i)
      REQUIRE(v1[i] == to_lstring(i));
  };

  test_size(5, true);
  test_size(10, false);
}

TEST_CASE("small_vector: size and value construct", "[small_vector][construct]")
{
  auto test_size = [](uint32_t size, bool inlined)
  {
    auto s  = to_lstring(size);
    auto v1 = ouly::small_vector<std::string, 5>(size, s);
    for (uint32_t i = 0; i < size; ++i)
      REQUIRE(v1[i] == s);
    REQUIRE(v1.is_inlined() == inlined);
  };

  test_size(5, true);
  test_size(10, false);
}

TEST_CASE("small_vector: construct from range", "[small_vector][construct]")
{
  auto test_size = [](uint32_t size, bool inlined)
  {
    std::vector<std::string> values;
    for (uint32_t i = 0; i < size; ++i)
      values.emplace_back(to_lstring(i));
    auto v1 = ouly::small_vector<std::string, 5>(values.begin(), values.end());
    for (uint32_t i = 0; i < size; ++i)
      REQUIRE(v1[i] == values[i]);
    REQUIRE(v1.is_inlined() == inlined);
  };

  test_size(5, true);
  test_size(10, false);
}

TEST_CASE("small_vector: copy ctor", "[small_vector][construct]")
{
  auto test_size = [](uint32_t size, bool inlined)
  {
    ouly::small_vector<std::string, 5> values;
    for (uint32_t i = 0; i < size; ++i)
      values.emplace_back(to_lstring(i));
    auto v1 = ouly::small_vector<std::string, 5>(values);
    for (uint32_t i = 0; i < size; ++i)
      REQUIRE(v1[i] == values[i]);
    REQUIRE(v1.is_inlined() == inlined);
  };

  test_size(5, true);
  test_size(10, false);
}

TEST_CASE("small_vector: move ctor", "[small_vector][construct]")
{
  auto test_size = [](uint32_t size, bool inlined)
  {
    ouly::small_vector<std::string, 5> values;
    for (uint32_t i = 0; i < size; ++i)
      values.emplace_back(to_lstring(i));
    auto v1 = ouly::small_vector<std::string, 5>(values);
    auto v2 = ouly::small_vector<std::string, 5>(std::move(v1));
    for (uint32_t i = 0; i < size; ++i)
      REQUIRE(v2[i] == values[i]);
    REQUIRE(v2.is_inlined() == inlined);
    REQUIRE(v1.empty() == true);
  };

  test_size(5, true);
  test_size(10, false);
}

TEST_CASE("small_vector: copy/move ctor with linear_allocator", "[small_vector][construct]")
{
  auto test_size = [](uint32_t size, bool inlined)
  {
    using type = ouly::small_vector<std::string, 5>;
    type values;
    for (uint32_t i = 0; i < size; ++i)
      values.emplace_back(to_lstring(i));
    auto v1 = type(values, ouly::default_allocator<>());
    auto v2 = type(std::move(v1));
    auto v3 = type(std::move(v2), ouly::default_allocator<>());
    REQUIRE(v1.empty() == true);
    REQUIRE(v2.empty() == true);
    for (uint32_t i = 0; i < size; ++i)
      REQUIRE(v3[i] == values[i]);
    REQUIRE(v3.is_inlined() == inlined);
    auto v4 = v3;
    for (uint32_t i = 0; i < size; ++i)
      REQUIRE(v4[i] == values[i]);
  };

  test_size(5, true);
  test_size(10, false);
}

TEST_CASE("small_vector: initializer", "[small_vector][construct]")
{
  auto test_size = []<uint32_t... I>(std::integer_sequence<uint32_t, I...> seq, bool inlined)
  {
    using type  = ouly::small_vector<std::string, 5>;
    type inited = {to_lstring(I)...};
    type values;
    for (uint32_t i = 0; i < seq.size(); ++i)
      values.emplace_back(to_lstring(i));
    REQUIRE(values.size() == inited.size());
    for (uint32_t i = 0; i < seq.size(); ++i)
      REQUIRE(values[i] == inited[i]);
    REQUIRE(values.is_inlined() == inlined);
    REQUIRE(inited.is_inlined() == inlined);
  };

  test_size(std::make_integer_sequence<uint32_t, 5>(), true);
  test_size(std::make_integer_sequence<uint32_t, 10>(), false);
}

TEST_CASE("small_vector: shrink to fit", "[small_vector][shrink_to_fit]")
{
  tracker a, b, c, d, e, f;

  ouly::small_vector<destroy_tracker, 4> v1;
  v1.push_back(destroy_tracker(a));
  v1.push_back(destroy_tracker(b));
  v1.push_back(destroy_tracker(c));
  v1.push_back(destroy_tracker(d));
  v1.push_back(destroy_tracker(e));
  v1.push_back(destroy_tracker(f));
  REQUIRE(v1.capacity() >= 6);
  REQUIRE(f.tracking == 1);
  v1.pop_back();
  REQUIRE(f.tracking == 0);
  v1.shrink_to_fit();
  REQUIRE(v1.capacity() <= 5);
  v1.pop_back();
  REQUIRE(e.tracking == 0);
  v1.shrink_to_fit();
  REQUIRE(v1.capacity() < 5);
  REQUIRE(v1.back() == destroy_tracker(d));
  v1.pop_back();
  v1.pop_back();
  REQUIRE(v1.back() == destroy_tracker(b));
  REQUIRE(v1.is_inlined() == true);

  v1.clear();

  REQUIRE(a.tracking == 0);
  REQUIRE(b.tracking == 0);
}

TEST_CASE("small_vector: insert", "[small_vector][insert]")
{
  tracker a('a'), b('b'), c('c'), d('d'), e('e'), f('f'), g('g'), h('h');

  ouly::small_vector<destroy_tracker, 4> v1;

  v1.insert(v1.cend(), destroy_tracker(c));
  REQUIRE(v1.size() == 1);
  REQUIRE(v1.at(0) == destroy_tracker(c));
  REQUIRE(v1.is_inlined());

  v1.insert(v1.cend(), destroy_tracker(d));
  REQUIRE(v1.size() == 2);
  REQUIRE(v1.at(1) == destroy_tracker(d));
  REQUIRE(v1.is_inlined());

  v1.insert(v1.begin(), destroy_tracker(a));
  REQUIRE(v1.size() == 3);
  REQUIRE(v1.at(0) == destroy_tracker(a));
  REQUIRE(v1.at(1) == destroy_tracker(c));
  REQUIRE(v1.at(2) == destroy_tracker(d));
  REQUIRE(v1.is_inlined());

  v1.insert(v1.begin() + 1, destroy_tracker(b));
  REQUIRE(v1.size() == 4);
  REQUIRE(v1.at(0) == destroy_tracker(a));
  REQUIRE(v1.at(1) == destroy_tracker(b));
  REQUIRE(v1.at(2) == destroy_tracker(c));
  REQUIRE(v1.at(3) == destroy_tracker(d));
  REQUIRE(v1.is_inlined());

  v1.pop_back();
  v1.pop_back();
  REQUIRE(v1.size() == 2);

  v1.insert(v1.cbegin(), {destroy_tracker(c), destroy_tracker(d), destroy_tracker(e)});
  REQUIRE(v1.size() == 5);
  REQUIRE(v1.at(0) == destroy_tracker(c));
  REQUIRE(v1.at(1) == destroy_tracker(d));
  REQUIRE(v1.at(2) == destroy_tracker(e));
  REQUIRE(v1.at(3) == destroy_tracker(a));
  REQUIRE(v1.at(4) == destroy_tracker(b));

  v1.pop_back();
  v1.pop_back();

  v1.insert(v1.cend(), {destroy_tracker(a), destroy_tracker(b), destroy_tracker(f)});
  REQUIRE(v1.size() == 6);
  REQUIRE(v1.at(0) == destroy_tracker(c));
  REQUIRE(v1.at(1) == destroy_tracker(d));
  REQUIRE(v1.at(2) == destroy_tracker(e));
  REQUIRE(v1.at(3) == destroy_tracker(a));
  REQUIRE(v1.at(4) == destroy_tracker(b));
  REQUIRE(v1.at(5) == destroy_tracker(f));

  v1.pop_back();
  v1.pop_back();
  v1.pop_back();

  v1.insert(v1.cbegin() + 1, {destroy_tracker(a), destroy_tracker(b), destroy_tracker(f)});
  REQUIRE(v1.size() == 6);
  REQUIRE(v1.at(0) == destroy_tracker(c));
  REQUIRE(v1.at(1) == destroy_tracker(a));
  REQUIRE(v1.at(2) == destroy_tracker(b));
  REQUIRE(v1.at(3) == destroy_tracker(f));
  REQUIRE(v1.at(4) == destroy_tracker(d));
  REQUIRE(v1.at(5) == destroy_tracker(e));

  v1.clear();
  v1.insert(v1.begin(), destroy_tracker(a));
  v1.insert(v1.begin(), destroy_tracker(b));
  v1.insert(v1.begin(), destroy_tracker(c));

  std::vector<destroy_tracker> list;
  list.push_back(destroy_tracker(d));
  list.push_back(destroy_tracker(e));
  list.push_back(destroy_tracker(f));

  v1.insert(v1.begin(), list.begin(), list.end());
  REQUIRE(v1.at(0) == destroy_tracker(d));
  REQUIRE(v1.at(1) == destroy_tracker(e));
  REQUIRE(v1.at(2) == destroy_tracker(f));
  REQUIRE(v1.at(3) == destroy_tracker(c));
  REQUIRE(v1.at(4) == destroy_tracker(b));
  REQUIRE(v1.at(5) == destroy_tracker(a));

  v1.resize(3);
  REQUIRE(v1.size() == 3);
  v1.insert(v1.begin() + 1, list.begin(), list.end());
  REQUIRE(v1.at(0) == destroy_tracker(d));
  REQUIRE(v1.at(1) == destroy_tracker(d));
  REQUIRE(v1.at(2) == destroy_tracker(e));
  REQUIRE(v1.at(3) == destroy_tracker(f));
  REQUIRE(v1.at(4) == destroy_tracker(e));
  REQUIRE(v1.at(5) == destroy_tracker(f));
  REQUIRE(v1.size() == 6);
}

TEST_CASE("small_vector: erase", "[small_vector][erase]")
{
  tracker a('a'), b('b'), c('c'), d('d'), e('e'), f('f'), g('g'), h('h');

  ouly::small_vector<destroy_tracker, 4> v1;
  v1.push_back(destroy_tracker(a));
  v1.push_back(destroy_tracker(b));
  v1.push_back(destroy_tracker(c));
  v1.push_back(destroy_tracker(d));
  v1.push_back(destroy_tracker(e));
  v1.push_back(destroy_tracker(f));
  REQUIRE(v1.size() == 6);

  v1.erase(v1.begin());
  REQUIRE(v1.at(0) == destroy_tracker(b));
  REQUIRE(v1.at(1) == destroy_tracker(c));
  REQUIRE(v1.at(2) == destroy_tracker(d));
  REQUIRE(v1.at(3) == destroy_tracker(e));
  REQUIRE(v1.at(4) == destroy_tracker(f));
  REQUIRE(v1.size() == 5);
  REQUIRE(a.tracking == 0);

  v1.erase(v1.begin() + 1);
  REQUIRE(v1.at(0) == destroy_tracker(b));
  REQUIRE(v1.at(1) == destroy_tracker(d));
  REQUIRE(v1.at(2) == destroy_tracker(e));
  REQUIRE(v1.at(3) == destroy_tracker(f));
  REQUIRE(v1.size() == 4);
  REQUIRE(c.tracking == 0);
  REQUIRE(v1.is_inlined());

  v1.erase(v1.begin(), v1.begin() + 2);
  REQUIRE(v1.at(0) == destroy_tracker(e));
  REQUIRE(v1.at(1) == destroy_tracker(f));
  REQUIRE(v1.size() == 2);
  REQUIRE(v1.is_inlined());

  v1.push_back(destroy_tracker(a));
  v1.push_back(destroy_tracker(b));
  v1.push_back(destroy_tracker(c));
  v1.push_back(destroy_tracker(d));

  v1.erase(v1.begin() + 2, v1.begin() + 4);
  REQUIRE(v1.at(0) == destroy_tracker(e));
  REQUIRE(v1.at(1) == destroy_tracker(f));
  REQUIRE(v1.at(2) == destroy_tracker(c));
  REQUIRE(v1.at(3) == destroy_tracker(d));
  REQUIRE(v1.size() == 4);
  REQUIRE(v1.is_inlined());

  // A range that runs through end() is a valid erase.
  v1.erase(v1.begin() + 1, v1.end());
  REQUIRE(v1.size() == 1);
  REQUIRE(v1.at(0) == destroy_tracker(e));
  REQUIRE(f.tracking == 0);
  REQUIRE(c.tracking == 0);
  REQUIRE(d.tracking == 0);

  v1.erase(v1.begin(), v1.end());
  REQUIRE(v1.empty());
  REQUIRE(e.tracking == 0);

  v1.push_back(destroy_tracker(a));
  v1.erase(v1.end(), v1.end());
  REQUIRE(v1.size() == 1);
}

TEST_CASE("small_vector: capacity full self insertion", "[small_vector][default]")
{
  ouly::small_vector<std::string, 4> v1;
  v1.push_back("failure");
  v1.push_back("1");
  v1.push_back("2");
  v1.push_back("3");
  v1.push_back(v1[0]);

  REQUIRE(v1.size() == 5);

  REQUIRE(v1[3] == "3");
  REQUIRE(v1.back() == "failure");
  REQUIRE(v1.at(0) == "failure");
}
namespace
{
struct live_counter
{
  static inline int live  = 0;
  int               value = 0;
  live_counter() noexcept
  {
    ++live;
  }
  explicit live_counter(int v) noexcept : value(v)
  {
    ++live;
  }
  live_counter(live_counter const& o) noexcept : value(o.value)
  {
    ++live;
  }
  live_counter(live_counter&& o) noexcept : value(o.value)
  {
    ++live;
  }
  auto operator=(live_counter const&) noexcept -> live_counter& = default;
  auto operator=(live_counter&&) noexcept -> live_counter&      = default;
  ~live_counter()
  {
    --live;
  }
};
} // namespace

TEST_CASE("small_vector: resize destroys truncated elements", "[small_vector][resize]")
{
  live_counter::live = 0;
  {
    ouly::small_vector<live_counter, 4> v;
    v.resize(10);
    REQUIRE(live_counter::live == 10);
    v.resize(6); // heap -> heap
    REQUIRE(live_counter::live == 6);
    v.resize(3); // heap -> inline
    REQUIRE(live_counter::live == 3);
    v.resize(1); // inline -> inline
    REQUIRE(live_counter::live == 1);
    v.assign(8, live_counter(7));
    REQUIRE(live_counter::live == 8);
    v.assign(2, live_counter(3));
    REQUIRE(live_counter::live == 2);
  }
  REQUIRE(live_counter::live == 0);
}

TEST_CASE("small_vector: pop_back and move do not leak", "[small_vector][move]")
{
  live_counter::live = 0;
  {
    ouly::small_vector<live_counter, 4> v;
    for (int i = 0; i < 6; ++i)
    {
      v.emplace_back(i);
    }
    while (!v.empty())
    {
      v.pop_back();
      REQUIRE(live_counter::live == static_cast<int>(v.size()));
    }

    ouly::small_vector<live_counter, 4> a;
    a.emplace_back(1);
    a.emplace_back(2);
    ouly::small_vector<live_counter, 4> b = std::move(a);
    REQUIRE(b.size() == 2);
    REQUIRE(live_counter::live == 2);
    ouly::small_vector<live_counter, 4> c;
    c = std::move(b);
    REQUIRE(c[1].value == 2);
    REQUIRE(live_counter::live == 2);
  }
  REQUIRE(live_counter::live == 0);
}

TEST_CASE("small_vector: resize grows geometrically and assign keeps capacity", "[small_vector][capacity]")
{
  ouly::small_vector<int, 4> v;
  uint32_t                   reallocations = 0;
  auto                       last_cap      = v.capacity();
  for (uint32_t i = 1; i <= 10000; ++i)
  {
    v.resize(i);
    if (v.capacity() != last_cap)
    {
      ++reallocations;
      last_cap = v.capacity();
    }
  }
  REQUIRE(reallocations < 40);

  auto cap = v.capacity();
  v.assign(50, 1);
  REQUIRE(v.capacity() == cap);
  REQUIRE(v.size() == 50);
}
// NOLINTEND
