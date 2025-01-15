#include <cstdint>

namespace acl
{
struct default_task_traits
{
  /**
   * Relevant for ranged executers, this value determines the number of batches dispatched per worker on average. Higher
   * value means the individual task batches are smaller.
   */
  static constexpr uint32_t batches_per_worker = 4;
  /**
   * This value is used as the minimum task count that will fire the parallel executer, if the task count is less than
   * this value, a for loop is executed instead.
   */
  static constexpr uint32_t parallel_execution_threshold = 16;
  /**
   * This value, if set to non-zero, would override the `batches_per_worker` value and instead be used as the batch size
   * for the tasks.
   */
  static constexpr uint32_t fixed_batch_size = 0;
};
} // namespace acl