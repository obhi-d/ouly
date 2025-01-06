
#include <acl/scheduler/event_types.hpp>
#include <acl/scheduler/scheduler.hpp>

namespace acl
{

void busywork_event::wait(worker_id worker, scheduler& sc)
{
  using namespace std::chrono_literals;
  while (!semaphore_.try_acquire())
  {
    sc.busy_work(worker);
  }
}
} // namespace acl