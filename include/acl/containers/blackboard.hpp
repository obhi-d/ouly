#pragma once
#include "podvector.hpp"
#include <acl/allocators/allocator.hpp>
#include <acl/allocators/default_allocator.hpp>
#include <acl/utils/config.hpp>
#include <acl/utils/utils.hpp>
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

namespace acl
{

struct blackboard_offset
{
	using dtor = void (*)(void*);

	void* data_				= nullptr;
	dtor	destructor_ = nullptr;
};

template <typename T>
concept BlackboardHashMap = requires(T& obj, T const& cobj) {
	typename T::key_type;
	requires std::same_as<typename T::mapped_type, blackboard_offset>;
	typename T::iterator;
	typename T::const_iterator;
	{ obj.find(std::declval<typename T::key_type>()) } -> std::same_as<typename T::iterator>;
	{ cobj.find(std::declval<typename T::key_type>()) } -> std::same_as<typename T::const_iterator>;

	obj.erase(obj.find(std::declval<typename T::key_type>()));
	{ obj[std::declval<typename T::key_type>()] } -> std::same_as<typename T::mapped_type&>;
	requires std::same_as<typename T::mapped_type, blackboard_offset>;
};

namespace detail
{

template <typename T>
concept HashMapDeclTraits = requires {
	typename T::name_map_type;
	requires BlackboardHashMap<typename T::name_map_type>;
};

template <typename H>
struct name_index_map
{
	using type = std::unordered_map<std::string, blackboard_offset>;
};

template <HashMapDeclTraits H>
struct name_index_map<H>
{
	using type = typename H::name_map_type;
};
} // namespace detail

namespace opt
{
/**
 * Provide a custom blackboard hash map lookup implementation.
 */
template <BlackboardHashMap H>
struct name_val_map
{
	using name_map_type = H;
};

/**
 * This option allows blackboard to customize the <type key> by using std::type_index, you are free to provide
 * the hash map implementation and replace std::unordered_map by your own class if it satisfies
 *
 */
template <template <typename K, typename V> typename H>
struct map
{
	using name_map_type = H<std::type_index, blackboard_offset>;
};

/**
 * This option allows you to implement your own key type, the offset is always required to be blackboard_offset type and
 * provided by blackboard.
 */
template <template <typename V> typename H>
struct name_map
{
	using name_map_type = H<blackboard_offset>;
};

} // namespace opt

/**
 * @brief Store data as name value pairs, value can be any blob of data, has no free memory management
 *
 * @remark Data is stored as a blob, names are stored seperately if required for lookup
 *         Data can also be retrieved by index.
 *         There is no restriction on the data type that is supported (POD or non-POD both are supported).
 */
template <typename Options = acl::options<>>
class blackboard : public detail::custom_allocator_t<Options>
{
	using dtor = bool (*)(void*);

	using options															= Options;
	static constexpr auto total_atoms_in_page = detail::pool_size_v<options>;
	using allocator														= detail::custom_allocator_t<options>;
	using base_type														= allocator;
	using name_index_map											= typename detail::name_index_map<options>::type;

public:
	using link					 = void*;
	using clink					 = void const*;
	using key_type			 = typename name_index_map::key_type;
	using iterator			 = typename name_index_map::iterator;
	using const_iterator = typename name_index_map::const_iterator;

	static constexpr bool is_type_indexed = std::is_same_v<key_type, std::type_index>;

	blackboard() noexcept = default;
	blackboard(allocator&& alloc) noexcept : base_type(std::move<allocator>(alloc)) {}
	blackboard(allocator const& alloc) noexcept : base_type(alloc) {}
	blackboard(blackboard&& other) noexcept													= default;
	blackboard(blackboard const& other) noexcept										= delete;
	auto operator=(blackboard&& other) noexcept -> blackboard&			= default;
	auto operator=(blackboard const& other) noexcept -> blackboard& = delete;
	~blackboard()
	{
		clear();
	}

	void clear()
	{
		std::for_each(lookup_.begin(), lookup_.end(),
									[this](auto& el)
									{
										if (el.second.destructor)
										{
											el.second.destructor(el.second.data);
											el.second.destructor = nullptr;
										}
									});
		auto h = head_;
		while (h)
		{
			auto n = h->pnext;
			acl::deallocate(*this, h, h->size + sizeof(arena));
			h = n;
		}
		lookup_.clear();
		head_		 = nullptr;
		current_ = nullptr;
	}

	template <typename T>
	auto get() const noexcept -> T const&
		requires(is_type_indexed)
	{
		return get<T>(std::type_index(typeid(T)));
	}

	template <typename T>
	auto get() noexcept -> T& requires(is_type_indexed) { return get<T>(std::type_index(typeid(T))); }

	template <typename T>
	auto get(key_type v) const noexcept -> T const&
	{
		// NOLINTNEXTLINE
		return const_cast<blackboard&>(*this).get<T>(v);
	}

	template <typename T>
	auto get(key_type k) noexcept -> T&
	{
		auto it = lookup_.find(k);
		assert(it != lookup_.end());
		// NOLINTNEXTLINE
		return *reinterpret_cast<T*>(it->second.data);
	}

	template <typename T>
	auto get_if() const noexcept -> T const* requires(is_type_indexed) { return get_if<T>(std::type_index(typeid(T))); }

	template <typename T>
	auto get_if() noexcept -> T* requires(is_type_indexed) { return get_if<T>(std::type_index(typeid(T))); }

	template <typename T>
	auto get_if(key_type v) const noexcept -> T const*
	{
		// NOLINTNEXTLINE
		return const_cast<blackboard&>(*this).get_if<T>(v);
	}

	template <typename T>
	auto get_if(key_type k) noexcept -> T*
	{
		auto it = lookup_.find(k);
		if (it == lookup_.end())
		{
			return nullptr;
		}
		// NOLINTNEXTLINE
		return reinterpret_cast<T*>(it->second.data);
	}

	/**
	 *
	 */
	template <typename T, typename... Args>
	auto emplace(Args&&... args) noexcept
	 -> T& requires(is_type_indexed) { return emplace<T>(std::type_index(typeid(T)), std::forward<Args>(args)...); }

	template <typename T, typename... Args>
	auto emplace(key_type k, Args&&... args) noexcept -> T&
	{
		auto& lookup_ent = lookup_[k];
		if (lookup_ent.destructor && lookup_ent.data)
		{
			lookup_ent.destructor(lookup_ent.data);
		}
		if (!lookup_ent.data)
		{
			lookup_ent.data = allocate_space(sizeof(T), alignof(T));
		}

		// NOLINTNEXTLINE
		std::construct_at(reinterpret_cast<T*>(lookup_ent.data), std::forward<Args>(args)...);
		lookup_ent.destructor = std::is_trivially_destructible_v<T> ? &do_nothing : &destroy_at<T>;
		// NOLINTNEXTLINE
		return *reinterpret_cast<T*>(lookup_ent.data);
	}

	template <typename T>
	void erase() noexcept
		requires(is_type_indexed)
	{
		erase(std::type_index(typeid(T)));
	}

	void erase(key_type index) noexcept
	{
		auto it = lookup_.find(index);
		if (it != lookup_.end())
		{
			auto& lookup_ent = it->second;
			if (lookup_ent.destructor && lookup_ent.data)
			{
				lookup_ent.destructor(lookup_ent.data);
			}
			lookup_ent.destructor = nullptr;
		}
	}

	template <typename T>
	void contains() const noexcept
		requires(is_type_indexed)
	{
		contains(std::type_index(typeid(T)));
	}

	auto contains(key_type index) const noexcept -> bool
	{
		auto it = lookup_.find(index);
		return it != lookup_.end() && it->second.destructor != nullptr;
	}

private:
	static void do_nothing(void* /*unused*/) {}

	struct arena
	{
		arena*	 pnext_			= nullptr;
		uint32_t size_			= 0;
		uint32_t remaining_ = 0;
	};

	template <typename T>
	static void destroy_at(void* s)
	{
		// NOLINTNEXTLINE
		reinterpret_cast<T*>(s)->~T();
	}

	auto allocate_space(size_t size, size_t alignment) -> void*
	{
		auto req = static_cast<uint32_t>(size + (alignment - 1));
		if ((current_ == nullptr) || current_->remaining_ < req)
		{
			uint32_t page_size	 = std::max<uint32_t>(total_atoms_in_page, req);
			auto		 new_current = acl::allocate<arena>(*this, sizeof(arena) + page_size);
			if (current_)
			{
				current_->pnext_ = new_current;
			}
			new_current->pnext		 = nullptr;
			new_current->size			 = page_size;
			new_current->remaining = (page_size - req);
			current_							 = new_current;
			if (head_ == nullptr)
			{
				head_ = current_;
			}
			return acl::align(new_current + 1, alignment);
		}
		// NOLINTNEXTLINE
		void* ptr = reinterpret_cast<std::byte*>(current_ + 1) + (current_->size_ - current_->remaining_);
		current_->remaining_ -= req;
		return acl::align(ptr, alignment);
	}

	arena*				 head_		= nullptr;
	arena*				 current_ = nullptr;
	name_index_map lookup_;
};
} // namespace acl
