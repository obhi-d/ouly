
#pragma once

#include <atomic>
#include <vector>

namespace acl::detail
{

template <typename RevType>
class revision_table
{
protected:
  std::vector<RevType> revisions_;
};

template <>
class revision_table<void>
{};

template <typename S>
class counter
{
public:
  auto fetch_sub(S /*unused*/) -> S
  {
    return value_--;
  }

  auto fetch_add(S /*unused*/) -> S
  {
    return value_++;
  }

  auto load() const noexcept -> S
  {
    return value_;
  }

  void store(S v, std::memory_order /*unused*/)
  {
    value_ = v;
  }

  S value_;
};
} // namespace acl::detail