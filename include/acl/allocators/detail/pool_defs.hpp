
#include <cstdint>
#include <string>

namespace acl::detail
{
template <typename O>
concept HasAtomCount = O::atom_count_v > 0;

template <typename O>
concept HasAtomSize = O::atom_size_v > 0;

template <typename T>
struct atom_count
{
  static constexpr std::size_t value = 128;
};

template <typename T>
struct atom_size
{
  static constexpr std::size_t value = 32;
};

template <HasAtomCount T>
struct atom_count<T>
{
  static constexpr std::size_t value = T::atom_count_v;
};

template <HasAtomSize T>
struct atom_size<T>
{
  static constexpr std::size_t value = T::atom_size_v;
};

struct padding_stats
{
  std::uint32_t padding_atoms_ = 0;

  void pad_atoms(std::uint32_t v) noexcept
  {
    padding_atoms_ += v;
  }

  void unpad_atoms(std::uint32_t v) noexcept
  {
    padding_atoms_ -= v;
  }

  [[nodiscard]] auto padding_atoms_count() const noexcept -> std::uint32_t
  {
    return padding_atoms_;
  }

  [[nodiscard]] static auto print() -> std::string
  {
    return {};
  }
};
} // namespace acl::detail