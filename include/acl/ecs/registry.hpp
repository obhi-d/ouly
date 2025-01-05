
#pragma once

#include <acl/ecs/entity.hpp>
#include <algorithm>
#include <atomic>
#include <cassert>
#include <span>
#include <vector>

namespace acl::ecs
{

namespace detail
{
template <typename RevType>
class revision_table
{
protected:
	std::vector<RevType> revisions_;
};

template <>
class revision_table<void>
{};

template <typename S>
class counter
{
public:
	auto fetch_sub(S /*unused*/) -> S
	{
		return value_--;
	}

	auto fetch_add(S /*unused*/) -> S
	{
		return value_++;
	}

	auto load() const noexcept -> S
	{
		return value_;
	}

	void store(S v, std::memory_order /*unused*/)
	{
		value_ = v;
	}

	S value_;
};

} // namespace detail

/**
 * @brief This class stores a list of reusable links, and allows for vector allocating objects on a seperate container
 * based on the links. This class also stores link revisions that are updated when links are deleted, and allows for
 * verification if links are still alive.
 * @tparam Ty
 * @tparam SizeType
 */
template <typename EntityTy, template <typename S> class CounterType = detail::counter>
class basic_registry : detail::revision_table<typename EntityTy::revision_type>
{
	using base = detail::revision_table<typename EntityTy::revision_type>;

public:
	using size_type			= typename EntityTy::size_type;
	using ssize_type		= std::make_signed_t<size_type>;
	using revision_type = typename EntityTy::revision_type;
	using type					= EntityTy;

	/** @brief This can possibly be thread safe. */
	auto emplace() -> type
	{
		auto i = free_slot_.fetch_sub(1);
		if (i > 0)
		{
			return type(free_[i - 1]);
		}

		return type(max_size_.fetch_add(1));
	}

	/** @brief This is not thread safe */
	void erase(type l)
	{
		auto count = free_slot_.load();
		if (count > 0)
		{
			free_.resize(static_cast<size_t>(count));
			free_.emplace_back(l.revised());
		}
		else
		{
			free_.clear();
			free_.emplace_back(l.revised());
		}

		free_slot_.store((ssize_type)free_.size(), std::memory_order_relaxed);

		if constexpr (!std::is_same_v<revision_type, void>)
		{
			auto idx = l.get();
			if (idx >= base::revisions_.size())
			{
				base::revisions_.resize(idx + 1);
			}
			assert(l.revision() == base::revisions_[idx]);
			base::revisions_[idx]++;
		}

		sorted_ = false;
	}

	/** @brief This is not thread safe */
	void erase(std::span<type const> ls)
	{
		auto count = free_slot_.load();
		if (count > 0)
		{
			free_.resize(static_cast<size_t>(count));
			free_.reserve(static_cast<size_t>(count) + ls.size());
			for (auto const& l : ls)
			{
				free_.emplace_back(l.revised());
			}
		}
		else
		{
			free_.clear();
			free_.reserve(ls.size());
			for (auto const& l : ls)
			{
				free_.emplace_back(l.revised());
			}
		}

		free_slot_.store((ssize_type)free_.size(), std::memory_order_relaxed);

		if constexpr (!std::is_same_v<revision_type, void>)
		{
			for (auto const& l : ls)
			{
				auto idx = l.get();
				if (idx >= base::revisions_.size())
				{
					base::revisions_.resize(idx + 1);
				}
				assert(l.revision() == base::revisions_[idx]);
				base::revisions_[idx]++;
			}
		}

		sorted_ = false;
	}

	auto is_valid(type l) const noexcept -> bool
		requires(!std::is_same_v<revision_type, void>)
	{
		return static_cast<revision_type>(l.revision()) == base::revisions_[l.get()];
	}

	auto get_revision(type l) const noexcept
		requires(!std::is_same_v<revision_type, void>)
	{
		return get_revision(l.get());
	}

	auto get_revision(size_type l) const noexcept
		requires(!std::is_same_v<revision_type, void>)
	{
		return l < base::revisions_.size() ? base::revisions_[l] : 0;
	}

	template <typename Lambda>
	void for_each_index(Lambda&& l)
	{
		if (!sorted_)
		{
			sort_free();
		}
		for_each_(std::forward<Lambda>(l), free_, max_size_.load());
	}

	template <typename Lambda>
	void for_each_index(Lambda&& l) const
	{
		if (sorted_)
		{
			for_each_(std::forward<Lambda>(l), free_, max_size_.load());
		}
		else
		{
			auto copy = free_;
			std::ranges::sort(copy,
												[](size_type first, size_type second)
												{
													return type(first).get() < type(second).get();
												});
			for_each_(std::forward<Lambda>(l), copy, max_size_.load());
		}
	}

	[[nodiscard]] auto max_size() const -> uint32_t
	{
		return max_size_.load();
	}

	void sort_free()
	{
		std::ranges::sort(free_,
											[](size_type first, size_type second)
											{
												return type(first).get() < type(second).get();
											});
		sorted_ = true;
	}

	/**
	 * @brief This function is not threadsafe
	 */
	void shrink() noexcept
	{
		free_.resize(free_slot_.load());
	}

private:
	template <typename Lambda>
	static void for_each(Lambda lambda, auto const& copy, size_type max_size)
	{
		for (size_type i = 1, fi = 0; i < max_size; ++i)
		{
			if (fi < copy.size() && type(copy[fi]).get() == i)
			{
				fi++;
			}
			else
			{
				lambda(i);
			}
		}
	}

	std::vector<size_type>	free_;
	CounterType<size_type>	max_size_	 = {1};
	CounterType<ssize_type> free_slot_ = {0};
	bool										sorted_		 = false;
};

template <typename T = std::true_type>
using registry = basic_registry<entity<T>>;

template <typename T = std::true_type>
using rxregistry = basic_registry<rxentity<T>>;

} // namespace acl::ecs
