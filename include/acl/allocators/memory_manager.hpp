
#include <concepts>
#include <cstdint>

namespace acl
{
template <typename T>
concept MemoryManager = requires(T m) {
  /**
   * Drop Arena
   */
  { m.drop_arena(std::uint32_t()) } -> std::same_as<bool>;
  /**
   * Add an arena
   */
  { m.add_arena(std::uint32_t(), std::size_t()) } -> std::same_as<std::uint32_t>;
  // Remoe an arena
  m.remove_arena(std::uint32_t());
};

template <typename T, typename A>
concept HasDefragmentSupport = requires(T a, A& allocator, std::uint32_t src_arena, std::uint32_t dst_arena,
                                        std::uint32_t alloc_info, std::size_t from, std::size_t to, std::size_t size) {
  // Begin defragment
  a.begin_defragment(allocator);
  // End defragmentation
  a.end_defragment(allocator);
  // Rebind an allocation to another value
  a.rebind_alloc(alloc_info, src_arena, alloc_info, typename A::size_type());
  // Move memory from source arena to another
  a.move_memory(src_arena, dst_arena, from, to, size);
};

} // namespace acl