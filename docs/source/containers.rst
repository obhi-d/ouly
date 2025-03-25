Containers
==========

ACL provides several container types optimized for different use cases.

Basic Queue
----------

A queue container with block-based memory allocation that provides efficient push and pop operations. 

Key features:

- Block-based memory allocation with configurable block size
- Memory reuse through free list
- Support for move and copy operations 
- Exception-safe operations
- Custom allocator support

Example usage::

	basic_queue<int> queue;
	queue.emplace_back(1);  
	int value = queue.pop_front();

Small Vector
-----------

A vector implementation that can store a small number of elements inline before requiring heap allocation.

Key features:

- Inline storage for small arrays (configurable capacity N)
- Automatic transition to heap storage when size exceeds inline capacity
- STL-compatible container interface
- Custom allocator support
- Move semantics support

Example usage::

	small_vector<int, 16> vec;  // Can store up to 16 ints inline
	vec.push_back(42);         

Dynamic Array
------------

A dynamic non-growable vector-like container.

Key features:

- Fixed size after construction
- Contiguous memory layout
- Custom allocator support
- STL-compatible iterators
- Support for trivial and non-trivial types

Example usage::

	dynamic_array<int> arr(10);  // Fixed size of 10 elements
	arr[0] = 42;

Intrusive List
-------------

An intrusive linked list implementation that does not manage memory for nodes.

Key features:

- O(1) insertion and removal operations
- Optional tail caching for O(1) push_back
- Support for singly and doubly-linked modes
- No memory allocation/deallocation
- Requires nodes to contain hook members

Example usage::

	struct Node {
		acl::list_hook hook;
		// ... other data
	};
	
	intrusive_list<&Node::hook> list;

Blackboard
---------

A key-value store container that can hold heterogeneous data types.

Key features:

- Store and retrieve arbitrary data types
- String or type-based indexing
- Custom allocator support
- Optional memory tracking
- Support for non-POD types

Example usage::

	blackboard<> board;
	board.set<int>("counter", 42);
	int value = board.get<int>("counter");



.. autodoxygenindex::
   :project: containers
