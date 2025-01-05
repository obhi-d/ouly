
#pragma once

#include <acl/allocators/allocator.hpp>
#include <acl/allocators/memory_stats.hpp>
#include <acl/containers/vlist.hpp>
#include <acl/utils/config.hpp>
#include <compare>
#include <cstdint>
#include <limits>
#include <span>
#include <vector>

namespace acl
{
#define ACL_BINARY_SEARCH_STEP                                                                                         \
	{                                                                                                                    \
		const size_type* const middle = it + (size >> 1);                                                                  \
		size													= (size + 1) >> 1;                                                                   \
		it														= *middle < key ? middle : it;                                                       \
	}

/**
 * @brief This allocator grows the buffer size, and merges free sizes_.
 */
struct allocation_id
{
	uint32_t id_ = std::numeric_limits<uint32_t>::max();

	[[nodiscard]] auto get() const noexcept -> uint32_t
	{
		return id_;
	}

	auto operator<=>(allocation_id const&) const noexcept = default;
};

struct arena_id
{
	uint16_t id_ = std::numeric_limits<uint16_t>::max();

	[[nodiscard]] auto get() const noexcept -> uint16_t
	{
		return id_;
	}
	auto operator<=>(arena_id const&) const noexcept = default;
};

using allocation_size_type = std::conditional_t<detail::coalescing_allocator_large_size, uint64_t, uint32_t>;

template <typename T>
concept CoalescingMemoryManager = requires(T m) {
	/**
	 * Drop Arena
	 */
	{ m.remove(arena_id()) } -> std::same_as<void>;
	/**
	 * Add an arena
	 */
	{ m.add(arena_id(), allocation_size_type()) } -> std::same_as<void>;
};

namespace detail
{

template <typename T>
struct ca_bank
{
	uint32_t			 free_idx_ = 0;
	std::vector<T> entries_	 = {T()};

	auto push(T const& data) -> uint32_t
	{
		if (free_idx_ != 0U)
		{
			auto entry			= free_idx_;
			free_idx_				= entries_[free_idx_].order_.next_;
			entries_[entry] = data;
			return entry;
		}

		auto id = static_cast<uint32_t>(entries_.size());
		entries_.emplace_back(data);
		return id;
	}
};

template <typename T>
struct ca_accessor
{
	using value_type = T;
	using bank_type	 = ca_bank<T>;
	using size_type	 = allocation_size_type;
	using container	 = bank_type;

	static void erase(bank_type& bank, std::uint32_t node)
	{
		bank.entries_[node].order_.next_ = bank.free_idx_;
		bank.free_idx_									 = node;
	}

	static auto node(bank_type& bank, std::uint32_t node) -> detail::list_node&
	{
		return bank.entries_[node].order_;
	}

	static auto node(bank_type const& bank, std::uint32_t node) -> detail::list_node const&
	{
		return bank.entries_[node].order_;
	}

	static auto get(bank_type const& bank, std::uint32_t node) -> value_type const&
	{
		return bank.entries_[node];
	}

	static auto get(bank_type& bank, std::uint32_t node) -> value_type&
	{
		return bank.entries_[node];
	}
};

template <typename T>
using ca_list = detail::vlist<ca_accessor<T>>;

struct ca_block_entries
{
	uint32_t													free_idx_		 = 0;
	std::vector<detail::list_node>		ordering_		 = {detail::list_node()};
	std::vector<allocation_size_type> offsets_		 = {0};
	std::vector<allocation_size_type> sizes_			 = {0};
	std::vector<uint16_t>							arenas_			 = {0};
	std::vector<bool>									free_marker_ = {false};

	auto push() -> uint32_t
	{
		if (free_idx_ != 0U)
		{
			auto entry = free_idx_;
			free_idx_	 = offsets_[free_idx_];
			return entry;
		}

		auto id = static_cast<uint32_t>(ordering_.size());
		ordering_.emplace_back();
		offsets_.emplace_back();
		sizes_.emplace_back();
		arenas_.emplace_back();
		free_marker_.emplace_back();
		return id;
	}

	auto push(allocation_size_type offset, allocation_size_type size, uint16_t arena, bool is_free) -> uint32_t
	{
		if (free_idx_ != 0U)
		{
			auto entry					= free_idx_;
			free_idx_						= offsets_[free_idx_];
			ordering_[entry]		= {};
			offsets_[entry]			= offset;
			sizes_[entry]				= size;
			arenas_[entry]			= arena;
			free_marker_[entry] = is_free;
			return entry;
		}

		auto id = static_cast<uint32_t>(offsets_.size());
		ordering_.emplace_back();
		offsets_.emplace_back(offset);
		sizes_.emplace_back(size);
		arenas_.emplace_back(arena);
		free_marker_.emplace_back(is_free);
		return id;
	}
};

struct ca_block_accessor
{
	using container	 = ca_block_entries;
	using value_type = std::uint32_t;

	static void erase(ca_block_entries& bank, std::uint32_t node)
	{
		bank.offsets_[node] = bank.free_idx_;
		bank.free_idx_			= node;
	}

	static auto node(ca_block_entries& bank, std::uint32_t node_id) -> detail::list_node&
	{
		return bank.ordering_[node_id];
	}

	static auto node(ca_block_entries const& bank, std::uint32_t node_id) -> detail::list_node const&
	{
		return bank.ordering_[node_id];
	}

	static auto get(ca_block_entries const& bank, std::uint32_t node) -> value_type
	{
		return node;
	}

	static auto get(ca_block_entries& bank, std::uint32_t& node) -> value_type&
	{
		return node;
	}
};

using ca_block_list = detail::vlist<ca_block_accessor>;

struct ca_arena
{
	using size_type = allocation_size_type;

	ca_block_list			blocks_;
	detail::list_node order_;
	size_type					size_			 = 0;
	size_type					free_size_ = 0;
};

using ca_arena_entries = ca_bank<ca_arena>;
using ca_arena_list		 = ca_list<ca_arena>;

struct ca_allocator_tag
{};

} // namespace detail

struct ca_allocation
{
	allocation_size_type offset_ = 0;
	allocation_id				 id_;
	arena_id						 arena_;

	auto operator<=>(ca_allocation const&) const noexcept = default;

	[[nodiscard]] auto get_offset() const noexcept -> allocation_size_type
	{
		return offset_;
	}

	[[nodiscard]] auto get_allocation_id() const noexcept -> allocation_id
	{
		return id_;
	}

	[[nodiscard]] auto get_arena_id() const noexcept -> arena_id
	{
		return arena_;
	}
};

#ifdef ACL_DEBUG
using coalescing_arena_allocator_base = detail::statistics<detail::ca_allocator_tag, acl::options<opt::compute_stats>>;
#else
using coalescing_arena_allocator_base = detail::statistics<detail::ca_allocator_tag, acl::options<>>;
#endif

/**
 * @brief A coalescing arena allocator that manages memory blocks within arenas
 *
 * This allocator maintains a collection of memory arenas and manages allocations within them.
 * It supports coalescing of free blocks to reduce fragmentation. The allocator tracks block
 * sizes, offsets and arena assignments.
 *
 * Key features:
 * - Supports variable sized allocations
 * - Coalesces adjacent free blocks
 * - Tracks arena assignments for blocks
 * - Allows arena size adjustments
 * - Supports dedicated allocations
 * - Maintains allocation metadata
 *
 * The allocator uses an arena-based approach where memory is allocated in chunks (arenas)
 * and then sub-allocated into smaller blocks. When blocks are freed, adjacent free blocks
 * are merged to reduce fragmentation.
 *
 * @note This allocator inherits from coalescing_arena_allocator_base
 * @note Arena size can only be increased, not decreased
 * @note Allocation information can be retrieved using allocation IDs
 * @note Dedicated allocations bypass block coalescing
 * @note Arena and allocation IDs are consequetive integers, and can be used as indexes.
 */
class coalescing_arena_allocator : coalescing_arena_allocator_base
{
public:
	using size_type = allocation_size_type;

	coalescing_arena_allocator() noexcept																						 = default;
	auto operator=(const coalescing_arena_allocator&) -> coalescing_arena_allocator& = delete;
	auto operator=(coalescing_arena_allocator&&) -> coalescing_arena_allocator&			 = delete;
	coalescing_arena_allocator(coalescing_arena_allocator const&) noexcept					 = delete;
	coalescing_arena_allocator(coalescing_arena_allocator&&) noexcept								 = delete;
	coalescing_arena_allocator(size_type arena_sz) noexcept : arena_size_(arena_sz) {}
	~coalescing_arena_allocator() noexcept = default;

	/** @brief Arena size can be changed any time with this method, but it can only increase in size. */
	void set_arena_size(size_type s) noexcept
	{
		arena_size_ = std::max(arena_size_, s);
	}

	/** @return Arena size currently in use. */
	[[nodiscard]] auto get_arena_size() const noexcept -> size_type
	{
		return arena_size_;
	}

	/** @brief Given an allocation_id return the offset in the arena it belongs to */
	[[nodiscard]] auto get_size(allocation_id id) const noexcept -> size_type
	{
		return block_entries_.sizes_[id.get()];
	}

	/** @brief Given an allocation_id return the offset in the arena it belongs to */
	[[nodiscard]] auto get_offset(allocation_id id) const noexcept -> size_type
	{
		return block_entries_.offsets_[id.get()];
	}

	/** @brief Given an allocation_id return the arena it belongs to */
	[[nodiscard]] auto get_arena(allocation_id id) const noexcept -> arena_id
	{
		return arena_id{block_entries_.arenas_[id.get()]};
	}
	/** @brief The method `allocate` returns an allocation desc, with extra information about the allocation offset and
	 * the arena the allocation belongs to. The information need not be stored, as the alllocation_id can be used to fetch
	 * this information */
	template <CoalescingMemoryManager M, typename Alignment = acl::alignment<>, typename Dedicated = std::false_type>
	auto allocate(size_type size, M& manager, Alignment alignment = {}, Dedicated /*unused*/ = {}) -> ca_allocation
	{
		auto measure = statistics::report_allocate(size);
		auto vsize	 = alignment ? size + static_cast<size_type>(alignment) : size;

		if (Dedicated::value || vsize >= arena_size_)
		{
			auto [arena, block] = add_arena_filled(vsize, manager);
			return ca_allocation{.offset_ = 0, .id_ = block, .arena_ = arena};
		}

		ca_allocation al = try_allocate(size);

		if (al.get_allocation_id() == allocation_id())
		{
			add_arena(vsize, manager);
			al = try_allocate(size);
		}

		return al;
	}

	/** @brief Dellocate an allocation. The manager must be provided for removal of arenas_. */
	template <CoalescingMemoryManager M>
	void deallocate(allocation_id id, M& manager)
	{
		auto aa = deallocate(id);
		if (aa != arena_id())
		{
			manager.remove(aa);
		}
	}

	void validate_integrity() const;

	[[nodiscard]] auto get_offsets() const noexcept -> std::span<allocation_size_type const>
	{
		return block_entries_.offsets_;
	}

	[[nodiscard]] auto get_sizes() const noexcept -> std::span<allocation_size_type const>
	{
		return block_entries_.sizes_;
	}

	[[nodiscard]] auto get_arena_indices() const noexcept -> std::span<uint16_t const>
	{
		return block_entries_.arenas_;
	}

private:
	auto add_arena(size_type size, bool empty) -> std::pair<arena_id, allocation_id>;

	template <CoalescingMemoryManager M>
	auto add_arena_filled(size_type size, M& manager) -> std::pair<arena_id, allocation_id>
	{
		statistics::report_new_arena();
		auto ret = add_arena(size, false);
		manager.add(ret.first, size);
		return ret;
	}

	template <CoalescingMemoryManager M>
	void add_arena(size_type size, M& manager)
	{
		size						= std::max(size, arena_size_);
		auto [arena, _] = add_arena(size, true);
		manager.add(arena, size);
	}

	void add_free_arena(uint32_t block)
	{
		sizes_.push_back(block_entries_.sizes_[block]);
		free_ordering_.push_back(block);
	}

	void grow_free_node(uint32_t /*block*/, size_type size);
	void replace_and_grow(uint32_t right, uint32_t node, size_type new_size);
	void add_free(uint32_t node);
	void erase(uint32_t node);
	auto deallocate(allocation_id id) -> arena_id;

	static auto mini2(size_type const* it, size_t size, size_type key) noexcept
	{
		while (true)
		{
			ACL_BINARY_SEARCH_STEP;
			ACL_BINARY_SEARCH_STEP;
			if (size <= 2)
			{
				break;
			}
		}

		it += static_cast<size_type>(size > 1 && (*it < key));
		it += static_cast<size_type>(size > 0 && (*it < key));
		return it;
	}

	static auto mini2_it(size_type const* it, size_t s, size_type key) noexcept
	{
		return std::distance(it, mini2(it, s, key));
	}

	[[nodiscard]] auto find_free(size_type size) const noexcept
	{
		return mini2(sizes_.data(), sizes_.size(), size);
	}

	[[nodiscard]] auto total_free_nodes() const noexcept -> uint32_t
	{
		return static_cast<std::uint32_t>(free_ordering_.size());
	}

	[[nodiscard]] auto total_free_size() const noexcept -> size_type
	{
		size_type sz = 0;
		for (auto fn : sizes_)
		{
			sz += fn;
		}

		return sz;
	}

	auto try_allocate(size_type size) noexcept -> ca_allocation
	{
		if (sizes_.empty() || sizes_.back() < size)
		{
			return {};
		}
		const auto* it = find_free(size);
		auto				id = commit(size, it);
		return ca_allocation{
		 .offset_ = block_entries_.offsets_[id], .id_ = {.id_ = id}, .arena_ = {.id_ = block_entries_.arenas_[id]}};
	}

	void reinsert_left(size_t of, size_type size, std::uint32_t node) noexcept;
	void reinsert_right(size_t of, size_type size, std::uint32_t node);
	auto commit(size_type size, size_type const* found) noexcept -> uint32_t;

	// Free blocks
	detail::ca_arena_entries arena_entries_;
	detail::ca_block_entries block_entries_;
	detail::ca_arena_list		 arenas_;

	std::vector<size_type> sizes_;
	std::vector<uint32_t>	 free_ordering_;

	size_type arena_size_ = 0;
};
#undef ACL_BINARY_SEARCH_STEP
} // namespace acl
