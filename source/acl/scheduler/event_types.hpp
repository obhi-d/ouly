
#pragma once

#include <acl/utils/config.hpp>
#include "worker_context.hpp"
#include <semaphore>

namespace acl
{

class blocking_event
{

public:
  inline blocking_event(bool set) noexcept : semaphore((std::ptrdiff_t)set) {}
  inline blocking_event() noexcept : semaphore(0) {}

  inline void wait()
  {
    semaphore.acquire();
  }

  inline void notify()
  {
    semaphore.release();
  }

private:
  std::binary_semaphore semaphore;
};

class scheduler;
class busywork_event
{
public:
  inline busywork_event(bool set) noexcept : semaphore((std::ptrdiff_t)set) {}
  inline busywork_event() noexcept : semaphore(0) {}

  ACL_API void wait(worker_id worker, scheduler& s);

  inline void notify()
  {
    semaphore.release();
  }

private:
  std::binary_semaphore semaphore;
};

} // namespace acl
