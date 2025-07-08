

#include <cstdint>
#if __has_include(<tbb/tbb.h>)
#include <tbb/task.h>
#include <tbb/task_arena.h>
#include <tbb/task_group.h>
#include <tbb/tbb.h>

namespace ouly::detail
{
class tbb_scheduler
{
  enum class workgroup_priority : uint8_t
  {
    low,
    normal,
    high
  };

  class workgroup_index
  {
  public:
    workgroup_index() noexcept                                     = default;
    workgroup_index(const workgroup_index&)                        = default;
    auto operator=(const workgroup_index&) -> workgroup_index&     = default;
    workgroup_index(workgroup_index&&) noexcept                    = default;
    auto operator=(workgroup_index&&) noexcept -> workgroup_index& = default;
    ~workgroup_index()                                             = default;

    [[nodiscard]] auto get() const -> uint32_t
    {
      return value_;
    }

    void set(uint32_t value)
    {
      value_ = value;
    }

  private:
    uint32_t value_ = 0;
  };

  class workgroup
  {
  public:
    workgroup(int32_t size, workgroup_priority priority) noexcept
        : arena_(tbb::task_arena(size), to_tbb_priority(priority))
    {}

  private:
    static constexpr auto to_tbb_priority(workgroup_priority priority) noexcept -> tbb::priority
    {
      switch (priority)
      {
      case workgroup_priority::low:
        return tbb::priority::low;
      case workgroup_priority::normal:
        return tbb::priority::normal;
      case workgroup_priority::high:
        return tbb::priority::high;
      default:
        return tbb::priority::normal; // Fallback to normal if unknown
      }
    }

    tbb::task_arena arena_;
  };
};
} // namespace ouly::detail
#endif