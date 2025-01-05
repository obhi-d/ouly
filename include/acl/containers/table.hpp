#pragma once
#include <acl/allocators/default_allocator.hpp>
#include <acl/containers/podvector.hpp>
#include <acl/utils/common.hpp>
#include <acl/utils/utils.hpp>
#include <type_traits>
#include <vector>

namespace acl
{

template <DefaultConstructible T, bool IsPOD = std::is_trivial_v<T>>
class table
{
	struct free_idx
	{
		std::uint32_t unused_ = detail::k_null_32;
		std::uint32_t valids_ = 0;
	};

	using vector	 = std::conditional_t<IsPOD, podvector<T>, acl::vector<T>>;
	using freepool = std::conditional_t<IsPOD, free_idx, podvector<std::uint32_t>>;

public:
	table() noexcept = default;
	table(vector pool, freepool free_pool) : pool_(std::move(pool)), free_pool_(std::move(free_pool)) {}
	table(table const&)			= default;
	table(table&&) noexcept = default;
	~table() noexcept				= default;

	auto operator=(table const&) -> table&		 = default;
	auto operator=(table&&) noexcept -> table& = default;

	template <typename... Args>
	auto emplace(Args&&... args) -> std::uint32_t
	{
		std::uint32_t index = 0;
		if constexpr (IsPOD)
		{
			if (free_pool_.unused != detail::k_null_32)
			{
				index = free_pool_.unused;
				// NOLINTNEXTLINE
				free_pool_.unused = reinterpret_cast<std::uint32_t&>(pool_[free_pool_.unused]);
			}
			else
			{
				index = static_cast<std::uint32_t>(pool_.size());
				pool_.resize(index + 1);
			}
			pool_[index] = T(std::forward<Args>(args)...);
			free_pool_.valids++;
		}
		else
		{
			if (!free_pool_.empty())
			{
				index = free_pool_.back();
				free_pool_.pop_back();
				pool_[index] = std::move(T(std::forward<Args>(args)...));
			}
			else
			{
				index = static_cast<std::uint32_t>(pool_.size());
				pool_.emplace_back(std::forward<Args>(args)...);
			}
		}
		return index;
	}

	void erase(std::uint32_t index)
	{
		if constexpr (IsPOD)
		{
			// NOLINTNEXTLINE
			reinterpret_cast<std::uint32_t&>(pool_[index]) = free_pool_.unused;
			free_pool_.unused															 = index;
			free_pool_.valids--;
		}
		else
		{
			pool_[index] = T();
			free_pool_.emplace_back(index);
		}
	}

	auto operator[](std::uint32_t i) -> T&
	{
		// NOLINTNEXTLINE
		return reinterpret_cast<T&>(pool_[i]);
	}

	auto operator[](std::uint32_t i) const -> T const&
	{
		// NOLINTNEXTLINE
		return reinterpret_cast<T const&>(pool_[i]);
	}

	auto at(std::uint32_t i) -> T&
	{
		// NOLINTNEXTLINE
		return reinterpret_cast<T&>(pool_[i]);
	}

	auto at(std::uint32_t i) const -> T const&
	{
		// NOLINTNEXTLINE
		return reinterpret_cast<T const&>(pool_[i]);
	}

	[[nodiscard]] auto size() const -> std::uint32_t
	{
		if constexpr (IsPOD)
		{
			return free_pool_.valids;
		}
		else
		{
			return static_cast<std::uint32_t>(pool_.size() - free_pool_.size());
		}
	}

	[[nodiscard]] auto capacity() const -> uint32_t
	{
		return static_cast<uint32_t>(pool_.size());
	}

private:
	vector	 pool_;
	freepool free_pool_;
};

} // namespace acl
