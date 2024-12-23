#pragma once

#include <acl/allocators/allocator.hpp>
#include <acl/allocators/default_allocator.hpp>
#include <acl/utils/utils.hpp>
#include <cstddef>
#include <optional>

namespace acl
{

template <typename Ty, typename Options = acl::default_options<Ty>>
class basic_queue : public detail::custom_allocator_t<Options>
{
public:
	using value_type		 = Ty;
	using size_type			 = detail::choose_size_t<uint32_t, Options>;
	using allocator_type = detail::custom_allocator_t<Options>;

private:
	static constexpr auto pool_mul	= detail::log2(detail::pool_size_v<Options>);
	static constexpr auto pool_size = static_cast<size_type>(1) << pool_mul;
	static constexpr auto pool_mod	= pool_size - 1;
	static constexpr bool has_pod		= detail::HasTrivialAttrib<Options>;

	static constexpr size_t alignment = std::max(alignof(std::max_align_t), alignof(Ty));
	using storage											= detail::aligned_storage<sizeof(value_type), alignment>;

	struct deque_block
	{
		alignas(alignment) std::array<storage, pool_size> data;
		deque_block* next = nullptr;
	};

public:
	inline basic_queue() noexcept = default;

	inline basic_queue(basic_queue const& other)
		requires(std::is_copy_constructible_v<Ty>)
	{
		copy(other);
	}

	inline basic_queue(basic_queue&& other) noexcept
			: head_(other.head_), tail_(other.tail_), free_(other.free_), front_(other.front_), back_(other.back_)
	{
		other.head_	 = nullptr;
		other.tail_	 = nullptr;
		other.free_	 = nullptr;
		other.front_ = 0;
		other.back_	 = 0;
	}

	inline basic_queue& operator=(basic_queue const& other)
		requires(std::is_copy_constructible_v<Ty>)
	{
		copy(other);
		return *this;
	}

	inline basic_queue& operator=(basic_queue&& other) noexcept
	{
		clear();
		free_chain(free_);

		head_				 = other.head_;
		tail_				 = other.tail_;
		free_				 = other.free_;
		front_			 = other.front_;
		back_				 = other.back_;
		other.head_	 = nullptr;
		other.tail_	 = nullptr;
		other.free_	 = nullptr;
		other.front_ = 0;
		other.back_	 = 0;

		return *this;
	}

	~basic_queue()
	{
		clear();
		free_chain(free_);
	}

	template <typename... Args>
	inline auto& emplace_back(Args&&... args) noexcept
	{
		if (back_ == pool_size || !tail_)
		{
			add_tail();
			back_ = 0;
		}
		auto ptr = tail_->data[back_++].template as<Ty>();
		std::construct_at(ptr, std::forward<Args>(args)...);
		return *ptr;
	}

	inline Ty pop_front()
	{
		if (empty())
			throw std::runtime_error("Deque is empty");

		return pop_front_unsafe();
	}

	inline Ty pop_front_unsafe() noexcept
	{
		ACL_ASSERT(!empty());

		Ty ret = std::move(*head_->data[front_].template as<Ty>());

		if constexpr (!std::is_trivially_destructible_v<Ty>)
			std::destroy_at(head_->data[front_].template as<Ty>());

		if (++front_ == pool_size)
		{
			remove_head();
			front_ = 0;
		}

		return ret;
	}

	inline bool empty() const noexcept
	{
		return (head_ == tail_ && front_ == back_);
	}

	void clear() noexcept
	{
		if constexpr (!std::is_trivially_destructible_v<Ty>)
			for_each(
			 [](Ty& v)
			 {
				 std::destroy_at(&v);
			 });

		if (head_)
			head_->next = free_;
		free_ = head_;
		head_ = tail_ = 0;
		front_ = back_ = 0;
	}

	template <typename L>
	void for_each(L&& l)
	{
		_for_each<Ty>(*this, std::forward<L>(l));
	}

	template <typename L>
	void for_each(L&& l) const
	{
		_for_each<Ty const>(*this, std::forward<L>(l));
	}

private:
	template <typename V, typename T, typename L>
	static void _for_each(T& self, L&& l)
	{
		auto block = self.head_;
		auto start = self.front_;
		while (block)
		{
			auto end = block == self.tail_ ? self.back_ : pool_size;
			for (; start < end; ++start)
			{
				l(*block->data[start].template as<V>());
			}
			start = 0;
			block = block->next;
		}
	}

	void copy(basic_queue const& src)
		requires(std::is_copy_constructible_v<Ty>)
	{
		clear();
		src.for_each(
		 [this](Ty const& v)
		 {
			 emplace_back(v);
		 });
	}

	void add_tail()
	{
		deque_block* db = free_;
		if (free_)
			free_ = free_->next;
		else
		{
			db			 = acl::allocate<deque_block>(static_cast<allocator_type&>(*this), sizeof(deque_block), alignarg<Ty>);
			db->next = nullptr;
		}

		if (tail_)
			tail_->next = db;
		else
			head_ = db;
		tail_ = db;
	}

	void remove_head()
	{
		auto h	= head_;
		head_		= head_->next;
		h->next = free_;
		free_		= h;
		if (!head_)
		{
			tail_	 = nullptr;
			front_ = 0;
			back_	 = 0;
		}
	}

	void free_chain(deque_block* start)
	{
		while (start)
		{
			auto next = start->next;
			acl::deallocate(static_cast<allocator_type&>(*this), start, sizeof(deque_block), alignarg<Ty>);
			start = next;
		}
	}

	deque_block* head_	= nullptr;
	deque_block* tail_	= nullptr;
	deque_block* free_	= nullptr;
	size_type		 front_ = 0;
	size_type		 back_	= 0;
};
} // namespace acl
