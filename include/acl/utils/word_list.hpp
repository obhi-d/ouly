
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
    iterator(view string) noexcept : location(0), word(0), object(string) {}
    iterator(view string, uint32_t loc, uint32_t word) noexcept : location(loc), word(word), object(string) {}
    iterator(const iterator& it) noexcept : location(it.location), word(it.word), object(it.object) {}
    iterator(iterator&& it) noexcept : location(it.location), word(it.word), object(it.object) {}

    iterator& operator=(const iterator& it)
    {
      assert(object.data() == it.object.data());
      word     = it.word;
      location = it.location;
      return *this;
    }

    iterator& operator=(iterator&& it) noexcept
    {
      assert(object.data() == object.data());
      word     = it.word;
      location = it.location;
      return *this;
    }

    iterator& operator++()
    {
      if (location < object.size())
      {
        view v = get_nocheck();
        location += static_cast<uint32_t>(v.length() + 1);
        word++;
      }
      return *this;
    }

    bool operator==(view what) const
    {
      return (location < object.size() && what == get_nocheck());
    }

    friend bool operator==(const type& what, const iterator& it)
    {
      return it.get() == what;
    }

    friend bool operator!=(const type& what, const iterator& it)
    {
      return !(it == what);
    }

    friend bool operator==(const iterator& first, const iterator& second)
    {
      return first.location == second.location && first.word == second.word;
    }
    friend bool operator!=(const iterator& first, const iterator& second)
    {
      return !(first == second);
    }

    template <typename StringType>
    bool has_next(StringType& view)
    {
      if (location < object.size())
      {
        view = get_nocheck();
        location += static_cast<uint32_t>(view.length() + 1);
        word++;
        return true;
      }
      return false;
    }

    bool operator()(view& view)
    {
      return has_next(view);
    }

    uint32_t index()
    {
      return word;
    }

    inline view operator*() const
    {
      return get();
    }

    inline explicit operator bool() const
    {
      return location < object.size();
    }

    [[nodiscard]] view get() const
    {
      return (location < object.size()) ? get_nocheck() : view();
    }

    friend std::ostream& operator<<(std::ostream& os, iterator const& it)
    {
      os << it.get();
      return os;
    }

  private:
    view get_nocheck() const
    {
      size_t pos = object.find_first_of(Delimiter, location);
      return (pos == view::npos) ? view(object.data() + location) : view(object.data() + location, pos - location);
    }

    view     object;
    uint32_t location;
    uint32_t word;
  };

  static void push_back(type& this_param, const type& value)
  {
    if (this_param.length() > 0)
      this_param += Delimiter;
    this_param += value;
  }
  static void push_back(type& this_param, view value)
  {
    if (this_param.length() > 0)
      this_param += Delimiter;
    this_param += value;
  }
  static void push_back(type& this_param, const char* value)
  {
    if (this_param.length() > 0)
      this_param += Delimiter;
    this_param += value;
  }
  static uint32_t length(view this_param)
  {
    return this_param.length() > 0
             ? static_cast<uint32_t>(std::count(this_param.begin(), this_param.end(), Delimiter) + 1)
             : 0u;
  }
  static iterator iter(view of)
  {
    return iterator(of);
  }
  /**
   * @brief Find the index of what in this_param
   * @param this_param The word list
   * @param what The string view whose index needs to be found
   * @return returns (uint32_t)-1 if not found
   */
  static uint32_t index_of(view this_param, view what)
  {
    iterator it(this_param);

    view view;
    while (it.has_next(view))
    {
      if (view == what)
        return it.index() - 1;
    }
    return std::numeric_limits<uint32_t>::max();
  }
  static iterator find(view this_param, view what)
  {
    size_t it = this_param.find(what);
    return it == type::npos ? iterator(this_param, (uint32_t)this_param.length(), 0)
                            : iterator(this_param, (uint32_t)it,
                                       (uint32_t)std::count(this_param.begin(), this_param.begin() + it, Delimiter));
  }
};

} // namespace acl
