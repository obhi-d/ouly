// SPDX-License-Identifier: MIT

#pragma once

namespace ouly
{
struct slist_hook
{
  void* pnext_ = nullptr;
};

struct list_hook
{
  void* pnext_ = nullptr;
  void* pprev_ = nullptr;
};
} // namespace ouly