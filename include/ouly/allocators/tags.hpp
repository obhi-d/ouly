#pragma once
// SPDX-License-Identifier: MIT

#pragma once

namespace ouly
{
struct default_allocator_tag
{};

struct linear_allocator_tag
{};

struct linear_arena_allocator_tag
{};

struct linear_stack_allocator_tag
{};

struct mmap_allocator_tag
{};

struct virtual_memory_allocator_tag
{};
} // namespace ouly