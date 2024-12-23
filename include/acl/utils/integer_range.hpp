
#pragma once

namespace acl
{

template <typename I>
class integer_range
{
public:
	using difference_type						= I;
	inline integer_range() noexcept = default;
	inline integer_range(I vbegin, I vend) noexcept : begin_(vbegin), end_(vend) {}

	inline void begin(I v) noexcept
	{
		begin_ = v;
	}

	inline void end(I v) noexcept
	{
		end_ = v;
	}

	inline I begin() const noexcept
	{
		return begin_;
	}

	inline I end() const noexcept
	{
		return end_;
	}

	inline I size() const noexcept
	{
		return end_ - begin_;
	}

	inline bool empty() const noexcept
	{
		return begin_ == end_;
	}

private:
	I begin_ = {};
	I end_	 = {};
};

} // namespace acl
