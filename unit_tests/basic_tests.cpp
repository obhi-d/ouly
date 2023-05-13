
#include <acl/default_allocator.hpp>
#include <acl/export.hxx>
#include <acl/intrusive_ptr.hpp>
#include <acl/packed_table.hpp>
#include <acl/sparse_table.hpp>
#include <acl/std_allocator_wrapper.hpp>
#include <acl/tagged_ptr.hpp>
#include <catch2/catch_all.hpp>

TEST_CASE("Validate malloc")
{
  char* data = (char*)acl::detail::malloc(100);
  std::memset(data, 1, 100);
  acl::detail::free(data);

  data = (char*)acl::detail::zmalloc(100);
  for (int i = 0; i < 100; ++i)
    REQUIRE(data[i] == 0);
  acl::detail::free(data);

  data = (char*)acl::detail::aligned_alloc(16, 16);
  REQUIRE((reinterpret_cast<uintptr_t>(data) & 15) == 0);
  acl::detail::aligned_free(data);

  data = (char*)acl::detail::aligned_zmalloc(16, 16);
  REQUIRE((reinterpret_cast<uintptr_t>(data) & 15) == 0);
  for (int i = 0; i < 16; ++i)
    REQUIRE(data[i] == 0);
  acl::detail::aligned_free(data);
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
  return ++m->c;
}
int intrusive_count_sub(myBase* m)
{
  return --m->c;
}
int intrusive_count_get(myBase* m)
{
  return m->c;
}

TEST_CASE("Validate general_allocator", "[intrusive_ptr]")
{
  int check = 0;

  acl::intrusive_ptr<myClass> ptr;
  CHECK(ptr == nullptr);
  ptr = acl::intrusive_ptr<myClass>(new myClass(check));
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
  acl::intrusive_ptr<myBase> base = acl::static_pointer_cast<myBase>(ptr);
  CHECK(ptr.use_count() == 2);

  base.reset();
  ptr.reset();

  CHECK(check == -1);
}

TEST_CASE("Validate general_allocator", "[general_allocator]")
{

  using namespace acl;
  using allocator_t   = default_allocator<std::uint32_t, true, false>;
  using std_allocator = allocator_wrapper<int, allocator_t>;
  std_allocator allocator;

  allocator_t::address data = allocator_t::allocate(256, 128);
  CHECK((reinterpret_cast<std::uintptr_t>(data) & 127) == 0);
  allocator_t::deallocate(data, 256, 128);
}

TEST_CASE("Validate tagged_ptr", "[tagged_ptr]")
{
  using namespace acl;
  tagged_ptr<std::string> tagged_string;

  std::string my_string = "This is my string";
  std::string copy      = my_string;

  tagged_string.set(&my_string, 1);

  CHECK(tagged_string.get_ptr() == &my_string);
  CHECK(*tagged_string.get_ptr() == copy);

  tagged_string.set(&my_string, tagged_string.get_next_tag());

  CHECK(tagged_string.get_tag() == 2);

  CHECK(tagged_string.get_ptr() == &my_string);
  CHECK(*tagged_string.get_ptr() == copy);

  auto second = tagged_ptr(&my_string, 2);
  CHECK((tagged_string == second) == true);

  tagged_string.set(&my_string, tagged_string.get_next_tag());
  CHECK(tagged_string != second);
}
