#define CATCH_CONFIG_MAIN
#include <acl/default_allocator.hpp>
#include <acl/export.hxx>
#include <acl/packed_table.hpp>
#include <acl/sparse_table.hpp>
#include <catch2/catch.hpp>

TEST_CASE("Validate general_allocator", "[general_allocator]")
{

  using namespace acl;
  using allocator_t = default_allocator<std::uint32_t, 0, true, false>;

  allocator_t::address data = allocator_t::allocate(256, 128);
  CHECK((reinterpret_cast<std::uintptr_t>(data) & 127) == 0);
  allocator_t::deallocate(data, 256, 128);
}