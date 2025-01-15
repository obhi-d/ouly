
#include <acl/utility/type_traits.hpp>
#include <compare>
#include <ranges>

namespace acl
{

template <auto M, typename C = typename acl::cfg::member<M>::class_type>
class projected_view : public std::ranges::view_interface<projected_view<M, C>>
{
public:
  static constexpr bool is_const = std::is_const_v<C>;

  struct iterator_wrapper
  {

  public:
    iterator_wrapper() noexcept = default;
    iterator_wrapper(C* d) noexcept : data_(d) {} // Concept requirements
    using iterator_category = std::random_access_iterator_tag;
    using value_type        = std::remove_reference_t<decltype(std::declval<C&>().*M)>;
    using difference_type   = std::ptrdiff_t;

    // Member functions (stubs)
    auto operator*() const& noexcept -> value_type&
    {
      return data_->*M;
    }

    auto operator->() const& noexcept -> value_type*
    {
      return &(data_->*M);
    };

    auto operator++() & noexcept -> iterator_wrapper&
    {
      ++data_;
      return *this;
    };

    auto operator++(int) & noexcept -> iterator_wrapper
    {
      return iterator_wrapper(data_++);
    };

    auto operator--() & noexcept -> iterator_wrapper&
    {
      --data_;
      return *this;
    }

    auto operator--(int) & noexcept -> iterator_wrapper
    {
      return iterator_wrapper(data_--);
    }

    auto operator+=(difference_type n) & noexcept -> iterator_wrapper&
    {
      data_ += n;
      return *this;
    }

    auto operator+(difference_type n) const& noexcept -> iterator_wrapper
    {
      return iterator_wrapper(data_ + n);
    }

    friend auto operator+(difference_type n, iterator_wrapper other) noexcept -> iterator_wrapper
    {
      return iterator_wrapper(other.data_ + n);
    }

    auto operator-=(difference_type n) & noexcept -> iterator_wrapper&
    {
      data_ -= n;
      return *this;
    }

    auto operator-(difference_type n) const& noexcept -> iterator_wrapper
    {
      return iterator_wrapper(data_ - n);
    }

    auto operator-(iterator_wrapper other) const& noexcept -> difference_type
    {
      return static_cast<difference_type>(data_ - other.data_);
    }

    auto operator<=>(iterator_wrapper const& other) const& noexcept = default;

    auto operator[](difference_type n) const& noexcept -> value_type&
    {
      return data_[n].*M;
    }

  private:
    C* data_ = nullptr;
  };

  auto begin() const noexcept
  {
    return iterator_wrapper(begin_);
  }

  auto end() const noexcept
  {
    return iterator_wrapper(end_);
  }

  projected_view(C* first, size_t count) noexcept : begin_(first), end_(first + count) {}
  projected_view() noexcept = default;

private:
  C* begin_ = nullptr;
  C* end_   = nullptr;
};

template <auto M>
using projected_cview = projected_view<M, typename acl::cfg::member<M>::class_type const>;

} // namespace acl
