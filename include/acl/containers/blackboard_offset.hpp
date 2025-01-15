
#pragma once

namespace acl
{
struct blackboard_offset
{
  using dtor = void (*)(void*);

  void* data_       = nullptr;
  dtor  destructor_ = nullptr;
};
} // namespace acl