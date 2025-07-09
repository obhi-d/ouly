

#include "oneapi/tbb/task_arena.h"
#include <limits>
#if __has_include(<tbb/tbb.h>)
#include <cstdint>
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

  using workgroup_index = int32_t;

  class workgroup
  {
  public:
    workgroup(int32_t size, workgroup_priority priority) noexcept : arena_(size, 1, to_tbb_priority(priority)) {}

    struct this_workgroup
    {
      static auto indext() -> workgroup_index
      {
        return tbb::this_task_arena::current_thread_index();
      }
    };

    void initialize(int32_t size, workgroup_priority priority) noexcept
    {
      arena_.initialize(size, 1, to_tbb_priority(priority));
    }

  private:
    static constexpr auto to_tbb_priority(workgroup_priority priority) noexcept -> tbb::task_arena::priority
    {
      switch (priority)
      {
      case workgroup_priority::low:
        return tbb::task_arena::priority::low;
      case workgroup_priority::normal:
        return tbb::task_arena::priority::normal;
      case workgroup_priority::high:
        return tbb::task_arena::priority::high;
      default:
        return tbb::task_arena::priority::normal; // Fallback to normal if unknown
      }
    }

    tbb::task_arena arena_;
  };

public:
  void reserve_workgroups(uint32_t num_workgroups)
  {
    workgroups_.reserve(num_workgroups);
  }

  auto create_workgroup(int32_t size, workgroup_priority priority,
                        uint32_t group_index = std::numeric_limits<uint32_t>::max()) -> int
  {
    if (group_index == std::numeric_limits<uint32_t>::max())
    {
      workgroups_.emplace_back(size, priority);
      return static_cast<int>(workgroups_.size()) - 1; // Return the index of the newly created workgroup
    }

    if (group_index > static_cast<uint32_t>(workgroups_.size()))
    {
      throw std::out_of_range("Workgroup index is out of range");
    }
    // Resize existing workgroup if index is valid
    workgroups_[group_index].initialize(size, priority);
    return static_cast<int>(group_index); // Return the index of the existing workgroup
  }

private:
  std::vector<workgroup> workgroups_;
};
} // namespace ouly::detail
#endif