
#pragma once

#include <acl/utils/type_traits.hpp>
#include <memory>
#include <memory_resource>

namespace acl
{

namespace detail
{
template <typename T>
struct allocator_common
{
	using value_type			= T;
	using size_type				= std::size_t;
	using difference_type = std::ptrdiff_t;
	using reference				= value_type&;
	using const_reference = value_type const&;
	using pointer					= value_type*;
	using const_pointer		= value_type const*;

	using propagate_on_container_copy_assignment = std::true_type;
	using propagate_on_container_move_assignment = std::true_type;
	using propagate_on_container_swap						 = std::true_type;
};

} // namespace detail

template <typename T, typename UA>
struct allocator_wrapper : public detail::allocator_common<T>, public UA
{
	using typename detail::allocator_common<T>::value_type;
	using typename detail::allocator_common<T>::size_type;
	using typename detail::allocator_common<T>::difference_type;
	using typename detail::allocator_common<T>::reference;
	using typename detail::allocator_common<T>::const_reference;
	using typename detail::allocator_common<T>::pointer;
	using typename detail::allocator_common<T>::const_pointer;

	template <typename U>
	struct rebind
	{
		using other = allocator_wrapper<U, UA>;
	};

	allocator_wrapper() noexcept = default;
	template <typename U>
	allocator_wrapper(allocator_wrapper<U, UA> const& other) : UA((UA const&)other)
	{}
	template <typename U>
	allocator_wrapper(allocator_wrapper<U, UA>&& other) : UA(std::move((UA&)other))
	{}

	[[nodiscard]] inline pointer allocate(size_type cnt) const
	{
		pointer ret = reinterpret_cast<pointer>(UA::allocate(static_cast<size_type>(sizeof(T) * cnt), alignarg<T>));
		return ret;
	}

	inline void deallocate(pointer p, size_type cnt) const
	{
		UA::deallocate(p, static_cast<size_type>(sizeof(T) * cnt), alignarg<T>);
	}
};

template <typename T, typename UA>
struct allocator_ref : public detail::allocator_common<T>
{

	using typename detail::allocator_common<T>::value_type;
	using typename detail::allocator_common<T>::size_type;
	using typename detail::allocator_common<T>::difference_type;
	using typename detail::allocator_common<T>::reference;
	using typename detail::allocator_common<T>::const_reference;
	using typename detail::allocator_common<T>::pointer;
	using typename detail::allocator_common<T>::const_pointer;

	template <typename U>
	struct rebind
	{
		using other = allocator_ref<U, UA>;
	};

	allocator_ref() noexcept = default;
	allocator_ref(UA& ref) noexcept : ref_(&ref) {}
	template <typename U>
	allocator_ref(allocator_ref<U, UA>&& ref) noexcept : ref_(ref.ref_)
	{
		ref.ref_ = nullptr;
	}
	template <typename U>
	allocator_ref& operator=(allocator_ref<U, UA>&& ref) noexcept
	{
		ref_		 = ref.ref_;
		ref.ref_ = nullptr;
		return *this;
	}
	template <typename U>
	allocator_ref(allocator_ref<U, UA> const& ref) noexcept : ref_(ref.ref_)
	{}
	template <typename U>
	allocator_ref& operator=(allocator_ref<U, UA> const& ref) noexcept
	{
		ref_ = ref.ref_;
		return *this;
	}

	[[nodiscard]] inline pointer allocate(size_type cnt) const
	{
		ACL_ASSERT(ref_);
		pointer ret = reinterpret_cast<pointer>(ref_->allocate(static_cast<size_type>(sizeof(T) * cnt), alignarg<T>));
		return ret;
	}

	inline void deallocate(pointer p, size_type cnt) const
	{
		ACL_ASSERT(ref_);
		ref_->deallocate(p, static_cast<size_type>(sizeof(T) * cnt), alignarg<T>);
	}

	UA* ref_ = nullptr;
};

template <typename UA>
class memory_resource_ref : public std::pmr::memory_resource
{
public:
	memory_resource_ref(UA* impl) : impl_(impl) {}
	memory_resource_ref(memory_resource_ref&& other) noexcept : impl_(other.impl_)
	{
		other.impl_ = nullptr;
	}
	memory_resource_ref& operator=(memory_resource_ref&& other) noexcept
	{
		impl_				= other.impl_;
		other.impl_ = nullptr;
		return *this;
	}

	/**
	 * \thread_safe
	 */
	[[nodiscard]] inline void* do_allocate(std::size_t bytes, std::size_t alignment) override
	{
		return impl_->allocate(bytes, alignment);
	}
	/**
	 * \thread_safe
	 */
	inline void do_deallocate(void* ptr, std::size_t bytes, std::size_t alignment) override
	{
		return impl_->deallocate(ptr, bytes, alignment);
	}
	/**
	 * \thread_safe
	 */
	inline bool do_is_equal(const memory_resource& other) const noexcept override
	{
		// TODO
		auto pother = dynamic_cast<memory_resource_ref<UA> const*>(&other);
		if (!pother || pother->impl_ != impl_)
			return false;
		return true;
	}

private:
	UA* impl_;
};

template <typename UA>
class memory_resource : public std::pmr::memory_resource
{
public:
	template <typename... Args>
	memory_resource(Args&&... args) : impl_(std::forward<Args>(args)...)
	{}

	memory_resource(memory_resource&& r) noexcept : impl_(std::move(r.impl_)) {}
	memory_resource& operator=(memory_resource&& r) noexcept
	{
		impl_ = std::move(r.impl_);
		return *this;
	}
	/**
	 * \thread_safe
	 */
	[[nodiscard]] inline void* do_allocate(std::size_t bytes, std::size_t alignment) override
	{
		return impl_.allocate(bytes, alignment);
	}
	/**
	 * \thread_safe
	 */
	inline void do_deallocate(void* ptr, std::size_t bytes, std::size_t alignment) override
	{
		return impl_.deallocate(ptr, bytes, alignment);
	}
	/**
	 * \thread_safe
	 */
	inline bool do_is_equal(const memory_resource& other) const noexcept override
	{
		// TODO
		auto pother = dynamic_cast<memory_resource<UA> const*>(&other);
		if (!pother || &(pother->impl_) != &impl_)
			return false;
		return true;
	}

private:
	UA impl_;
};
} // namespace acl
