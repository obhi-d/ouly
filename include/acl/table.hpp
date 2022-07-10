
#include <acl/detail/table.hpp>

namespace acl
{

template <typename Type, template <typename T> typename VectorType = acl::detail::podvector_wrapper>
using table = detail::table<Type, VectorType>;

} // namespace acl