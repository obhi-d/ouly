
#include <acl/default_allocator.hpp>
#include <acl/export.hxx>
#include <acl/intrusive_ptr.hpp>
#include <acl/packed_table.hpp>
#include <acl/sparse_table.hpp>
#include <acl/std_allocator_wrapper.hpp>
#include <acl/tagged_ptr.hpp>
#include <catch2/catch_all.hpp>

struct myClass
{
  int value = 0;
};

int intrusive_count_add(myClass* m)
{
  return m->value++;
}
int intrusive_count_sub(myClass* m)
{
  return m->value--;
}
int intrusive_count_get(myClass* m)
{
  return m->value;
}

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

TEST_CASE("Validate general_allocator", "[intrusive_ptr]")
{
  acl::intrusive_ptr<myClass> ptr;
  CHECK(ptr == nullptr);
  myClass instance;
  ptr = &instance;
  CHECK(ptr != nullptr);
  CHECK(ptr.use_count() == 1);
  CHECK(ptr->value == 1);
  ptr.reset();
  CHECK(instance.value == 0);
  ptr = &instance;
  CHECK(ptr.use_count() == 1);
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
