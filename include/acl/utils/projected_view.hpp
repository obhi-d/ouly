
#include <acl/utils/type_traits.hpp>
#include <compare>
#include <ranges>

namespace acl
{

template <auto M, typename C = typename acl::opt::member<M>::class_type>
class projected_view : public std::ranges::view_interface<projected_view<M, C>>
{
public:
	static constexpr bool is_const = std::is_const_v<C>;

public:
	struct iterator_wrapper
	{

	public:
		iterator_wrapper() noexcept = default;
		iterator_wrapper(C* d) noexcept : data(d) {} // Concept requirements
		using iterator_category = std::random_access_iterator_tag;
		using value_type				= std::remove_reference_t<decltype(std::declval<C&>().*M)>;
		using difference_type		= std::ptrdiff_t;

		// Member functions (stubs)
		value_type& operator*() const& noexcept
		{
			return data->*M;
		}

		value_type* operator->() const& noexcept
		{
			return &(data->*M);
		};

		iterator_wrapper& operator++() & noexcept
		{
			++data;
			return *this;
		};

		iterator_wrapper operator++(int) & noexcept
		{
			return iterator_wrapper(data++);
		};

		iterator_wrapper& operator--() & noexcept
		{
			--data;
			return *this;
		}

		iterator_wrapper operator--(int) & noexcept
		{
			return iterator_wrapper(data--);
		}

		iterator_wrapper& operator+=(difference_type n) & noexcept
		{
			data += n;
			return *this;
		}

		iterator_wrapper operator+(difference_type n) const& noexcept
		{
			return iterator_wrapper(data + n);
		}

		friend iterator_wrapper operator+(difference_type n, iterator_wrapper other) noexcept
		{
			return iterator_wrapper(other.data + n);
		}

		iterator_wrapper& operator-=(difference_type n) & noexcept
		{
			data -= n;
			return *this;
		}

		iterator_wrapper operator-(difference_type n) const& noexcept
		{
			return iterator_wrapper(data - n);
		}

		difference_type operator-(iterator_wrapper other) const& noexcept
		{
			return static_cast<difference_type>(data - other.data);
		}

		auto operator<=>(iterator_wrapper const& other) const& noexcept = default;

		value_type& operator[](difference_type n) const& noexcept
		{
			return data[n].*M;
		}

	private:
		C* data = nullptr;
	};

	auto begin() const noexcept
	{
		return iterator_wrapper(begin_);
	}

	auto end() const noexcept
	{
		return iterator_wrapper(end_);
	}

	projected_view(C* first, size_t count) noexcept : begin_(first), end_(first + count) {}
	projected_view() noexcept = default;

private:
	C* begin_ = nullptr;
	C* end_		= nullptr;
};

template <auto M>
using projected_cview = projected_view<M, typename acl::opt::member<M>::class_type const>;

} // namespace acl
