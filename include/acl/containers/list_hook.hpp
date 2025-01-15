
#pragma once

namespace acl
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
} // namespace acl