
#include "allocator.hpp"
#include "default_allocator.hpp"
#include "detail/utils.hpp"
#include <optional>

namespace acl
{

template <typename Ty, typename Options = acl::default_options<Ty>>
class basic_queue : public detail::custom_allocator_t<Options>
{
public:
  using value_type     = Ty;
  using size_type      = detail::choose_size_t<uint32_t, Options>;
  using allocator_type = detail::custom_allocator_t<Options>;

private:
  static constexpr auto pool_div  = detail::log2(detail::pool_size_v<Options>);
  static constexpr auto pool_size = static_cast<size_type>(1) << pool_div;
  static constexpr auto pool_mod  = pool_size - 1;
  using storage                   = detail::aligned_storage<sizeof(value_type), alignof(value_type)>;
  static constexpr bool has_pod   = detail::HasTrivialAttrib<Options>;

  struct deque_block
  {
    std::array<storage, pool_size> data;
    deque_block*                   next = nullptr;
  };

public:
  basic_queue() noexcept
  {
    add_tail();
  }

  ~basic_queue()
  {
    clear();
    free_chain(free_);
  }

  template <typename... Args>
  inline auto& emplace_back(Args&&... args) noexcept
  {
    if (back_ == pool_size)
    {
      add_tail();
      back_ = 0;
    }
    auto ptr = (Ty*)(&tail_->data[back_++]);
    std::construct_at(ptr, std::forward<Args>(args)...);
    return *ptr;
  }

  inline Ty pop_front()
  {
    if (empty())
      throw std::runtime_error("Deque is empty");

    return pop_front_unsafe();
  }

  inline Ty pop_front_unsafe() noexcept
  {
    ACL_ASSERT(!empty());

    if (front_ == pool_size)
    {
      remove_head();
      front_ = 0;
    }

    Ty ret = std::move(*(Ty*)(&head_->data[front_]));
    std::destroy_at((Ty*)(&head_->data[front_++]));
    return ret;
  }

  inline bool empty() const noexcept
  {
    return (head_ == tail_ && front_ == back_);
  }

  void clear() noexcept
  {
    auto block = head_;
    auto start = front_;
    while (block)
    {
      auto end = block == tail_ ? back_ : pool_size;
      for (; start < end; ++start)
      {
        std::destroy_at((Ty*)(&block->data[start]));
      }
      start = 0;
      block = block->next;
    }
    if (head_)
      head_->next = free_;
    free_ = head_;
    head_ = tail_ = 0;
    front_ = back_ = 0;
  }

private:
  void add_tail()
  {
    deque_block* db = free_;
    if (free_)
      free_ = free_->next;
    else
    {
      db       = acl::allocate<deque_block>(static_cast<allocator_type&>(*this), sizeof(deque_block), alignarg<Ty>);
      db->next = nullptr;
    }

    if (tail_)
      tail_->next = db;
    else
      head_ = db;
    tail_ = db;
  }

  void remove_head()
  {
    auto h  = head_;
    head_   = head_->next;
    h->next = free_;
    free_   = h;
  }

  void free_chain(deque_block* start)
  {
    while (start)
    {
      auto next = start->next;
      acl::deallocate(static_cast<allocator_type&>(*this), start, sizeof(deque_block), alignarg<Ty>);
      start = next;
    }
  }

  deque_block* head_  = nullptr;
  deque_block* tail_  = nullptr;
  deque_block* free_  = nullptr;
  size_type    front_ = 0;
  size_type    back_  = 0;
};
} // namespace acl
