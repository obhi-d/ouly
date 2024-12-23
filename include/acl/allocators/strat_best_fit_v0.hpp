#pragma once

#include <acl/allocators/arena.hpp>
#include <acl/utils/type_traits.hpp>
#include <optional>

namespace acl::strat
{

/**
 * @brief  Strategy class for arena_allocator that stores a
 *         sorted list of free available slots.
 *         Binary search is used to find the best slot that fits
 *         the requested memory
 * @todo   optimize, branchless binary
 */
template <typename Options = acl::options<>>
class best_fit_v0
{
	using optional_addr = std::optional<detail::free_list::iterator>;

public:
	using extension				= uint64_t;
	using size_type				= detail::choose_size_t<uint32_t, Options>;
	using arena_bank			= detail::arena_bank<size_type, extension>;
	using block_bank			= detail::block_bank<size_type, extension>;
	using block						= detail::block<size_type, extension>;
	using bank_data				= detail::bank_data<size_type, extension>;
	using block_link			= typename block_bank::link;
	using allocate_result = optional_addr;

	static constexpr size_type min_granularity = 4;

	best_fit_v0() noexcept							= default;
	best_fit_v0(best_fit_v0 const&)			= default;
	best_fit_v0(best_fit_v0&&) noexcept = default;

	best_fit_v0& operator=(best_fit_v0 const&)		 = default;
	best_fit_v0& operator=(best_fit_v0&&) noexcept = default;

	[[nodiscard]] inline optional_addr try_allocate(bank_data& bank, size_type size)
	{
		if (free_ordering.size() == 0 || bank.blocks[block_link(free_ordering.back())].size < size)
			return optional_addr();
		return find_free(bank.blocks, free_ordering.begin(), free_ordering.end(), size);
	}

	inline std::uint32_t commit(bank_data& bank, size_type size, optional_addr const& found_)
	{
		auto					found			= *found_;
		std::uint32_t free_node = *found;
		auto&					blk				= bank.blocks[block_link(free_node)];
		// Marker
		size_type			offset		= blk.offset;
		std::uint32_t arena_num = blk.arena;

		blk.is_free = false;

		auto remaining = blk.size - size;
		blk.size			 = size;
		if (remaining > 0)
		{
			auto& list	 = bank.arenas[blk.arena].block_order;
			auto	arena	 = blk.arena;
			auto	newblk = bank.blocks.emplace(blk.offset + size, remaining, arena, detail::k_null_sz<uhandle>, true);
			list.insert_after(bank.blocks, free_node, (uint32_t)newblk);
			// reinsert the left-over size in free list
			reinsert_left(bank.blocks, found, (uint32_t)newblk);
		}
		else
		{
			// delete the existing found index from free list
			free_ordering.erase(found);
		}

		return free_node;
	}

	inline void add_free_arena([[maybe_unused]] block_bank& blocks, std::uint32_t block)
	{
		free_ordering.push_back(block);
	}

	inline void add_free(block_bank& blocks, std::uint32_t block)
	{
		add_free_after_begin(blocks, block);
	}

	inline void grow_free_node(block_bank& blocks, std::uint32_t block, size_type newsize)
	{
		auto& blk = blocks[block_link(block)];

		auto end = free_ordering.end();
		auto it	 = find_free_it(blocks, free_ordering.begin(), end, blk.size);
		if constexpr (detail::debug)
		{
			while (it != end && *it != block)
				it++;
			ACL_ASSERT(it < end);
		}
		else
		{
			while (*it != block)
				it++;
		}
		blk.size = newsize;
		reinsert_right(blocks, it, block);
	}

	inline void replace_and_grow(block_bank& blocks, std::uint32_t block, std::uint32_t new_block, size_type new_size)
	{
		size_type size = blocks[block_link(block)].size;

		auto end = free_ordering.end();
		auto it	 = find_free_it(blocks, free_ordering.begin(), end, size);
		if constexpr (detail::debug)
		{
			while (it != end && *it != block)
				it++;
			ACL_ASSERT(it < end);
		}
		else
		{
			while (*it != block)
				it++;
		}

		blocks[block_link(new_block)].size = new_size;
		reinsert_right(blocks, it, new_block);
	}

	inline void erase(block_bank& blocks, std::uint32_t block)
	{
		auto end = free_ordering.end();
		auto it	 = find_free_it(blocks, free_ordering.begin(), end, blocks[block_link(block)].size);
		if constexpr (detail::debug)
		{
			while (it != end && *it != block)
				it++;
			ACL_ASSERT(it < end);
		}
		else
		{
			while (*it != block)
				it++;
		}
		free_ordering.erase(it);
	}

	inline std::uint32_t total_free_nodes(block_bank const& blocks) const
	{
		return static_cast<std::uint32_t>(free_ordering.size());
	}

	inline size_type total_free_size(block_bank const& blocks) const
	{
		size_type sz = 0;
		for (auto fn : free_ordering)
		{
			ACL_ASSERT(blocks[block_link(fn)].is_free);
			sz += blocks[block_link(fn)].size;
		}

		return sz;
	}

	void validate_integrity(block_bank const& blocks) const
	{
		size_type sz = 0;
		for (auto fn : free_ordering)
		{
			ACL_ASSERT(sz <= blocks[block_link(fn)].size);
			sz = blocks[block_link(fn)].size;
		}
	}

	template <typename Owner>
	inline void init(Owner const& owner)
	{}

protected:
	// Private
	inline void add_free_after_begin(block_bank& blocks, std::uint32_t block)
	{
		auto blkid						= block_link(block);
		blocks[blkid].is_free = true;
		auto it								= find_free_it(blocks, free_ordering.begin(), free_ordering.end(), blocks[blkid].size);
		free_ordering.emplace(it, block);
	}

	template <typename It>
	static inline auto find_free_it(block_bank& blocks, It b, It e, size_type i_size)
	{
		return std::lower_bound(b, e, i_size,
														[&blocks](std::uint32_t block, size_type i_size) -> bool
														{
															return blocks[block_link(block)].size < i_size;
														});
	}

	inline optional_addr find_free(block_bank& blocks, auto b, auto e, size_type i_size) const
	{
		auto it = find_free_it(blocks, b, e, i_size);
		return (it != e) ? optional_addr(it) : optional_addr();
	}

	inline void reinsert_left(block_bank& blocks, auto of, std::uint32_t node)
	{
		auto begin_it = free_ordering.begin();
		if (begin_it == of)
		{
			*of = node;
		}
		else
		{
			auto it = find_free_it(blocks, begin_it, of, blocks[block_link(node)].size);
			if (it != of)
			{
				std::uint32_t* src	 = &*it;
				std::uint32_t* dest	 = src + 1;
				std::size_t		 count = std::distance(it, of);
				std::memmove(dest, src, count * sizeof(std::uint32_t));
				*it = node;
			}
			else
			{
				*of = node;
			}
		}
	}

	inline void reinsert_right(block_bank& blocks, auto of, std::uint32_t node)
	{

		auto end_it = free_ordering.end();
		auto next		= std::next(of);
		if (next == end_it)
		{
			*of = node;
		}
		else
		{
			auto it = find_free_it(blocks, next, end_it, blocks[block_link(node)].size);
			if (it != next)
			{
				std::uint32_t* dest	 = &(*of);
				std::uint32_t* src	 = dest + 1;
				std::size_t		 count = std::distance(next, it);
				std::memmove(dest, src, count * sizeof(std::uint32_t));
				auto ptr = (dest + count);
				*ptr		 = node;
			}
			else
			{
				*of = node;
			}
		}
	}

	detail::free_list free_ordering;
};

/**
 * alloc_strategy::best_fit Impl
 */

} // namespace acl::strat
