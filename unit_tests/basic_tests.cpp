
#include "catch2/catch_all.hpp"
#include "ouly/allocators/allocator.hpp"
#include "ouly/allocators/default_allocator.hpp"
#include "ouly/allocators/std_allocator_wrapper.hpp"
#include "ouly/containers/index_map.hpp"
#include "ouly/containers/sparse_table.hpp"
#include "ouly/reflection/visitor.hpp"
#include "ouly/utility/delegate.hpp"
#include "ouly/utility/intrusive_ptr.hpp"
#include "ouly/utility/komihash.hpp"
#include "ouly/utility/tagged_ptr.hpp"
#include "ouly/utility/wyhash.hpp"
#include "ouly/utility/zip_view.hpp"
#include "test_common.hpp"
#include <span>

// NOLINTBEGIN

#define BINARY_SEARCH_STEP                                                                                             \
  {                                                                                                                    \
    const size_t* const middle = it + (size >> 1);                                                                     \
    size                       = (size + 1) >> 1;                                                                      \
    it                         = *middle < key ? middle : it;                                                          \
  }

static inline auto mini0(size_t const* it, size_t size, size_t key) noexcept
{
  while (size > 2)
    BINARY_SEARCH_STEP;
  it += static_cast<size_t>(size > 1 && (*it < key));
  it += static_cast<size_t>(size > 0 && (*it < key));
  return it;
}

TEST_CASE("Lower bound")
{
  std::vector<size_t> vec{3, 20, 60, 400};

  const auto* i = mini0(vec.data(), 3, 40);
  REQUIRE(i < vec.data() + vec.size());
}

struct myBase
{
  int& c;
  myBase(int& a) : c(a) {}
  virtual ~myBase() = default;
};

struct myClass : myBase
{
  myClass(int& a) : myBase(a) {}
  ~myClass()
  {
    c = -1;
  }
};

int intrusive_count_add(myBase* m)
{
  return m->c++;
}
int intrusive_count_sub(myBase* m)
{
  return m->c--;
}
int intrusive_count_get(myBase* m)
{
  return m->c;
}

TEST_CASE("Validate smart pointer", "[intrusive_ptr]")
{
  int check = 0;

  ouly::intrusive_ptr<myClass> ptr;
  CHECK(ptr == nullptr);
  ptr = ouly::intrusive_ptr<myClass>(new myClass(check));
  CHECK(ptr != nullptr);
  CHECK(ptr.use_count() == 1);
  auto copy = ptr;
  CHECK(ptr.use_count() == 2);
  CHECK(ptr->c == 2);
  ptr.reset();
  CHECK(check == 1);
  ptr = std::move(copy);
  CHECK(ptr.use_count() == 1);
  // Static cast
  ouly::intrusive_ptr<myBase> base  = ouly::static_pointer_cast<myBase>(ptr);
  ouly::intrusive_ptr<myBase> base2 = ptr;
  CHECK(ptr.use_count() == 3);

  base.reset();
  ptr.reset();
  base2.reset();

  CHECK(check == -1);
}

TEST_CASE("Validate general_allocator", "[general_allocator]")
{

  using namespace ouly;
  using allocator_t   = default_allocator<ouly::config<ouly::cfg::compute_stats>>;
  using std_allocator = allocator_wrapper<int, allocator_t>;
  [[maybe_unused]] std_allocator allocator;

  allocator_t::address data = allocator_t::allocate(256, ouly::alignment<64>());
  CHECK((reinterpret_cast<std::uintptr_t>(data) & 63) == 0);
  allocator_t::deallocate(data, 256, ouly::alignment<64>());
  // Should be fine to free nullptr
  allocator_t::deallocate(nullptr, 0, {});
}

TEST_CASE("Validate tagged_ptr", "[tagged_ptr]")
{
  using namespace ouly;
  ouly::detail::tagged_ptr<std::string> tagged_string;

  std::string my_string = "This is my string";
  std::string copy      = my_string;

  tagged_string.set(&my_string, 1);

  CHECK(tagged_string.get_ptr() == &my_string);
  CHECK(*tagged_string.get_ptr() == copy);

  tagged_string.set(&my_string, tagged_string.get_next_tag());

  CHECK(tagged_string.get_tag() == 2);

  CHECK(tagged_string.get_ptr() == &my_string);
  CHECK(*tagged_string.get_ptr() == copy);

  auto second = ouly::detail::tagged_ptr(&my_string, 2);
  CHECK((tagged_string == second) == true);

  tagged_string.set(&my_string, tagged_string.get_next_tag());
  CHECK(tagged_string != second);

  ouly::detail::tagged_ptr<std::void_t<>> null = nullptr;
  CHECK(!null);
}

TEST_CASE("Validate compressed_ptr", "[compressed_ptr]")
{
  using namespace ouly;
  ouly::detail::compressed_ptr<std::string> tagged_string;

  std::string my_string = "This is my string";
  std::string copy      = my_string;

  tagged_string.set(&my_string, 1);

  CHECK(tagged_string.get_ptr() == &my_string);
  CHECK(*tagged_string.get_ptr() == copy);

  tagged_string.set(&my_string, tagged_string.get_next_tag());

  CHECK(tagged_string.get_tag() == 2);

  CHECK(tagged_string.get_ptr() == &my_string);
  CHECK(*tagged_string.get_ptr() == copy);

  auto second = ouly::detail::compressed_ptr(&my_string, 2);
  CHECK((tagged_string == second) == true);

  tagged_string.set(&my_string, tagged_string.get_next_tag());
  CHECK(tagged_string != second);

  ouly::detail::compressed_ptr<std::void_t<>> null = nullptr;
  CHECK(!null);
}

TEST_CASE("Validate Hash: wyhash", "[hash]")
{
  constexpr std::string_view cs = "A long string whose hash we are about to find out !";
  std::string                s{cs};
  ouly::wyhash32             wyh32;

  auto value32 = wyh32(s.c_str(), s.length());
  REQUIRE(value32 != 0);

  auto new_value32 = wyh32(s.c_str(), s.length());
  REQUIRE(value32 != new_value32);

  constexpr auto constval = ouly::wyhash32::make(cs.data(), cs.size());
  REQUIRE(value32 == constval);

  REQUIRE(wyh32() == new_value32);

  ouly::wyhash64 wyh64;

  auto value64 = wyh64(s.c_str(), s.length());
  REQUIRE(value64 != 0);

  auto new_value64 = wyh64(s.c_str(), s.length());
  REQUIRE(value64 != new_value64);

  REQUIRE(wyh64() == new_value64);
}

TEST_CASE("Validate Hash: komihash", "[hash]")
{
  std::string      s = "A long string whose hash we are about to find out !";
  ouly::komihash64 k64;

  auto value = k64(s.c_str(), s.length());
  REQUIRE(value != 0);

  auto new_value = k64(s.c_str(), s.length());
  REQUIRE(value != new_value);

  REQUIRE(k64() == new_value);

  ouly::komihash64_stream k64s;

  k64s(s.c_str(), s.length());
  k64s(s.c_str(), s.length());

  REQUIRE(k64s() != 0);
}

TEST_CASE("Test index_map", "[index_map]")
{
  ouly::index_map<uint32_t, 5> map;

  REQUIRE(map.empty() == true);
  map[24] = 5;
  REQUIRE(map.size() == 1);
  REQUIRE(map[24] == 5);
  map[25] = 25;
  REQUIRE(map[24] == 5);
  REQUIRE(map[25] == 25);
  REQUIRE(map.base_offset() == 24);
  map[15] = 15;
  REQUIRE(map[24] == 5);
  REQUIRE(map[25] == 25);
  REQUIRE(map[15] == 15);
  REQUIRE(map.size() == 11);
  REQUIRE(map.base_offset() == 15);
  map[40] = 40;
  REQUIRE(map[24] == 5);
  REQUIRE(map[25] == 25);
  REQUIRE(map[15] == 15);
  REQUIRE(map[40] == 40);
  REQUIRE(map.size() == 26);
  REQUIRE(map.base_offset() == 15);
  map[31] = 31;
  REQUIRE(map[24] == 5);
  REQUIRE(map[25] == 25);
  REQUIRE(map[15] == 15);
  REQUIRE(map[40] == 40);
  REQUIRE(map[31] == 31);
  REQUIRE(map.base_offset() == 15);
  map[41] = 41;
  REQUIRE(map[24] == 5);
  REQUIRE(map[25] == 25);
  REQUIRE(map[15] == 15);
  REQUIRE(map[40] == 40);
  REQUIRE(map[31] == 31);
  REQUIRE(map[41] == 41);
  REQUIRE(map.base_offset() == 15);
  map[2] = 2;
  REQUIRE(map[2] == 2);
  REQUIRE(map[15] == 15);
  REQUIRE(map[24] == 5);
  REQUIRE(map[25] == 25);
  REQUIRE(map[31] == 31);
  REQUIRE(map[40] == 40);
  REQUIRE(map[41] == 41);
  REQUIRE(map.base_offset() == 0);
  REQUIRE(map.empty() == false);

  auto     data = std::array{2, 15, 5, 25, 31, 40, 41};
  uint32_t it   = 0;
  for (auto& i : map)
  {
    if (i != map.null)
    {
      REQUIRE(i == static_cast<unsigned>(data[it++]));
    }
  }

  for (auto r = map.rbegin(); r != map.rend(); ++r)
  {
    if (*r != map.null)
    {
      REQUIRE(*r == static_cast<unsigned>(data[--it]));
    }
  }
}

TEST_CASE("Test index_map fuzz test", "[index_map]")
{
  ouly::index_map<uint32_t, 64> map;
  std::vector<uint32_t>         full_map;

  uint32_t fixed_seed = Catch::rngSeed();

  auto seed = xorshift32(fixed_seed);
  auto end  = seed % 100;
  full_map.resize(100, 99999);
  for (uint32_t i = 0; i < end; ++i)
  {
    auto idx      = (seed = xorshift32(seed)) % 100;
    full_map[idx] = map[idx] = ((seed = xorshift32(seed)) % 1000);
  }

  for (uint32_t i = 0; i < end; ++i)
  {
    if (full_map[i] == 99999)
      REQUIRE(map[i] == map.null);
    else
      REQUIRE(map[i] == full_map[i]);
  }
}

TEST_CASE("Test zip_view", "[zip_view]")
{
  std::vector<std::string> strings;
  std::vector<int32_t>     integers;

  for (int i = 0; i < 10; ++i)
  {
    strings.emplace_back(std::to_string(i) + "-item");
    integers.emplace_back(i * 10);
  }
  int start = 0;
  for (auto&& [val, ints] : ouly::zip(strings, integers))
  {
    REQUIRE(val == std::to_string(start) + "-item");
    REQUIRE(ints == start * 10);
    start++;
  }

  start = 0;
  for (auto&& [val, ints] : ouly::zip(std::span(strings), std::span(integers)))
  {
    REQUIRE(val == std::to_string(start) + "-item");
    REQUIRE(ints == start * 10);
    start++;
  }
}

// Free function for testing
int free_function(int a, int b)
{
  return a + b;
}

// A class with member functions for testing
class MyClass
{
public:
  int add(int a, int b) const
  {
    return a + b;
  }

  int multiply(int a, int b)
  {
    return a * b;
  }
  int def = 0;
};

using test_delegate_t = ouly::delegate<int(int, int)>;
TEST_CASE("Test free function delegate", "[delegate]")
{
  auto del = test_delegate_t::bind(free_function);
  REQUIRE(static_cast<bool>(del) == true); // Ensure the delegate was initialized correctly
  REQUIRE(del(3, 4) == 7);                 // Check that it correctly calls the free function

  del = test_delegate_t::bind(free_function);
  REQUIRE(static_cast<bool>(del) == true); // Ensure the delegate was initialized correctly
  REQUIRE(del(3, 4) == 7);                 // Check that it correctly calls the free function
}

TEST_CASE("Test lambda delegate", "[delegate]")
{
  MyClass obj;
  auto    lambda = [&obj](int a, int b)
  {
    return a * b + obj.def;
  };

  auto del = test_delegate_t::bind(lambda);
  REQUIRE(static_cast<bool>(del) == true);
  REQUIRE(del(3, 4) == 12); // Ensure the lambda is correctly called

  // copy constructor
  auto copy = del;
  del       = copy;

  obj.def = 10;
  REQUIRE(copy(3, 4) == 22); // Ensure the lambda is correctly called
}

TEST_CASE("Test member function delegate", "[delegate]")
{
  MyClass obj;

  SECTION("Test non-const member function")
  {
    auto del = test_delegate_t::bind(
     [&obj](int a, int b)
     {
       return obj.multiply(a, b);
     });
    REQUIRE(static_cast<bool>(del) == true);
    REQUIRE(del(3, 4) == 12); // Test non-const member function invocation
  }

  SECTION("Test const member function")
  {
    auto del = test_delegate_t::bind(
     [&obj](int a, int b) -> int
     {
       return obj.add(a, b);
     });
    REQUIRE(static_cast<bool>(del) == true);
    REQUIRE(del(3, 4) == 7); // Test const member function invocation
  }
}

TEST_CASE("Test move semantics", "[delegate]")
{
  auto lambda = [](int a, int b)
  {
    return a * b;
  };
  auto del = test_delegate_t::bind(lambda);

  // Move the delegate to another instance
  ouly::delegate<int(int, int)> moved = std::move(del);

  REQUIRE(static_cast<bool>(moved) == true);
  REQUIRE(moved(5, 6) == 30); // Ensure the moved delegate still works

  // The original delegate should now be empty
  // REQUIRE(static_cast<bool>(del) == false);
}

TEST_CASE("Test empty delegate behavior", "[delegate]")
{
  ouly::delegate<int(int, int)> del;
  REQUIRE(static_cast<bool>(del) == false); // Delegate should be empty initially
}

TEST_CASE("Test direct delegate behavior", "[delegate]")
{
  int  value  = 20;
  auto lambda = [value](int a, int b) -> int
  {
    return a + b + value;
  };

  auto del = test_delegate_t::bind(lambda);

  REQUIRE(static_cast<bool>(del) == true);
  REQUIRE(del(10, 5) == 35);
}

TEST_CASE("Test tuple expand delegate behavior", "[delegate]")
{
  int  value1 = 20;
  int  value2 = 30;
  auto lambda = [value1, value2](int a, int b) -> int
  {
    return a + b + value1 + value2;
  };
  auto del = test_delegate_t::bind(lambda);
  REQUIRE(static_cast<bool>(del) == true);
  REQUIRE(del(10, 5) == 65);
}

std::string throw_error(ouly::visitor_error::code code)
{
  try
  {
    throw ouly::visitor_error(code);
  }
  catch (const ouly::visitor_error& e)
  {
    return e.what();
  }
}

TEST_CASE("Test visitor error codes and strings", "[visitor]")
{
  /*

    case invalid_tuple:
      return "Invalid tuple";
    case invalid_container:
      return "Invalid container";
    case invalid_variant:
      return "Invalid variant";
    case invalid_variant_type:
      return "Invalid variant type";
    case invalid_aggregate:
      return "Invalid aggregate";
    case invalid_null_sentinel:
      return "Invalid null sentinel";
    case invalid_value:
      return "Invalid value";
    case invalid_key:
      return "Invalid key";
    case type_is_not_an_object:
      return "Type is not an object";
    case type_is_not_an_array:
      return "Type is not an array";
    default:
      return "Unknown visitor error";
   */
  REQUIRE(throw_error(ouly::visitor_error::code::invalid_tuple) == "Invalid tuple");
  REQUIRE(throw_error(ouly::visitor_error::code::invalid_container) == "Invalid container");
  REQUIRE(throw_error(ouly::visitor_error::code::invalid_variant) == "Invalid variant");
  REQUIRE(throw_error(ouly::visitor_error::code::invalid_variant_type) == "Invalid variant type");
  REQUIRE(throw_error(ouly::visitor_error::code::invalid_aggregate) == "Invalid aggregate");
  REQUIRE(throw_error(ouly::visitor_error::code::invalid_null_sentinel) == "Invalid null sentinel");
  REQUIRE(throw_error(ouly::visitor_error::code::invalid_value) == "Invalid value");
  REQUIRE(throw_error(ouly::visitor_error::code::invalid_key) == "Invalid key");
  REQUIRE(throw_error(ouly::visitor_error::code::type_is_not_an_object) == "Type is not an object");
  REQUIRE(throw_error(ouly::visitor_error::code::type_is_not_an_array) == "Type is not an array");
  REQUIRE(throw_error(ouly::visitor_error::code::unknown) == "Unknown visitor error");
}
// NOLINTEND
