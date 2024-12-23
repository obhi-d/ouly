#pragma once
#include <acl/allocators/arena.hpp>
#include <acl/utils/type_traits.hpp>

namespace acl::strat
{

/**
 * @brief This class provides a mechanism to allocate blocks of addresses
 *        by linearly searching through a list of available free sizes and
 *        returning the first chunk that can fit the requested memory size.
 */
template <typename Options = acl::options<>>
class greedy_v0
{
	using optional_addr = detail::optional_val<detail::k_null_32>;

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

	greedy_v0() noexcept						= default;
	greedy_v0(greedy_v0 const&)			= default;
	greedy_v0(greedy_v0&&) noexcept = default;

	greedy_v0& operator=(greedy_v0 const&)		 = default;
	greedy_v0& operator=(greedy_v0&&) noexcept = default;

	[[nodiscard]] inline optional_addr try_allocate(bank_data& bank, size_type size)
	{
		for (uint32_t i = 0, en = static_cast<uint32_t>(free_list.size()); i < en; ++i)
		{
			auto& f = free_list[i];
			if (f.first >= size)
				return optional_addr(i);
		}
		return optional_addr();
	}

	inline std::uint32_t commit(bank_data& bank, size_type size, optional_addr found)
	{
		ACL_ASSERT(found.value < static_cast<uint32_t>(free_list.size()));

		auto& free_node = free_list[found.value];
		auto	block			= free_node.second;
		auto& blk				= bank.blocks[block];
		// Marker
		size_type			offset		= blk.offset;
		std::uint32_t arena_num = blk.arena;

		blk.is_free = false;

		auto remaining = blk.size - size;
		blk.size			 = size;
		if (remaining > 0)
		{
			auto& list	= bank.arenas[blk.arena].block_order;
			auto	arena = blk.arena;

			auto newblk = bank.blocks.emplace(blk.offset + size, remaining, arena, found.value, true);
			list.insert_after(bank.blocks, (uint32_t)free_node.second, (uint32_t)newblk);
			// reinsert the left-over size in free list
			free_node.first	 = remaining;
			free_node.second = newblk;
		}
		else
		{
			free_node.first	 = 0;
			free_node.second = block_link(free_slot);
			free_slot				 = found.value;
		}

		return (uint32_t)block;
	}

	inline void add_free_arena([[maybe_unused]] block_bank& blocks, std::uint32_t block)
	{
		add_free(blocks, block);
	}

	inline void add_free(block_bank& blocks, std::uint32_t block)
	{
		auto	hblock					 = block_link(block);
		auto	slot						 = ensure_free_slot();
		auto& blk							 = blocks[hblock];
		blk.reserved32_				 = slot;
		free_list[slot].first	 = blk.size;
		free_list[slot].second = hblock;
	}

	inline void grow_free_node(block_bank& blocks, std::uint32_t block, size_type newsize)
	{
		erase(blocks, block);
		blocks[block_link(block)].size = newsize;
		add_free(blocks, block);
	}

	inline void replace_and_grow(block_bank& blocks, std::uint32_t block, std::uint32_t new_block, size_type new_size)
	{
		erase(blocks, block);
		blocks[block_link(new_block)].size = new_size;
		add_free(blocks, new_block);
	}

	inline void erase(block_bank& blocks, std::uint32_t node)
	{
		auto	hblock		 = block_link(node);
		auto	idx				 = blocks[hblock].reserved32_;
		auto& free_node	 = free_list[idx];
		free_node.first	 = 0;
		free_node.second = block_link(free_slot);
		free_slot				 = idx;
	}

	inline std::uint32_t total_free_nodes(block_bank const& blocks) const
	{
		uint32_t count = 0;
		for (auto fn : free_list)
		{
			if (fn.first)
				count++;
		}
		return count;
	}

	inline size_type total_free_size(block_bank const& blocks) const
	{
		size_type sz = 0;
		for (auto fn : free_list)
		{
			sz += fn.first;
		}
		return sz;
	}

	void validate_integrity(block_bank const& blocks) const
	{
		size_type sz = 0;
		for (uint32_t i = 0, en = (uint32_t)free_list.size(); i < en; ++i)
		{
			auto fn = free_list[i];
			if (fn.first)
			{
				auto& blk = blocks[fn.second];
				ACL_ASSERT(blk.size == fn.first);
				ACL_ASSERT(blk.reserved32_ == i);
			}
		}
	}

	template <typename Owner>
	inline void init(Owner const& owner)
	{}

protected:
	// Private

	inline uint32_t ensure_free_slot()
	{
		uint32_t r = free_slot;
		if (free_slot)
		{
			free_slot = free_list[free_slot].first;
		}
		else
		{
			r = (uint32_t)free_list.size();
			free_list.emplace_back();
		}
		return r;
	}

	std::vector<std::pair<size_type, block_link>> free_list;
	uint32_t																			free_slot = 0;
};

/**
 * alloc_strategy::best_fit Impl
 */

} // namespace acl::strat