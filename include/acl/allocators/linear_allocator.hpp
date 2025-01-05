#pragma once

#include "default_allocator.hpp"
#include <acl/utils/common.hpp>

namespace acl
{

struct linear_allocator_tag
{};

template <typename Options = acl::options<>>
class linear_allocator : detail::statistics<linear_allocator_tag, Options>
{
public:
	using tag									 = linear_allocator_tag;
	using statistics					 = detail::statistics<linear_allocator_tag, Options>;
	using underlying_allocator = detail::underlying_allocator_t<Options>;
	using size_type						 = typename underlying_allocator::size_type;
	using address							 = typename underlying_allocator::address;

	template <typename... Args>
	linear_allocator(size_type i_arena_size, [[maybe_unused]] Args... args) noexcept
			: k_arena_size_(i_arena_size), left_over_(i_arena_size)
	{
		statistics::report_new_arena();
		buffer_ = underlying_allocator::allocate(k_arena_size_, {});
	}

	linear_allocator(linear_allocator const&) = delete;

	linear_allocator(linear_allocator&& other) noexcept
			: buffer_(other.buffer_), left_over_(other.left_over_), k_arena_size_(other.k_arena_size_)
	{
		other.buffer_		 = nullptr;
		other.left_over_ = 0;
	}

	~linear_allocator() noexcept
	{
		underlying_allocator::deallocate(buffer_, k_arena_size_);
	}

	auto operator=(linear_allocator const&) -> linear_allocator& = delete;

	auto operator=(linear_allocator&& other) noexcept -> linear_allocator&
	{
		assert(k_arena_size_ == other.k_arena_size_);
		buffer_					 = other.buffer_;
		left_over_			 = other.left_over_;
		other.buffer_		 = nullptr;
		other.left_over_ = 0;
		return *this;
	}

	constexpr static auto null() -> address
	{
		return underlying_allocator::null();
	}

	template <typename Alignment = alignment<>>
	[[nodiscard]] auto allocate(size_type i_size, Alignment i_alignment = {}) -> address
	{
		auto measure = statistics::report_allocate(i_size);
		// assert
		auto const fixup = i_alignment - 1;
		// make sure you allocate enough space
		// but keep alignment distance so that next allocations
		// do not suffer from lost alignment
		if (i_alignment)
		{
			i_size += i_alignment;
		}

		assert(left_over_ >= i_size);
		size_type offset = k_arena_size_ - left_over_;
		left_over_ -= i_size;
		if (i_alignment)
		{
			// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
			auto pointer = reinterpret_cast<std::uintptr_t>(buffer_) + static_cast<std::uintptr_t>(offset);
			if ((pointer & fixup) == 0)
			{
				left_over_ += i_alignment;
				// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
				return reinterpret_cast<address>(reinterpret_cast<std::uint8_t*>(buffer_) + offset);
			}

			auto ret = (pointer + static_cast<std::uintptr_t>(fixup)) & ~static_cast<std::uintptr_t>(fixup);
			// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
			return reinterpret_cast<address>(ret);
		}

		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
		return reinterpret_cast<address>(reinterpret_cast<std::uint8_t*>(buffer_) + offset);
	}

	template <typename Alignment = alignment<>>
	[[nodiscard]] auto zero_allocate(size_type i_size, Alignment i_alignment = {}) -> address
	{
		auto z = allocate(i_size, i_alignment);
		std::memset(z, 0, i_size);
		return z;
	}

	template <typename Alignment = alignment<>>
	void deallocate(address i_data, size_type i_size, Alignment i_alignment = {})
	{
		auto measure = statistics::report_deallocate(i_size);

		// merge back?
		size_type new_left_over = left_over_ + i_size;
		size_type offset				= (k_arena_size_ - new_left_over);
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
		if (reinterpret_cast<std::uint8_t*>(buffer_) + offset == reinterpret_cast<std::uint8_t*>(i_data))
		{
			left_over_ = new_left_over;
		}
		else
		{
			if constexpr (i_alignment)
			{
				i_size += (std::size_t)i_alignment;

				new_left_over = left_over_ + i_size;
				offset				= (k_arena_size_ - new_left_over);

				// This memory fixed up by alignment and is within range of alignment
				// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
				if ((reinterpret_cast<std::uintptr_t>(i_data) - (reinterpret_cast<std::uintptr_t>(buffer_) + offset)) <
						i_alignment)
				{
					left_over_ = new_left_over;
				}
			}
		}
	}

	auto get_free_size() const -> size_type
	{
		return left_over_;
	}

	auto operator<=>(linear_allocator const&) const = default;

private:
	address		buffer_;
	size_type left_over_;
	size_type k_arena_size_;
};

} // namespace acl