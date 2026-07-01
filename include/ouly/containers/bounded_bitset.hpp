// SPDX-License-Identifier: MIT
#pragma once
#include "ouly/containers/small_vector.hpp"
#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <type_traits>
#include <utility>
#include <vector>

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)
#include <immintrin.h>
#endif

namespace ouly
{
enum class bounded_bitset_isa : uint8_t
{
  scalar,
  sse2,
  avx2
};

/**
 * @brief Dynamic bitset with a compact base offset for sparse, bounded index ranges.
 *
 * bounded_bitset stores bits in a contiguous vector of machine words. When OffsetLimit is non-zero, the first inserted
 * word becomes the base offset, so setting a high bit does not allocate storage for all lower words. If the live
 * storage grows past the offset limit and later needs to shift lower, it falls back to a zero base, matching
 * index_map's bounded-offset behavior.
 */
constexpr uint32_t bounded_bitset_default_offset_limit = 16;
template <typename T = uint32_t, T OffsetLimit = bounded_bitset_default_offset_limit, typename Word = uint64_t,
          bounded_bitset_isa Isa = bounded_bitset_isa::scalar>
class bounded_bitset
{
  static_assert(std::is_integral_v<T> && std::is_unsigned_v<T>, "bounded_bitset index type must be unsigned");
  static_assert(std::is_integral_v<Word> && std::is_unsigned_v<Word>, "bounded_bitset word type must be unsigned");

  static constexpr std::size_t min_offset = static_cast<std::size_t>(std::max<T>(OffsetLimit, 1));

  struct with_offset_limit
  {
    ouly::small_vector<Word, min_offset> words_;
    T                                    base_word_ = null;
  };

  struct without_offset_limit
  {
    std::vector<Word> words_;
  };

  using word_list = std::conditional_t<OffsetLimit == 0, without_offset_limit, with_offset_limit>;

public:
  using size_type       = T;
  using word_type       = Word;
  using value_type      = word_type;
  using reference       = word_type&;
  using const_reference = word_type const&;

  static constexpr T    limit         = OffsetLimit;
  static constexpr T    null          = std::numeric_limits<size_type>::max();
  static constexpr T    bits_per_word = static_cast<T>(std::numeric_limits<word_type>::digits);
  static constexpr auto isa           = Isa;
  static constexpr Word one           = Word{1};

  void set(T idx, bool value = true)
  {
    if (!value)
    {
      reset(idx);
      return;
    }

    auto& word = word_at(idx);
    word |= mask(idx);
  }

  void reset(T idx)
  {
    auto* word = word_if(idx);
    if (word == nullptr)
    {
      return;
    }

    *word &= ~mask(idx);
    trim();
  }

  [[nodiscard]] auto test(T idx) const noexcept -> bool
  {
    auto const* word = word_if(idx);
    return word != nullptr && ((*word & mask(idx)) != 0);
  }

  [[nodiscard]] auto operator[](T idx) const noexcept -> bool
  {
    return test(idx);
  }

  [[nodiscard]] auto contains(T idx) const noexcept -> bool
  {
    auto const local = local_word(idx);
    return local != null && static_cast<std::size_t>(local) < data_.words_.size();
  }

  void clear() noexcept
  {
    if constexpr (OffsetLimit > 0)
    {
      data_.base_word_ = null;
    }
    data_.words_.clear();
  }

  void trim()
  {
    while (!data_.words_.empty() && data_.words_.back() == 0)
    {
      data_.words_.pop_back();
    }

    if (data_.words_.empty())
    {
      if constexpr (OffsetLimit > 0)
      {
        data_.base_word_ = null;
      }
      return;
    }

    if constexpr (OffsetLimit > 0)
    {
      std::size_t leading = 0;
      while (leading < data_.words_.size() && data_.words_[leading] == 0)
      {
        ++leading;
      }

      if (leading != 0)
      {
        data_.words_.erase(data_.words_.begin(), std::next(data_.words_.begin(), static_cast<std::ptrdiff_t>(leading)));
        data_.base_word_ += static_cast<T>(leading);
      }
    }
  }

  /** @brief This value must be subtracted from the queried bit index before addressing the local bit range. */
  [[nodiscard]] auto base_offset() const noexcept -> T
  {
    if constexpr (OffsetLimit > 0)
    {
      return data_.words_.empty() ? null : static_cast<T>(data_.base_word_ * bits_per_word);
    }
    else
    {
      return 0;
    }
  }

  [[nodiscard]] auto empty() const noexcept -> bool
  {
    return data_.words_.empty();
  }

  [[nodiscard]] auto size() const noexcept -> T
  {
    return static_cast<T>(data_.words_.size()) * bits_per_word;
  }

  [[nodiscard]] auto storage_size() const noexcept -> std::size_t
  {
    return data_.words_.size();
  }

  [[nodiscard]] auto data() noexcept -> word_type*
  {
    return data_.words_.data();
  }

  [[nodiscard]] auto data() const noexcept -> word_type const*
  {
    return data_.words_.data();
  }

  template <typename Fn>
  void for_each(Fn&& fn) const
  {
    for_each_impl<Isa>(std::forward<Fn>(fn));
  }

  void swap(bounded_bitset& other) noexcept
  {
    using std::swap;
    swap(data_, other.data_);
  }

  friend void swap(bounded_bitset& lhs, bounded_bitset& rhs) noexcept
  {
    lhs.swap(rhs);
  }

  auto begin() noexcept
  {
    return data_.words_.begin();
  }

  auto end() noexcept
  {
    return data_.words_.end();
  }

  auto begin() const noexcept
  {
    return data_.words_.begin();
  }

  auto end() const noexcept
  {
    return data_.words_.end();
  }

  auto rbegin() noexcept
  {
    return data_.words_.rbegin();
  }

  auto rend() noexcept
  {
    return data_.words_.rend();
  }

  auto rbegin() const noexcept
  {
    return data_.words_.rbegin();
  }

  auto rend() const noexcept
  {
    return data_.words_.rend();
  }

private:
  [[nodiscard]] static constexpr auto word_index(T idx) noexcept -> T
  {
    return static_cast<T>(idx / bits_per_word);
  }

  [[nodiscard]] static constexpr auto mask(T idx) noexcept -> word_type
  {
    return static_cast<word_type>(one << static_cast<unsigned>(idx % bits_per_word));
  }

  [[nodiscard]] auto storage_base_word() const noexcept -> T
  {
    if constexpr (OffsetLimit > 0)
    {
      return data_.words_.empty() ? 0 : data_.base_word_;
    }
    else
    {
      return 0;
    }
  }

  template <bounded_bitset_isa SelectedIsa, typename Fn>
  void for_each_impl(Fn&& fn) const
  {
    if constexpr (SelectedIsa == bounded_bitset_isa::avx2)
    {
      for_each_avx2(std::forward<Fn>(fn));
    }
    else if constexpr (SelectedIsa == bounded_bitset_isa::sse2)
    {
      for_each_sse2(std::forward<Fn>(fn));
    }
    else
    {
      for_each_scalar(std::forward<Fn>(fn));
    }
  }

  template <typename Fn>
  void for_each_scalar(Fn&& fn) const
  {
    for_each_word_range(0, data_.words_.size(), std::forward<Fn>(fn));
  }

  template <typename Fn>
  void for_each_word_range(std::size_t first, std::size_t last, Fn&& fn) const
  {
    auto const base_word = storage_base_word();
    for (auto word_offset = first; word_offset < last; ++word_offset)
    {
      visit_word(data_.words_[word_offset], static_cast<T>(base_word + static_cast<T>(word_offset)),
                 std::forward<Fn>(fn));
    }
  }

  template <typename Fn>
  void visit_word(word_type word, T word_index_value, Fn&& fn) const
  {
    while (word != 0)
    {
      auto const bit = static_cast<T>(std::countr_zero(word));
      std::invoke(std::forward<Fn>(fn), static_cast<T>((word_index_value * bits_per_word) + bit));
      word &= static_cast<word_type>(word - 1);
    }
  }

  [[nodiscard]] static constexpr auto simd_word_type_supported() noexcept -> bool
  {
    // NOLINTNEXTLINE
    return sizeof(word_type) == 4 || sizeof(word_type) == 8;
  }

  template <typename Fn>
  void for_each_sse2(Fn&& fn) const
  {
#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
    if constexpr (simd_word_type_supported())
    {
      auto const* words          = data_.words_.data();
      auto const  count          = data_.words_.size();
      auto const  base_word      = storage_base_word();
      auto const  words_per_load = 16U / sizeof(word_type);
      auto        word_offset    = std::size_t{0};

      for (; word_offset + words_per_load <= count; word_offset += words_per_load)
      {
        constexpr uint16_t move_mask = 0xFFFF;
        // NOLINTNEXTLINE
        auto const loaded = _mm_loadu_si128(reinterpret_cast<__m128i const*>(words + word_offset));
        auto const zero   = _mm_setzero_si128();
        auto const eq     = _mm_cmpeq_epi8(loaded, zero);
        if (_mm_movemask_epi8(eq) == move_mask)
        {
          continue;
        }

        for (std::size_t lane = 0; lane < words_per_load; ++lane)
        {
          visit_word(words[word_offset + lane], static_cast<T>(base_word + static_cast<T>(word_offset + lane)),
                     std::forward<Fn>(fn));
        }
      }

      for_each_word_range(word_offset, count, std::forward<Fn>(fn));
    }
    else
#endif
    {
      for_each_scalar(std::forward<Fn>(fn));
    }
  }

  template <typename Fn>
  void for_each_avx2(Fn&& fn) const
  {
#if defined(__AVX2__) || defined(_M_AVX2)
    if constexpr (simd_word_type_supported())
    {
      auto const* words          = data_.words_.data();
      auto const  count          = data_.words_.size();
      auto const  base_word      = storage_base_word();
      auto const  words_per_load = 32U / sizeof(word_type);
      auto        word_offset    = std::size_t{0};

      for (; word_offset + words_per_load <= count; word_offset += words_per_load)
      {
        auto const loaded = _mm256_loadu_si256(reinterpret_cast<__m256i const*>(words + word_offset));
        auto const zero   = _mm256_setzero_si256();
        auto const eq     = _mm256_cmpeq_epi8(loaded, zero);
        if (_mm256_movemask_epi8(eq) == -1)
        {
          continue;
        }

        for (std::size_t lane = 0; lane < words_per_load; ++lane)
        {
          visit_word(words[word_offset + lane], static_cast<T>(base_word + static_cast<T>(word_offset + lane)),
                     std::forward<Fn>(fn));
        }
      }

      for_each_word_range(word_offset, count, std::forward<Fn>(fn));
    }
    else
#endif
    {
      for_each_sse2(std::forward<Fn>(fn));
    }
  }

  [[nodiscard]] auto local_word(T idx) const noexcept -> T
  {
    auto const word = word_index(idx);
    if constexpr (OffsetLimit > 0)
    {
      if (data_.words_.empty() || word < data_.base_word_)
      {
        return null;
      }
      return static_cast<T>(word - data_.base_word_);
    }
    else
    {
      return word;
    }
  }

  auto word_if(T idx) noexcept -> word_type*
  {
    auto const local = local_word(idx);
    if (local == null || static_cast<std::size_t>(local) >= data_.words_.size())
    {
      return nullptr;
    }
    return &data_.words_[local];
  }

  [[nodiscard]] auto word_if(T idx) const noexcept -> word_type const*
  {
    auto const local = local_word(idx);
    if (local == null || static_cast<std::size_t>(local) >= data_.words_.size())
    {
      return nullptr;
    }
    return &data_.words_[local];
  }

  auto word_at(T idx) -> word_type&
  {
    auto const word = word_index(idx);
    if constexpr (OffsetLimit > 0)
    {
      if (data_.base_word_ > word)
      {
        if (data_.words_.empty())
        {
          data_.base_word_ = word;
        }
        else
        {
          T to_base_word = 0;
          if (data_.words_.size() < limit)
          {
            to_base_word = word;
          }
          data_.base_word_ = shift(to_base_word);
        }
      }

      auto const local = static_cast<T>(word - data_.base_word_);
      if (static_cast<std::size_t>(local) >= data_.words_.size())
      {
        data_.words_.resize(static_cast<std::size_t>(local) + 1U, 0);
      }
      return data_.words_[local];
    }
    else
    {
      if (static_cast<std::size_t>(word) >= data_.words_.size())
      {
        data_.words_.resize(static_cast<std::size_t>(word) + 1U, 0);
      }
      return data_.words_[word];
    }
  }

  auto shift(T base_word) -> T
  {
    if constexpr (OffsetLimit > 0)
    {
      auto const amount   = static_cast<std::size_t>(data_.base_word_ - base_word);
      auto const cur_size = data_.words_.size();
      data_.words_.resize(cur_size + amount, 0);
      if (cur_size != 0)
      {
        auto begin = data_.words_.begin();
        std::move_backward(begin, std::next(begin, static_cast<std::ptrdiff_t>(cur_size)),
                           std::next(begin, static_cast<std::ptrdiff_t>(cur_size + amount)));
        std::fill_n(begin, amount, 0);
      }
    }
    return base_word;
  }

  word_list data_;
};

template <typename T = uint32_t, T OffsetLimit = bounded_bitset_default_offset_limit, typename Word = uint64_t>
using bounded_bitset_scalar = bounded_bitset<T, OffsetLimit, Word, bounded_bitset_isa::scalar>;

template <typename T = uint32_t, T OffsetLimit = bounded_bitset_default_offset_limit, typename Word = uint64_t>
using bounded_bitset_sse2 = bounded_bitset<T, OffsetLimit, Word, bounded_bitset_isa::sse2>;

template <typename T = uint32_t, T OffsetLimit = bounded_bitset_default_offset_limit, typename Word = uint64_t>
using bounded_bitset_avx2 = bounded_bitset<T, OffsetLimit, Word, bounded_bitset_isa::avx2>;
} // namespace ouly
