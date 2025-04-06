
Allocators
==========

OULY provides several allocator implementations to suit different memory management needs:

Linear Allocator 
---------------
A simple allocator that allocates memory linearly and only frees everything at once. 
Useful for temporary allocations with a known lifetime.

Pool Allocator
-------------
Fixed-size block allocator that maintains a free list of blocks.
Efficient for allocating many objects of the same size.

Arena Allocator 
--------------
An allocator that allocates from a fixed memory arena. Similar to linear allocator
but supports individual deallocations within the arena.

Coalescing Allocator
-------------------
An allocator that coalesces adjacent free blocks to reduce fragmentation.
Useful for long-running applications that need to minimize memory fragmentation.

Coalescing Arena Allocator 
-------------------------
Combines arena allocation with block coalescing. Memory is allocated from a fixed 
arena and adjacent free blocks are merged to reduce fragmentation.

.. autodoxygenindex::
   :project: allocators
