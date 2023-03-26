#define CATCH_CONFIG_MAIN

#include <acl/std_allocator_wrapper.hpp>

#include <acl/default_allocator.hpp>
#include <acl/export.hxx>
#include <acl/packed_table.hpp>
#include <acl/sparse_table.hpp>
#include <acl/tagged_ptr.hpp>
#include <catch2/catch.hpp>

TEST_CASE("Validate general_allocator", "[general_allocator]")
{

  using namespace acl;
  using allocator_t   = default_allocator<std::uint32_t, 0, true, false>;
  using std_allocator = std_allocator_wrapper<int, allocator_t>;
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
  std::string copy     = my_string;

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
