
#pragma once

#include <acl/serializers/binary_input_serializer.hpp>
#include <acl/serializers/binary_output_serializer.hpp>

namespace acl
{

template <typename T>
concept Serializable = acl::OutputSerializable<T> && acl::InputSerializable<T>;
}