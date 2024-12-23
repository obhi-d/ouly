#pragma once

#include "default_allocator.hpp"
#include <acl/utils/common.hpp>

namespace acl
{

namespace opt
{

template <std::size_t N>
struct atom_count
{
	static constexpr std::size_t atom_count_v = N;
};

template <std::size_t N>
struct atom_size
{
	static constexpr std::size_t atom_size_v = 1 << detail::log2(N);
};

template <std::size_t N>
struct atom_size_npt
{
	static constexpr std::size_t atom_size_v = N;
};
} // namespace opt

namespace detail
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
	std::uint32_t padding_atoms = 0;

	inline void pad_atoms(std::uint32_t v) noexcept
	{
		padding_atoms += v;
	}

	inline void unpad_atoms(std::uint32_t v) noexcept
	{
		padding_atoms -= v;
	}

	inline std::uint32_t padding_atoms_count() const noexcept
	{
		return padding_atoms;
	}

	std::string print() const
	{
		return {};
	}
};

} // namespace detail

struct pool_allocator_tag
{};

template <typename Options = acl::options<>>
class pool_allocator
		: detail::statistics<pool_allocator_tag, acl::options<Options, opt::base_stats<detail::padding_stats>>>
{
public:
	using tag = pool_allocator_tag;
	using statistics =
	 detail::statistics<pool_allocator_tag, acl::options<Options, opt::base_stats<detail::padding_stats>>>;
	static constexpr auto default_atom_size	 = detail::atom_size<Options>::value;
	static constexpr auto default_atom_count = detail::atom_count<Options>::value;
	using underlying_allocator							 = detail::underlying_allocator_t<Options>;
	using size_type													 = typename underlying_allocator::size_type;
	using address														 = typename underlying_allocator::address;

	pool_allocator() noexcept
			: k_atom_size(static_cast<size_type>(default_atom_size)), k_atom_count(static_cast<size_type>(default_atom_count))
	{}

	template <typename... Args>
	pool_allocator(size_type i_atom_size, size_type i_atom_count, Args&&... i_args)
			: k_atom_size(i_atom_size), k_atom_count(i_atom_count), statistics(std::forward<Args>(i_args)...)
	{}

	pool_allocator(pool_allocator const& i_other) = delete;
	inline pool_allocator(pool_allocator&& i_other) noexcept
			//  array_arena arrays;
			//  solo_arena      solo;
			//  const size_type k_atom_count;
			//  const size_type k_atom_size;
			//  arena_linker    linked_arenas;
			: arrays(std::move(i_other.arrays)), solo(std::move(i_other.solo)), k_atom_count(i_other.k_atom_count),
				k_atom_size(i_other.k_atom_size), linked_arenas(std::move(i_other.linked_arenas))
	{}

	pool_allocator&				 operator=(pool_allocator const& i_other) = delete;
	inline pool_allocator& operator=(pool_allocator&& i_other) noexcept
	{
		arrays = (std::move(i_other.arrays));
		solo	 = std::move(i_other.solo);
		ACL_ASSERT(k_atom_count == i_other.k_atom_count);
		ACL_ASSERT(k_atom_size = i_other.k_atom_size);
		linked_arenas = std::move(i_other.linked_arenas);
		return *this;
	}

	inline constexpr static address null()
	{
		return underlying_allocator::null();
	}

	template <typename Alignment = alignment<>>
	[[nodiscard]] inline address allocate(size_type size_value, Alignment alignment = {})
	{
		constexpr auto alignment_value = (size_t)alignment;
		auto					 fixup					 = alignment_value - 1;
		if (alignment_value && ((k_atom_size < alignment_value) || (k_atom_size & fixup)))
			size_value += alignment_value + 4;

		size_type i_count = (size_value + k_atom_size - 1) / k_atom_size;

		if constexpr (detail::HasComputeStats<Options>)
		{
			if (alignment_value && ((k_atom_size < alignment_value) || (k_atom_size & fixup)))
			{
				// Account for the missing atoms
				auto			real_size = size_value - alignment_value - 4;
				size_type count			= (real_size + k_atom_size - 1) / k_atom_size;
				this->statistics::pad_atoms(static_cast<std::uint32_t>(i_count - count));
			}
		}

		if (i_count > k_atom_count)
			return underlying_allocator::allocate(size_value, alignment);

		address ret_value;
		auto		measure = statistics::report_allocate(size_value);
		ret_value				= (i_count == 1) ? ((!solo) ? consume(1) : consume()) : consume(i_count);

		if (alignment_value && ((k_atom_size < alignment_value) || (k_atom_size & fixup)))
		{
			auto pointer = reinterpret_cast<std::uintptr_t>(ret_value);
			auto ret		 = ((pointer + 4 + static_cast<std::uintptr_t>(fixup)) & ~static_cast<std::uintptr_t>(fixup));
			*(reinterpret_cast<std::uint32_t*>(ret) - 1) = static_cast<std::uint32_t>(ret - pointer);
			return reinterpret_cast<address>(ret);
		}
		else
			return ret_value;
	}

	template <typename Alignment = alignment<>>
	inline void deallocate(address i_ptr, size_type size_value, Alignment alignment = {})
	{
		constexpr auto alignment_value = (size_t)alignment;
		auto					 fixup					 = alignment_value - 1;
		address				 orig_ptr				 = i_ptr;
		if (alignment_value && ((k_atom_size < alignment_value) || (k_atom_size & fixup)))
		{
			size_value += alignment_value + 4;
			std::uint32_t off_by = *(reinterpret_cast<std::uint32_t*>(i_ptr) - 1);
			i_ptr								 = reinterpret_cast<address>(reinterpret_cast<std::uint8_t*>(i_ptr) - off_by);
		}

		size_type i_count = (size_value + k_atom_size - 1) / k_atom_size;

		if constexpr (detail::HasComputeStats<Options>)
		{
			if (alignment_value && ((k_atom_size < alignment_value) || (k_atom_size & fixup)))
			{
				// Account for the missing atoms
				auto			real_size = size_value - alignment_value - 4;
				size_type count			= (real_size + k_atom_size - 1) / k_atom_size;
				this->statistics::unpad_atoms(static_cast<std::uint32_t>(i_count - count));
			}
		}

		if (i_count > k_atom_count)
			underlying_allocator::deallocate(orig_ptr, size_value, alignment);
		else
		{
			auto measure = statistics::report_deallocate(size_value);
			if (i_count == 1)
				release(i_ptr);
			else
				release(i_ptr, i_count);
		}
	}

private:
	struct array_arena
	{

		array_arena() : ppvalue(nullptr) {}
		array_arena(array_arena const& other) : ppvalue(other.ppvalue) {}
		array_arena(array_arena&& other) noexcept : ppvalue(other.ppvalue)
		{
			other.ppvalue = nullptr;
		}
		array_arena(void* i_pdata) : pvalue(i_pdata) {}
		array_arena& operator=(array_arena&& other) noexcept
		{
			ppvalue				= other.ppvalue;
			other.ppvalue = nullptr;
			return *this;
		}
		array_arena& operator=(array_arena const& other) noexcept
		{
			pvalue = other.pvalue;
			return *this;
		}
		explicit array_arena(address i_addr, size_type i_count) : ivalue(reinterpret_cast<std::uintptr_t>(i_addr) | 0x1)
		{
			*reinterpret_cast<size_type*>(i_addr) = i_count;
		}

		size_type length() const
		{
			return *reinterpret_cast<size_type*>(ivalue & ~0x1);
		}

		void set_length(size_type i_length)
		{
			*reinterpret_cast<size_type*>(ivalue & ~0x1) = i_length;
		}

		array_arena get_next() const
		{
			return *(reinterpret_cast<void**>(ivalue & ~0x1) + 1);
		}

		void set_next(array_arena next)
		{
			*(reinterpret_cast<void**>(ivalue & ~0x1) + 1) = next.value;
		}

		void clear_flag()
		{
			ivalue &= (~0x1);
		}

		std::uint8_t* get_value() const
		{
			return reinterpret_cast<std::uint8_t*>(ivalue & ~0x1);
		}
		void* update(size_type i_count)
		{
			return get_value() + i_count;
		}

		explicit operator bool() const
		{
			return ppvalue != nullptr;
		}
		union
		{
			address				 addr;
			void*					 pvalue;
			void**				 ppvalue;
			std::uint8_t*	 value;
			std::uintptr_t ivalue;
		};
	};

	struct solo_arena
	{
		solo_arena() : ppvalue(nullptr) {}
		solo_arena(solo_arena const&& other) : ppvalue(other.ppvalue) {}
		solo_arena(void* i_pdata) : pvalue(i_pdata) {}
		solo_arena(solo_arena&& other) noexcept : ppvalue(other.ppvalue)
		{
			other.ppvalue = nullptr;
		}
		solo_arena(solo_arena const& other) : pvalue(other.pvalue) {}
		solo_arena(array_arena const& other) : pvalue(other.get_value()) {}
		solo_arena& operator=(solo_arena&& other) noexcept
		{
			ppvalue				= other.ppvalue;
			other.ppvalue = nullptr;
			return *this;
		}
		solo_arena& operator=(array_arena const& other)
		{
			pvalue = other.get_value();
			return *this;
		}
		solo_arena& operator=(solo_arena const& other)
		{
			pvalue = other.pvalue;
			return *this;
		}

		void* get_value() const
		{
			return pvalue;
		}

		solo_arena get_next() const
		{
			return *(ppvalue);
		}
		void set_next(solo_arena next)
		{
			*(ppvalue) = next.value;
		}

		explicit operator bool() const
		{
			return ppvalue != nullptr;
		}
		union
		{
			address				 addr;
			void*					 pvalue;
			void**				 ppvalue;
			std::uint8_t*	 value;
			std::uintptr_t ivalue;
		};
	};

	struct arena_linker
	{
		arena_linker()																		 = default;
		explicit arena_linker(const arena_linker& i_other) = default;
		arena_linker(arena_linker&& i_other) : first(i_other.first)
		{
			i_other.first = nullptr;
		}

		enum : size_type
		{
			k_header_size = sizeof(void*)
		};

		void link_with(address arena, size_type size)
		{
			void** loc = reinterpret_cast<void**>(static_cast<std::uint8_t*>(arena) + size);
			*loc			 = first;
			first			 = loc;
		}

		template <typename lambda>
		void for_each(lambda&& i_deleter, size_type size)
		{
			size_type real_size = size + k_header_size;
			void*			it				= first;
			while (it)
			{
				void* next = *reinterpret_cast<void**>(it);
				i_deleter(reinterpret_cast<address>(reinterpret_cast<std::uint8_t*>(it) - size), real_size);
				it = next;
			}
		}

		explicit operator bool() const
		{
			return first != nullptr;
		}
		void* first = nullptr;
	};

public:
	~pool_allocator()
	{
		size_type size = k_atom_count * k_atom_size;
		linked_arenas.for_each(
		 [=](address i_value, size_type size_value)
		 {
			 underlying_allocator::deallocate(i_value, size_value);
		 },
		 size);
	}

private:
	address consume(size_type i_count)
	{
		size_type len;
		if (!arrays || (len = arrays.length()) < i_count)
		{
			allocate_arena();
			len = arrays.length();
		}

		ACL_ASSERT(len >= i_count);
		std::uint8_t* ptr				= arrays.get_value();
		std::uint8_t* head			= ptr + (i_count * k_atom_size);
		size_type			left_over = len - i_count;
		switch (left_over)
		{
		case 0:
			arrays = arrays.get_next();
			break;
		case 1:
		{
			solo_arena new_solo(head);
			arrays = arrays.get_next();
			new_solo.set_next(solo);
			solo = new_solo;
		}
		break;
		default:
		{
			// reorder arrays to sort them from big to small
			array_arena cur = arrays.get_next();
			array_arena save(head, left_over);
			if (cur && cur.length() > left_over)
			{
				arrays					 = cur;
				array_arena prev = save;
				while (true)
				{
					if (!cur || cur.length() <= left_over)
					{
						prev.set_next(save);
						save.set_next(cur);
						break;
					}
					prev = cur;
					cur	 = cur.get_next();
				}
			}
			else
			{
				save.set_next(cur);
				arrays = save;
			}
		}
		break;
		}

		return ptr;
	}

	address consume()
	{
		address ptr = solo.get_value();
		solo				= solo.get_next();
		return ptr;
	}

	void release(address i_only, size_type i_count)
	{
		array_arena new_arena(i_only, i_count);
		array_arena cur = arrays;
		if (cur.length() > i_count)
		{
			array_arena prev = new_arena;
			while (true)
			{
				if (!cur || cur.length() <= i_count)
				{
					prev.set_next(new_arena);
					new_arena.set_next(cur);
					break;
				}
				prev = cur;
				cur	 = cur.get_next();
			}
		}
		else
		{
			new_arena.set_next(cur);
			arrays = new_arena;
		}
	}

	void release(address i_only)
	{
		solo_arena arena(i_only);
		arena.set_next(std::move(solo));
		solo = arena;
	}

	void allocate_arena()
	{
		size_type		size			 = k_atom_count * k_atom_size;
		address			arena_data = underlying_allocator::allocate(size + arena_linker::k_header_size);
		array_arena new_arena(arena_data, k_atom_count);
		linked_arenas.link_with(arena_data, size);
		new_arena.set_next(arrays);
		arrays = new_arena;
		statistics::report_new_arena();
	}

	std::uint32_t get_total_free_count() const
	{
		std::uint32_t count		= 0;
		auto					a_first = arrays;
		while (a_first)
		{
			count += static_cast<std::uint32_t>(a_first.length());
			a_first = a_first.get_next();
		}

		auto s_first = solo;
		while (s_first)
		{
			count++;
			s_first = s_first.get_next();
		}
		return count;
	}

	std::uint32_t get_missing_atoms() const
	{
		if constexpr (detail::HasComputeStats<Options>)
			return this->statistics::padding_atoms_count();
		return 0;
	}

	std::uint32_t get_total_arena_count() const
	{
		std::uint32_t count = 0;
		arena_linker	a_first(linked_arenas);
		a_first.for_each(
		 [&](address i_value, size_type size_value)
		 {
			 count++;
		 },
		 k_atom_size * k_atom_count);
		return count;
	}

	array_arena			arrays;
	solo_arena			solo;
	const size_type k_atom_count;
	const size_type k_atom_size;
	arena_linker		linked_arenas;

public:
	template <typename record_ty>
	bool validate(record_ty const& records)
	{
		std::uint32_t rec_count = 0;
		for (auto& rec : records)
		{
			if (rec.count <= k_atom_count)
				rec_count += rec.count;
		}

		std::uint32_t arena_count = get_total_arena_count();
		if (rec_count + get_total_free_count() + get_missing_atoms() != arena_count * k_atom_count)
			return false;

		if (arena_count != this->statistics::get_arenas_allocated())
			return false;
		return true;
	}
};
} // namespace acl