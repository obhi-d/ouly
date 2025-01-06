
#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

namespace acl
{
template <typename TChar = char, TChar Delimiter = 0>
class word_list
{
public:
  using type = std::basic_string<TChar>;
  using view = std::basic_string_view<TChar>;
  class iterator
  {
  public:
    iterator(view string) noexcept : location_(0), word_(0), object_(string) {}
    iterator(view string, uint32_t loc, uint32_t word) noexcept : location_(loc), word_(word), object_(string) {}
    iterator(const iterator& it) noexcept : location_(it.location_), word_(it.word_), object_(it.object_) {}
    iterator(iterator&& it) noexcept : location_(it.location_), word_(it.word_), object_(it.object_) {}
    ~iterator() noexcept = default;

    auto operator=(const iterator& it) -> iterator&
    {
      assert(object_.data() == it.object_.data());
      word_     = it.word_;
      location_ = it.location_;
      return *this;
    }

    auto operator=(iterator&& it) noexcept -> iterator&
    {
      assert(object_.data() == object_.data());
      word_     = it.word_;
      location_ = it.location_;
      return *this;
    }

    auto operator++() -> iterator&
    {
      if (location_ < object_.size())
      {
        view v = get_nocheck();
        location_ += static_cast<uint32_t>(v.length() + 1);
        word_++;
      }
      return *this;
    }

    auto operator==(view what) const -> bool
    {
      return (location_ < object_.size() && what == get_nocheck());
    }

    friend auto operator==(const type& what, const iterator& it) -> bool
    {
      return it.get() == what;
    }

    friend auto operator!=(const type& what, const iterator& it) -> bool
    {
      return !(it == what);
    }

    friend auto operator==(const iterator& first, const iterator& second) -> bool
    {
      return first.location_ == second.location_ && first.word_ == second.word_;
    }
    friend auto operator!=(const iterator& first, const iterator& second) -> bool
    {
      return !(first == second);
    }

    template <typename StringType>
    auto has_next(StringType& view) -> bool
    {
      if (location_ < object_.size())
      {
        view = get_nocheck();
        location_ += static_cast<uint32_t>(view.length() + 1);
        word_++;
        return true;
      }
      return false;
    }

    auto operator()(view& view) -> bool
    {
      return has_next(view);
    }

    auto index() -> uint32_t
    {
      return word_;
    }

    auto operator*() const -> view
    {
      return get();
    }

    explicit operator bool() const
    {
      return location_ < object_.size();
    }

    [[nodiscard]] auto get() const -> view
    {
      return (location_ < object_.size()) ? get_nocheck() : view();
    }

    friend auto operator<<(std::ostream& os, iterator const& it) -> std::ostream&
    {
      os << it.get();
      return os;
    }

  private:
    [[nodiscard]] auto get_nocheck() const -> view
    {
      size_t pos = object_.find_first_of(Delimiter, location_);
      return (pos == view::npos) ? view(object_.data() + location_) : view(object_.data() + location_, pos - location_);
    }

    view     object_{};
    uint32_t location_;
    uint32_t word_;
  };

  static void push_back(type& this_param, const type& value)
  {
    if (this_param.length() > 0)
    {
      this_param += Delimiter;
    }
    this_param += value;
  }
  static void push_back(type& this_param, view value)
  {
    if (this_param.length() > 0)
    {
      this_param += Delimiter;
    }
    this_param += value;
  }
  static void push_back(type& this_param, const char* value)
  {
    if (this_param.length() > 0)
    {
      this_param += Delimiter;
    }
    this_param += value;
  }
  static auto length(view this_param) -> uint32_t
  {
    return !this_param.empty() ? static_cast<uint32_t>(std::count(this_param.begin(), this_param.end(), Delimiter) + 1)
                               : 0U;
  }
  static auto iter(view of) -> iterator
  {
    return iterator(of);
  }
  /**
   * @brief Find the index of what in this_param
   * @param this_param The word list
   * @param what The string view whose index needs to be found
   * @return returns (uint32_t)-1 if not found
   */
  static auto index_of(view this_param, view what) -> uint32_t
  {
    iterator it(this_param);

    view view;
    while (it.has_next(view))
    {
      if (view == what)
      {
        return it.index() - 1;
      }
    }
    return std::numeric_limits<uint32_t>::max();
  }
  static auto find(view this_param, view what) -> iterator
  {
    size_t it = this_param.find(what);
    return it == type::npos ? iterator(this_param, (uint32_t)this_param.length(), 0)
                            : iterator(this_param, (uint32_t)it,
                                       (uint32_t)std::count(this_param.begin(), this_param.begin() + it, Delimiter));
  }
};

} // namespace acl
