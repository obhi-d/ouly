
#include "ouly/scheduler/event_types.hpp"
#include "ouly/scheduler/scheduler.hpp"

namespace ouly
{

void busywork_event::wait(worker_id worker, scheduler& sc)
{
  using namespace std::chrono_literals;
  while (!semaphore_.try_acquire())
  {
    sc.busy_work(worker);
  }
}
} // namespace ouly