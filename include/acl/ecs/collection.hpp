
#pragma once

#include <acl/allocators/default_allocator.hpp>
#include <acl/containers/podvector.hpp>
#include <acl/ecs/entity.hpp>
#include <acl/utils/config.hpp>
#include <acl/utils/type_traits.hpp>
#include <acl/utils/utils.hpp>
#include <limits>

namespace acl::ecs
{

/**
 * @brief A collection of links, but not stored in vectors, instead managed by bitmap
 * @tparam Options options controlling behaviour of the container
 */
template <typename EntityTy, typename Options = acl::default_options<EntityTy>>
class collection : public detail::custom_allocator_t<Options>
{
public:
	using size_type			 = detail::choose_size_t<uint32_t, Options>;
	using entity_type		 = EntityTy;
	using allocator_type = detail::custom_allocator_t<Options>;

private:
	static constexpr auto pool_mul		 = detail::log2(detail::pool_size_v<Options>);
	static constexpr auto pool_size		 = static_cast<size_type>(1) << pool_mul;
	static constexpr auto pool_mod		 = pool_size - 1;
	using this_type										 = collection<EntityTy, Options>;
	using base_type										 = allocator_type;
	using storage											 = uint8_t;
	static constexpr bool has_revision = std::same_as<typename EntityTy::revision_type, uint8_t> && acl::detail::debug;

public:
	collection() noexcept = default;
	collection(allocator_type&& alloc) noexcept : base_type(std::move<allocator_type>(alloc)) {}
	collection(allocator_type const& alloc) noexcept : base_type(alloc) {}
	collection(collection&& other) noexcept = default;
	collection(collection const& other) noexcept
	{
		*this = other;
	}

	~collection() noexcept
	{
		clear();
		shrink_to_fit();
	}

	auto operator=(collection&& other) noexcept -> collection& = default;

	auto operator=(collection const& other) noexcept -> collection&
	{
		constexpr auto bit_page_size = sizeof(storage) * (pool_size >> 3);
		constexpr auto haz_page_size = sizeof(storage) * pool_size;

		items_.resize(other.items_.size());
		if constexpr (has_revision)
		{
			for (std::size_t i = 0, end = items_.size() / 2; i < end; ++i)
			{
				items_[(i * 2) + 0] = acl::allocate<storage>(*this, bit_page_size);
				items_[(i * 2) + 1] = acl::allocate<storage>(*this, haz_page_size);
				std::memcpy(items_[(i * 2) + 0], other.items_[(i * 2) + 0], bit_page_size);
				std::memcpy(items_[(i * 2) + 1], other.items_[(i * 2) + 1], haz_page_size);
			}
		}
		else
		{
			for (std::size_t i = 0, end = items_.size(); i < end; ++i)
			{
				items_[i] = acl::allocate<storage>(*this, bit_page_size);
				std::memcpy(items_[i], other.items_[i], bit_page_size);
			}
		}

		length_ = other.length_;
		return *this;
	}

	template <typename Cont, typename Lambda>
	void for_each(Cont& cont, Lambda&& lambda) noexcept
	{
		for_each_l<Cont, Lambda>(cont, 0, range(), std::forward<Lambda>(lambda));
	}
	template <typename Cont, typename Lambda>
	void for_each(Cont const& cont, Lambda&& lambda) const noexcept
	{
		// NOLINTNEXTLINE
		const_cast<this_type*>(this)->for_each_l(cont, 0, range(), std::forward<Lambda>(lambda));
	}

	template <typename Cont, typename Lambda>
	void for_each(Cont& cont, size_type first, size_type last, Lambda&& lambda) noexcept
	{
		for_each_l<Cont, Lambda>(cont, first, last, std::forward<Lambda>(lambda));
	}

	template <typename Cont, typename Lambda>
	void for_each(Cont const& cont, size_type first, size_type last, Lambda&& lambda) const noexcept
	{
		// NOLINTNEXTLINE
		const_cast<this_type*>(this)->for_each_l(cont, first, last, std::forward<Lambda>(lambda));
	}

	void emplace(entity_type l) noexcept
	{
		auto idx = l.get();
		max_lnk_ = std::max(idx, max_lnk_);
		set_bit(idx);
		if constexpr (has_revision)
		{
			set_hazard(idx, static_cast<uint8_t>(l.revision()));
		}
		length_++;
	}

	void erase(entity_type l) noexcept
	{
		auto idx = l.get();
		if constexpr (has_revision)
		{
			validate_hazard(idx, static_cast<uint8_t>(l.revision()));
		}
		unset_bit(idx);
		length_--;
	}

	auto contains(entity_type l) const noexcept -> bool
	{
		return is_bit_set(l.get());
	}

	auto size() const noexcept -> size_type
	{
		return length_;
	}

	auto capacity() const noexcept -> size_type
	{
		return static_cast<size_type>(items_.size()) * pool_size;
	}

	auto range() const noexcept -> size_type
	{
		return max_lnk_ + 1;
	}

	void shrink_to_fit() noexcept
	{
		if (!length_)
		{
			constexpr auto bit_page_size = sizeof(storage) * (pool_size >> 3);
			constexpr auto haz_page_size = sizeof(storage) * pool_size;

			if constexpr (has_revision)
			{
				for (size_type i = 0, end = static_cast<size_type>(items_.size()) / 2; i < end; ++i)
				{
					acl::deallocate(static_cast<allocator_type&>(*this), items_[(i * 2) + 0], bit_page_size);
					acl::deallocate(static_cast<allocator_type&>(*this), items_[(i * 2) + 1], haz_page_size);
				}
			}
			else
			{
				for (auto i : items_)
				{
					acl::deallocate(static_cast<allocator_type&>(*this), i, bit_page_size);
				}
			}
		}
	}

	void clear() noexcept
	{
		length_	 = 0;
		max_lnk_ = 0;
	}

private:
	void validate_hazard(size_type nb, std::uint8_t hz) const noexcept
	{
		auto block = hazard_page(nb >> pool_mul);
		auto index = nb & pool_mod;

		assert(items_[block][index] == hz);
	}

	auto bit_page(size_type p) const noexcept -> size_type
	{
		if constexpr (has_revision)
		{
			return p * 2;
		}
		else
		{
			return p;
		}
	}

	auto hazard_page(size_type p) const noexcept -> size_type
	{
		if constexpr (has_revision)
		{
			return (p * 2) + 1;
		}
		else
		{
			return std::numeric_limits<uint32_t>::max();
		}
	}

	auto is_bit_set(size_type nb) const noexcept -> bool
	{
		auto									 block = bit_page(nb >> pool_mul);
		auto									 index = nb & pool_mod;
		constexpr std::uint8_t one	 = 1;
		constexpr uint8_t			 mask	 = 0x7;

		return (block < items_.size()) && (items_[block][index >> 3] & (one << static_cast<std::uint8_t>(index & mask)));
	}

	void unset_bit(size_type nb) noexcept
	{
		auto							block = bit_page(nb >> pool_mul);
		auto							index = nb & pool_mod;
		constexpr uint8_t mask	= 0x7;

		constexpr std::uint8_t one = 1;
		items_[block][index >> 3] &= ~(one << static_cast<std::uint8_t>(index & mask));
	}

	void set_bit(size_type nb) noexcept
	{
		auto block = bit_page(nb >> pool_mul);
		auto index = nb & pool_mod;

		if (block >= items_.size())
		{
			constexpr auto bit_page_size = sizeof(storage) * (pool_size >> 3);
			constexpr auto haz_page_size = sizeof(storage) * pool_size;

			items_.emplace_back(acl::allocate<storage>(*this, bit_page_size));
			std::memset(items_.back(), 0, bit_page_size);
			if constexpr (has_revision)
			{
				items_.emplace_back(acl::allocate<storage>(*this, haz_page_size));
				std::memset(items_.back(), 0, haz_page_size);
			}
		}

		constexpr std::uint8_t one	= 1;
		constexpr uint8_t			 mask = 0x7;
		items_[block][index >> 3] |= one << static_cast<std::uint8_t>(index & mask);
	}

	void set_hazard(size_type nb, std::uint8_t hz) noexcept
	{
		auto block					 = hazard_page(nb >> pool_mul);
		auto index					 = nb & pool_mod;
		items_[block][index] = hz;
	}

	auto get_hazard(size_type nb) noexcept -> std::uint8_t
	{
		auto block = hazard_page(nb >> pool_mul);
		auto index = nb & pool_mod;
		return items_[block][index];
	}

	template <typename ContT, typename Lambda>
	void for_each_l(ContT& cont, size_type first, size_type last, Lambda lambda) noexcept
	{
		for (; first != last; ++first)
		{
			if (is_bit_set(first))
			{
				entity_type l = has_revision ? entity_type(first, get_hazard(first)) : entity_type(first);
				lambda(l, cont.at(l));
			}
		}
	}

	podvector<storage*, allocator_type> items_;
	size_type														length_	 = 0;
	size_type														max_lnk_ = 0;
};

} // namespace acl::ecs
