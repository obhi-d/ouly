
#include "event_types.hpp"
#include "scheduler.hpp"

namespace acl
{

void busywork_event::wait(worker_id worker, scheduler& sc)
{
  using namespace std::chrono_literals;
  while (!semaphore.try_acquire())
  {
    sc.busy_work(worker);
  }
}
} // namespace acl