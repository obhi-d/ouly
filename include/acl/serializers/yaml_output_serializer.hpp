
#pragma once
#include <acl/serializers/output_serializer.hpp>
#include <acl/utils/error_codes.hpp>
#include <acl/utils/reflection.hpp>
#include <acl/utils/type_traits.hpp>
#include <functional>

namespace acl
{
namespace detail
{
class writer_state;

class writer_state
{
	std::string stream;

	int	 indent_level = -1;
	bool skip_indent	= false;

public:
	std::string get()
	{
		return std::move(stream);
	}

	void begin_array()
	{
		indent_level++;
		indent();
		stream.push_back('-');
		stream.push_back(' ');
	}

	void end_array()
	{
		indent_level--;
	}

	void begin_object()
	{
		indent_level++;
		indent();
	}

	void end_object()
	{
		indent_level--;
	}

	void key(std::string_view slice)
	{
		stream.append(slice);
		stream.push_back(':');
		stream.push_back(' ');
		skip_indent = false;
	}

	void as_string(std::string_view slice)
	{
		stream.append(slice);
		skip_indent = false;
	}

	void as_uint64(uint64_t value)
	{
		stream.append(std::to_string(value));
		skip_indent = false;
	}

	void as_int64(int64_t value)
	{
		stream.append(std::to_string(value));
		skip_indent = false;
	}

	void as_double(double value)
	{
		stream.append(std::to_string(value));
		skip_indent = false;
	}

	void as_bool(bool value)
	{
		stream.append(value ? "true" : "false");
		skip_indent = false;
	}

	void as_null()
	{
		stream.append("null");
		skip_indent = false;
	}

	void next_map_entry()
	{
		indent();
	}

	void next_array_entry()
	{
		indent();
		stream.push_back('-');
		stream.push_back(' ');
	}

private:
	void indent()
	{
		stream.push_back('\n');
		if (!skip_indent)
			std::fill_n(std::back_inserter(stream), static_cast<size_t>(indent_level), ' ');
		skip_indent = true;
	}
};

} // namespace detail

namespace yaml
{

template <typename Class, typename Opt = acl::options<>>
std::string to_string(Class const& obj)
{
	auto state			= detail::writer_state();
	auto serializer = output_serializer<detail::writer_state, Opt>(state);
	serializer << obj;
	return state.get();
}

} // namespace yaml
} // namespace acl
