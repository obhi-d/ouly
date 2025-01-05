#pragma once

#include "arena.hpp"

#include <bit>
#include <concepts>
#include <functional>
#include <tuple>

#include "strat_best_fit_tree.hpp"
#include "strat_best_fit_v0.hpp"
#include "strat_best_fit_v1.hpp"
#include "strat_best_fit_v2.hpp"
#include "strat_greedy_v0.hpp"
#include "strat_greedy_v1.hpp"

namespace acl
{
template <typename T>
concept MemoryManager = requires(T m) {
	/**
	 * Drop Arena
	 */
	{ m.drop_arena(acl::uhandle()) } -> std::same_as<bool>;
	/**
	 * Add an arena
	 */
	{ m.add_arena(acl::ihandle(), std::size_t()) } -> std::same_as<acl::uhandle>;
	// Remoe an arena
	m.remove_arena(acl::uhandle());
};

template <typename T, typename A>
concept HasDefragmentSupport = requires(T a, A& allocator, acl::uhandle src_arena, acl::uhandle dst_arena,
																				acl::uhandle alloc_info, std::size_t from, std::size_t to, std::size_t size) {
	// Begin defragment
	a.begin_defragment(allocator);
	// End defragmentation
	a.end_defragment(allocator);
	// Rebind an allocation to another value
	a.rebind_alloc(alloc_info, src_arena, alloc_info, typename A::size_type());
	// Move memory from source arena to another
	a.move_memory(src_arena, dst_arena, from, to, size);
};

} // namespace acl

namespace acl::opt
{

template <typename T>
struct extension
{
	using extension_t = T;
};

template <MemoryManager T>
struct manager
{
	using manager_t = T;
};

template <typename T>
struct strategy
{
	using strategy_t = T;
};

} // namespace acl::opt

namespace acl::detail
{

template <typename T>
concept HasMemoryManager = requires { typename T::manager_t; };

template <typename T>
concept HasAllocStrategy = requires { typename T::strategy_t; };

template <typename T>
struct manager
{
	using manager_t = std::false_type;
};

template <HasMemoryManager T>
struct manager<T>
{
	using manager_t = typename T::manager_t;
};

template <typename T>
struct strategy
{
	using strategy_t = acl::strat::best_fit_v2<acl::opt::bsearch_min1>;
};

template <HasAllocStrategy T>
struct strategy<T>
{
	using strategy_t = typename T::strategy_t;
};

struct defrag_stats
{
	std::uint32_t total_mem_move_merge_ = 0;
	std::uint32_t total_arenas_removed_ = 0;

	void report_defrag_mem_move_merge()
	{
		total_mem_move_merge_++;
	}

	void report_defrag_arenas_removed()
	{
		total_arenas_removed_++;
	}

	[[nodiscard]] auto print() const -> std::string
	{
		std::stringstream ss;
		ss << "Defrag memory move merges: " << total_mem_move_merge_ << "\n"
			 << "Defrag arenas removed: " << total_arenas_removed_;
		return ss.str();
	}
};

struct arena_allocator_tag
{};

} // namespace acl::detail

namespace acl
{

template <typename Options = std::monostate>
class arena_allocator
		: detail::statistics<detail::arena_allocator_tag, acl::options<Options, opt::base_stats<detail::defrag_stats>>>
{

public:
	using strategy			= typename detail::strategy<Options>::strategy_t;
	using extension			= typename strategy::extension;
	using arena_manager = typename detail::manager<Options>::manager_t;
	using size_type			= detail::choose_size_t<uint32_t, Options>;
	using this_type			= arena_allocator<Options>;

	static constexpr bool has_memory_mgr = detail::HasMemoryManager<Options>;
	static constexpr bool can_defragment = HasDefragmentSupport<arena_manager, this_type>;

protected:
	using options = Options;

	using block					 = detail::block<size_type, extension>;
	using block_bank		 = detail::block_bank<size_type, extension>;
	using block_accessor = detail::block_accessor<size_type, extension>;
	using block_list		 = detail::block_list<size_type, extension>;
	using arena_bank		 = detail::arena_bank<size_type, extension>;
	using arena_list		 = detail::arena_list<size_type, extension>;

	using super =
	 detail::statistics<detail::arena_allocator_tag, acl::options<Options, opt::base_stats<detail::defrag_stats>>>;

	using statistics = super;
	using block_link = typename block_bank::link;

	using bank_data = detail::bank_data<size_type, extension>;

	struct remap_data
	{
		bank_data bank_;
		strategy	strat_;
	};

public:
	struct memory_move
	{
		size_type			from_{};
		size_type			to_{};
		size_type			size_{};
		std::uint32_t arena_src_{};
		std::uint32_t arena_dst_{};

		[[nodiscard]] auto is_moved() const noexcept -> bool
		{
			return (from_ != to_ || arena_src_ != arena_dst_);
		}

		memory_move() noexcept														 = default;
		memory_move(const memory_move&)										 = default;
		memory_move(memory_move&&)												 = default;
		auto operator=(const memory_move&) -> memory_move& = default;
		auto operator=(memory_move&&) -> memory_move&			 = default;
		memory_move(size_type ifrom, size_type ito, size_type isize, std::uint32_t iarena_src, std::uint32_t iarena_dst)
				: from_(ifrom), to_(ito), size_(isize), arena_src_(iarena_src), arena_dst_(iarena_dst)
		{}

		~memory_move() = default;

		void reset()
		{
			from_			 = 0;
			to_				 = 0;
			size_			 = 0;
			arena_src_ = 0;
			arena_dst_ = 0;
		}

		auto from() const noexcept -> size_type
		{
			return from_;
		}

		auto to() const noexcept -> size_type
		{
			return to_;
		}

		auto size() const noexcept -> size_type
		{
			return size_;
		}

		[[nodiscard]] auto arena_src() const noexcept -> std::uint32_t
		{
			return arena_src_;
		}

		[[nodiscard]] auto arena_dst() const noexcept -> std::uint32_t
		{
			return arena_dst_;
		}
	};

	/**
	 * Allocation info: [optional] arena, offset, input_handle
	 */
	using alloc_info =
	 std::conditional_t<has_memory_mgr, std::tuple<uhandle, ihandle, size_type>, std::pair<ihandle, size_type>>;

	arena_allocator(size_type i_arena_size, arena_manager& i_manager) noexcept
		requires(has_memory_mgr)
			: mgr_(&i_manager), arena_size_(i_arena_size)
	{
		ibank_.strat_.init(*this);
	}

	arena_allocator() noexcept
	{
		ibank_.strat_.init(*this);
		if constexpr (!has_memory_mgr)
		{
			add_arena(detail::k_null_sz<uhandle>, arena_size_, true);
		}
	}

	template <typename... Args>
	arena_allocator(size_type i_arena_size) noexcept : arena_size_(i_arena_size)
	{
		ibank_.strat_.init(*this);
		if constexpr (!has_memory_mgr)
		{
			add_arena(detail::k_null_sz<uhandle>, arena_size_, true);
		}
	}

	arena_allocator(arena_allocator const&) noexcept = default;
	arena_allocator(arena_allocator&&) noexcept			 = default;

	~arena_allocator() noexcept = default;

	auto operator=(arena_allocator const&) noexcept -> arena_allocator& = default;
	auto operator=(arena_allocator&&) noexcept -> arena_allocator&			= default;

	auto get_root_block() const
	{
		return ibank_.bank_.root_blk;
	}

	//! get allocation info
	auto get_alloc_offset(ihandle i_address) const
	{
		auto const& blk = ibank_.bank_.blocks()[block_link(i_address)];
		return std::pair(blk.arena_, blk.offset_);
	}

	//! Allocate
	template <typename Alignment = alignment<>, typename Dedicated = std::false_type>
	auto allocate(size_type isize, Alignment i_alignment = {}, uhandle huser = {}, Dedicated /*unused*/ = {})
	 -> alloc_info
	{
		auto measure = this->statistics::report_allocate(isize);
		auto size		 = isize + static_cast<size_type>(i_alignment);

		if (Dedicated::value || size >= arena_size_)
		{
			auto ret = add_arena(huser, size, false);
			if constexpr (has_memory_mgr)
			{
				return alloc_info(ibank_.bank_.arenas()[ret.first].data_, ret.second, 0);
			}
			else
			{
				return alloc_info(ret.second, 0);
			}
		}

		std::uint32_t id = null();
		if (auto ta = ibank_.strat_.try_allocate(ibank_.bank_, size))
		{
			id = ibank_.strat_.commit(ibank_.bank_, size, ta);
		}
		else
		{
			if constexpr (has_memory_mgr)
			{
				if (id == null())
				{
					add_arena(detail::k_null_sz<uhandle>, arena_size_, true);
					ta = ibank_.strat_.try_allocate(ibank_.bank_, size);
					if (ta)
					{
						id = ibank_.strat_.commit(ibank_.bank_, size, ta);
					}
				}
			}
		}

		if (id == null())
		{
			return alloc_info();
		}

		auto& blk = ibank_.bank_.blocks()[block_link(id)];

		if constexpr (has_memory_mgr)
		{
			return alloc_info(ibank_.bank_.arenas()[blk.arena_].data_, id, finalize_commit(blk, huser, i_alignment));
		}
		else
		{
			return alloc_info(id, finalize_commit(blk, huser, i_alignment));
		}
	}

	//! Deallocate, size is optional
	void deallocate(ihandle node)
	{
		auto& blk			= ibank_.bank_.blocks()[block_link(node)];
		auto	measure = this->statistics::report_deallocate(blk.size());

		enum
		{
			f_left	= 1 << 0,
			f_right = 1 << 1,
		};

		enum merge_type
		{
			e_none,
			e_left,
			e_right,
			e_left_and_right
		};

		auto& arena			= ibank_.bank_.arenas()[blk.arena_];
		auto& node_list = arena.block_order();

		// last index is not used
		ibank_.bank_.free_size_ += blk.size();
		arena.free_ += blk.size();
		auto size = blk.size();

		std::uint32_t left	 = 0;
		std::uint32_t right	 = 0;
		std::uint32_t merges = 0;

		if (node != node_list.front() && ibank_.bank_.blocks()[block_link(blk.arena_order_.prev_)].is_free_)
		{
			left = blk.arena_order_.prev_;
			merges |= f_left;
		}

		if (node != node_list.back() && ibank_.bank_.blocks()[block_link(blk.arena_order_.next_)].is_free_)
		{
			right = blk.arena_order_.next_;
			merges |= f_right;
		}

		if constexpr (has_memory_mgr)
		{
			if (arena.free_ == arena.size() && mgr_->drop_arena(arena.data_))
			{
				// drop arena?
				if (left != 0U)
				{
					ibank_.strat_.erase(ibank_.bank_.blocks(), left);
				}
				if (right != 0U)
				{
					ibank_.strat_.erase(ibank_.bank_.blocks(), right);
				}

				std::uint32_t arena_id = blk.arena_;
				ibank_.bank_.free_size_ -= arena.size();
				arena.size_ = 0;
				arena.block_order().clear(ibank_.bank_.blocks());
				ibank_.bank_.arena_order_.erase(ibank_.bank_.arenas(), arena_id);
				return;
			}
		}

		switch (merges)
		{
		case merge_type::e_none:
			ibank_.strat_.add_free(ibank_.bank_.blocks(), node);
			blk.is_free_ = true;
			break;
		case merge_type::e_left:
		{
			auto left_size = ibank_.bank_.blocks()[block_link(left)].size();
			ibank_.strat_.grow_free_node(ibank_.bank_.blocks(), left, left_size + size);
			node_list.erase(ibank_.bank_.blocks(), node);
		}
		break;
		case merge_type::e_right:
		{
			auto right_size = ibank_.bank_.blocks()[block_link(right)].size();
			ibank_.strat_.replace_and_grow(ibank_.bank_.blocks(), right, node, right_size + size);
			node_list.erase(ibank_.bank_.blocks(), right);
			blk.is_free_ = true;
		}
		break;
		case merge_type::e_left_and_right:
		{
			auto left_size	= ibank_.bank_.blocks()[block_link(left)].size();
			auto right_size = ibank_.bank_.blocks()[block_link(right)].size();
			ibank_.strat_.erase(ibank_.bank_.blocks(), right);
			ibank_.strat_.grow_free_node(ibank_.bank_.blocks(), left, left_size + right_size + size);
			node_list.erase2(ibank_.bank_.blocks(), node);
		}
		break;
		default:
			break;
		}
	}

	// set default arena size
	void set_arena_size(size_type isz)
	{
		arena_size_ = isz;
	}

	// null
	static constexpr auto null() -> ihandle
	{
		return 0;
	}

	// validate
	void validate_integrity() const
	{
		std::uint32_t total_free_nodes = 0;
		for (auto arena_it		 = ibank_.bank_.arena_order_.begin(ibank_.bank_.arenas()),
							arena_end_it = ibank_.bank_.arena_order_.end(ibank_.bank_.arenas());
				 arena_it != arena_end_it; ++arena_it)
		{
			auto& arena						= *arena_it;
			bool	arena_allocated = false;

			for (auto blk_it		 = arena.block_order().begin(ibank_.bank_.blocks()),
								blk_end_it = arena.block_order().end(ibank_.bank_.blocks());
					 blk_it != blk_end_it; ++blk_it)
			{
				auto& blk = *blk_it;
				if ((blk.is_free_))
				{
					total_free_nodes++;
				}
			}
		}

		assert(total_free_nodes == ibank_.strat_.total_free_nodes(ibank_.bank_.blocks()));
		auto total = ibank_.strat_.total_free_size(ibank_.bank_.blocks());
		assert(total == ibank_.bank_.free_size_);

		for (auto arena_it		 = ibank_.bank_.arena_order_.begin(ibank_.bank_.arenas()),
							arena_end_it = ibank_.bank_.arena_order_.end(ibank_.bank_.arenas());
				 arena_it != arena_end_it; ++arena_it)
		{
			auto&			arena						= *arena_it;
			bool			arena_allocated = false;
			size_type expected_offset = 0;

			for (auto blk_it		 = arena.block_order().begin(ibank_.bank_.blocks()),
								blk_end_it = arena.block_order().end(ibank_.bank_.blocks());
					 blk_it != blk_end_it; ++blk_it)
			{
				auto& blk = *blk_it;
				assert(blk.offset_ == expected_offset);
				expected_offset += blk.size();
			}
		}

		ibank_.strat_.validate_integrity(ibank_.bank_.blocks());
	}

	void defragment()
		requires(can_defragment)
	{
		mgr_->begin_defragment(*this);
		std::uint32_t arena_id = ibank_.bank_.arena_order_.first_;
		// refresh all banks
		remap_data refresh;
		refresh.strat_.init(*this);

		acl::vector<std::uint32_t> rebinds;
		rebinds.reserve(ibank_.bank_.blocks().size());

		acl::vector<memory_move>						moves;
		decltype(ibank_.bank_.arena_order_) deleted_arenas;
		for (auto arena_it = ibank_.bank_.arena_order_.front(); arena_it != 0;)
		{
			auto& arena						= ibank_.bank_.arenas()[arena_it];
			bool	arena_allocated = false;

			for (auto blk_it = arena.block_order().begin(ibank_.bank_.blocks()); blk_it;
					 blk_it			 = arena.block_order().erase(blk_it))
			{
				auto& blk = *blk_it;
				if (!blk.is_free_)
				{
					auto ta = refresh.strat_.try_allocate(refresh.bank_, blk.size());
					if (!ta && !arena_allocated)
					{
						auto p = add_arena(refresh, detail::k_null_uh, std::max(arena.size(), blk.size()), true);
						refresh.bank_.arenas()[p.first].data_ = arena.data_;
						ta																		= refresh.strat_.try_allocate(refresh.bank_, blk.size());
						arena_allocated												= true;
					}
					assert(ta);

					auto	new_blk_id = refresh.strat_.commit(refresh.bank_, blk.size(), ta);
					auto& new_blk		 = refresh.bank_.blocks()[block_link(new_blk_id)];
					refresh.bank_.arenas()[new_blk.arena_].free_ -= blk.size();
					refresh.bank_.free_size_ -= blk.size();

					copy(blk, new_blk);
					rebinds.emplace_back(new_blk_id);
					auto blk_adj = blk.adjusted_block();
					push_memmove(
					 moves, memory_move(blk_adj.first, new_blk.adjusted_offset(), blk_adj.second, blk.arena_, new_blk.arena_));
				}
			}

			if (!arena_allocated)
			{
				auto to_delete = arena_it;
				arena_it			 = ibank_.bank_.arena_order_.unlink(ibank_.bank_.arenas(), to_delete);
				arena.free_		 = arena.size();
				deleted_arenas.push_back(ibank_.bank_.arenas(), to_delete);
			}
			else
			{
				arena_it = ibank_.bank_.arena_order_.next(ibank_.bank_.arenas(), arena_it);
			}
		}

		for (auto& m : moves)
		{
			// follow the copy sequence to ensure there is no overwrite
			mgr_->move_memory(ibank_.bank_.arenas_[m.arena_src_].data_, refresh.bank_.arenas_[m.arena_dst_].data_, m.from_,
												m.to_, m.size_);
		}

		for (auto rb : rebinds)
		{
			auto& dst_blk = refresh.bank_.blocks()[block_link(rb)];
			mgr_->rebind_alloc(dst_blk.data_, refresh.bank_.arenas()[dst_blk.arena_].data_, rb, dst_blk.adjusted_offset());
		}

		for (auto arena_it = deleted_arenas.begin(ibank_.bank_.arenas()); arena_it;
				 arena_it			 = deleted_arenas.erase(arena_it))
		{
			auto& arena = *arena_it;
			mgr_->remove_arena(arena.data_);
			if constexpr (detail::HasComputeStats<Options>)
			{
				statistics::report_defrag_arenas_removed();
			}
		}

		ibank_.bank_	= std::move(refresh.bank_);
		ibank_.strat_ = std::move(refresh.strat_);
		mgr_->end_defragment(*this);
	}

private:
	auto add_arena(uhandle handle, size_type iarena_size, bool empty) -> std::pair<ihandle, ihandle>
	{
		this->statistics::report_new_arena();
		auto ret = add_arena(ibank_, handle, iarena_size, empty);
		if constexpr (has_memory_mgr)
		{
			ibank_.bank_.arenas()[ret.first].data_ = mgr_->add_arena(ret.first, iarena_size);
		}
		return ret;
	}

	static auto add_arena(remap_data& ibank, uhandle handle, size_type iarena_size, bool iempty)
	 -> std::pair<ihandle, ihandle>
	{

		std::uint32_t arena_id	= ibank.bank_.arenas().emplace();
		auto&					arena_ref = ibank.bank_.arenas()[arena_id];
		arena_ref.size_					= iarena_size;
		auto	 block_id					= ibank.bank_.blocks().emplace();
		block& block_ref				= ibank.bank_.blocks()[block_id];
		block_ref.offset_				= 0;
		block_ref.arena_				= arena_id;
		block_ref.data_					= handle;
		block_ref.size_					= iarena_size;
		if (iempty)
		{
			block_ref.is_free_ = true;
			arena_ref.free_		 = iarena_size;
			ibank.strat_.add_free_arena(ibank.bank_.blocks(), (uint32_t)block_id);
			ibank.bank_.free_size_ += iarena_size;
		}
		else
		{
			arena_ref.free_ = 0;
		}
		arena_ref.block_order().push_back(ibank.bank_.blocks(), (uint32_t)block_id);
		ibank.bank_.arena_order_.push_back(ibank.bank_.arenas(), arena_id);
		return std::make_pair(arena_id, (uint32_t)block_id);
	}

	template <typename Alignment = alignment<>>
	auto finalize_commit(block& blk, uhandle huser, Alignment ialign) -> size_type
	{
		blk.data_			 = huser;
		blk.alignment_ = static_cast<std::uint8_t>(std::popcount(static_cast<uint32_t>(ialign)));
		ibank_.bank_.arenas()[blk.arena_].free_ -= blk.size_;
		ibank_.bank_.free_size_ -= blk.size_;
		auto alignment = static_cast<size_type>(ialign);
		return ((blk.offset_ + alignment) & ~alignment);
	}

	static void copy(block const& src, block& dst)
	{
		dst.data_			 = src.data_;
		dst.alignment_ = src.alignment_;
	}

	void push_memmove(acl::vector<memory_move>& dst, memory_move value)
	{
		if (!value.is_moved())
		{
			return;
		}
		auto can_merge = [](memory_move const& m1, memory_move const& m2) -> bool
		{
			return ((m1.arena_dst_ == m2.arena_dst_ && m1.arena_src_ == m2.arena_src_) &&
							(m1.from_ + m1.size_ == m2.from_ && m1.to_ + m1.size_ == m2.to_));
		};
		if (dst.empty() || !can_merge(dst.back(), value))
		{
			dst.push_back(value);
		}
		else
		{
			dst.back().size_ += value.size_;
			if constexpr (detail::HasComputeStats<Options>)
			{
				statistics::report_defrag_mem_move_merge();
			}
		}
	}

	remap_data		 ibank_;
	size_type			 arena_size_ = std::numeric_limits<size_type>::max();
	arena_manager* mgr_				 = nullptr;
};

} // namespace acl
