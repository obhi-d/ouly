// SPDX-License-Identifier: MIT

#pragma once

namespace ouly
{
struct blackboard_offset
{
  using dtor = void (*)(void*);

  void* data_       = nullptr;
  dtor  destructor_ = nullptr;
};
} // namespace ouly