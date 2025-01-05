#pragma once

namespace acl::opt
{
struct bsearch_min0
{
	static constexpr int bsearch_algo = 0;
};
struct bsearch_min1
{
	static constexpr int bsearch_algo = 1;
};
struct bsearch_min2
{
	static constexpr int bsearch_algo = 2;
};
} // namespace acl::opt

namespace acl::detail
{
template <typename O>
concept HasBsearchAlgo = requires { O::bsearch_algo; };
} // namespace acl::detail
