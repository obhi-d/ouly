
#pragma once

#include <compare>
#include <cstdint>
#include <functional>
#include <limits>

namespace acl::ecs
{
constexpr uint8_t min_revision_bit_count = 8;
template <typename Ty, typename SizeType = uint32_t, uint32_t RevisionBits = 0, SizeType NullValue = (SizeType)0>
class basic_entity
{
public:
	static_assert(RevisionBits < sizeof(SizeType) * min_revision_bit_count,
								"Revision bits must be less than the SizeType");

	using revision_type =
	 std::conditional_t<RevisionBits == 0, void,
											std::conditional_t<(RevisionBits > min_revision_bit_count), uint16_t, uint8_t>>;

	using size_type															= SizeType;
	static constexpr size_type null_v						= NullValue;
	static constexpr size_type nb_revision_bits = RevisionBits;
	static constexpr size_type nb_usable_bits		= ((sizeof(size_type) * 8) - nb_revision_bits);
	static constexpr size_type index_mask_v			= std::numeric_limits<size_type>::max() >> nb_revision_bits;
	static constexpr size_type revision_mask_v	= []() -> size_type
	{
		if constexpr (nb_revision_bits > 0)
		{
			return std::numeric_limits<size_type>::max() << nb_usable_bits;
		}
		return 0;
	}();
	static constexpr size_type version_inc_v = []() -> size_type
	{
		constexpr size_type one = 1;
		if constexpr (nb_revision_bits > 0)
		{
			return one << nb_usable_bits;
		}
		return 0;
	}();

	constexpr basic_entity() noexcept											 = default;
	basic_entity(basic_entity&&)													 = default;
	auto operator=(basic_entity&&) -> basic_entity&				 = default;
	constexpr basic_entity(basic_entity const& i) noexcept = default;
	constexpr explicit basic_entity(size_type i) noexcept : i_(i) {}
	constexpr explicit basic_entity(size_type i, size_type revision) noexcept
		requires(nb_revision_bits > 0)
			: i_(revision << nb_usable_bits | i)
	{}
	~basic_entity() noexcept = default;

	constexpr auto operator=(basic_entity const& i) noexcept -> basic_entity& = default;

	constexpr explicit operator size_type() const noexcept
	{
		return value();
	}

	constexpr explicit operator bool() const noexcept
	{
		return value() != null_v;
	}

	constexpr auto revision() const noexcept -> size_type
		requires(nb_revision_bits > 0)
	{
		return i_ >> nb_usable_bits;
	}

	constexpr auto revised() const noexcept -> basic_entity
		requires(nb_revision_bits > 0)
	{
		return basic_entity(i_ + version_inc_v);
	}

	constexpr auto revision() const noexcept -> size_type
		requires(nb_revision_bits <= 0)
	{
		return 0;
	}

	constexpr auto revised() const noexcept -> basic_entity
		requires(nb_revision_bits <= 0)
	{
		return basic_entity(i_);
	}

	constexpr auto get() const noexcept -> size_type
	{
		if constexpr (nb_revision_bits > 0)
		{
			return (i_ & index_mask_v);
		}
		else
		{
			return i_;
		}
	}

	constexpr auto value() const noexcept -> size_type
	{
		return i_;
	}

	auto operator<=>(basic_entity const&) const noexcept = default;

private:
	size_type i_ = null_v;
};

template <typename T = std::true_type>
using entity = basic_entity<T, uint32_t>;

template <typename T = std::true_type>
using rxentity = basic_entity<T, uint32_t, min_revision_bit_count>;

} // namespace acl::ecs

template <typename Ty, typename SizeType, uint32_t RevisionBits>
struct std::hash<acl::ecs::basic_entity<Ty, SizeType, RevisionBits>>
{
	auto operator()(acl::ecs::basic_entity<Ty, SizeType, RevisionBits> const& s) const noexcept -> std::size_t
	{
		return std::hash<SizeType>{}(s.value());
	}
};
