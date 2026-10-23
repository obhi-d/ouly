Changelog
=========

Notable changes to OULY. The format is based on
`Keep a Changelog <https://keepachangelog.com/en/1.0.0/>`_. The current version, as set in
``CMakeLists.txt``, is 0.1.22; releases up to that version predate this changelog and are described
by the git history.

[Unreleased]
------------

Changed
~~~~~~~

- **small_vector**: whether the elements live inline or on the heap is now explicit state instead
  of being derived from the size. Once on the heap the vector stays there, as ``std::vector`` does:
  ``clear()``, ``pop_back()``, ``erase()`` and a shrinking ``resize()`` keep the heap buffer;
  ``reserve()`` allocates even while the elements are inline; ``shrink_to_fit()`` is the only call
  that moves the elements back inline. ``is_inlined()`` reports where the elements are. ``resize()``
  grows geometrically and ``assign()`` keeps the capacity. ``sizeof`` is unchanged.
- **coalescing_arena_allocator**: each memory class keeps one emptied arena registered and reuses
  it, so an allocate/free cycle no longer creates and destroys backing memory every time.
  ``set_retained_empty_arenas(n)`` (default 1) and ``get_retained_empty_arenas()`` control the
  limit, ``trim(manager)`` returns the retained arenas. Dedicated arenas, extra empty arenas of a
  class and arenas emptied by an evacuation are still released immediately.
- **auto_parallel_for**: stolen ranges are now split further as intended (the divisibility test was
  inverted, so the loop behaved like a static split into about one chunk per worker). Skewed
  workloads run about twice as fast on four workers; uniform work costs a few percent more.
- **v3::scheduler**: ``busy_work`` returns whether it ran a task, and cooperative waits only yield
  when it did not. Submission, chained wake-ups and task completion skip the global mutex and
  ``notify_all`` while no worker is parked.
- **v1/v2 schedulers**: the stealing PRNG no longer produces only even numbers, and v2 seeds it per
  worker so thieves do not all visit the same victims. v2 keeps its hot atomics on separate cache
  lines.
- **microexpr**: supports logical ``!``, ``$(NAME)`` and identifiers containing ``_``.
- **mmap_file**: ``map(path, offset, length)`` maps ``offset + length`` bytes so the view covers the
  requested range; ``make_mmap_source``/``make_mmap_sink`` are ``inline``.
- **soavector**: ``resize()`` grows geometrically.
- **concurrent_queue**: allocates buckets four per page (a ``concurrent_queue<int>`` previously
  reserved about 85 MB) and no longer zeroes bucket storage under the exclusive lock.
- **basic_queue**: slots use the element's alignment instead of ``max_align_t``.

Fixed
~~~~~

- **CMake**: OULY can be consumed with ``add_subdirectory`` and ``FetchContent``. The target's
  include directory (and the natvis and ``compile_commands.json`` paths) were derived from
  ``CMAKE_SOURCE_DIR``, which is the parent project's directory in that setup.
- **small_vector**: truncating ``resize()`` destroys the removed elements; moving from an inline
  vector destroys the moved-from elements; ``assign(n, value)`` no longer selects the iterator
  overload; ``swap`` with allocator propagation compiles.
- **sparse_vector**: iterators skip absent pages in one step; copy and move assignment no longer
  leak the target's pages. **sparse_table**: the ``const`` ``get_if()`` compiles.
- **pool_allocator**: releasing a multi-atom block after the array list was exhausted dereferenced
  null; move assignment frees the target's arenas.
- **ts_thread_local_allocator**: each instance has its own per-thread page, so alternating between
  instances on one thread no longer takes a new page per call; move assignment releases instead of
  leaking.
- **linear_arena_allocator** / **linear_stack_allocator**: full arenas leave the first-fit scan;
  ``deallocate`` and in-place resize only inspect the current arena.
- **default_allocator**: alignments up to ``__STDCPP_DEFAULT_NEW_ALIGNMENT__`` use plain
  ``operator new``/``delete``.
- **ecs::collection**: tracks entity revisions whenever the entity type has revision bits; Release
  builds previously skipped recycled entities during ``for_each``. **registry::for_each_index**
  sorts the free list only after an erase.
- **Serialization**: YAML ``null`` no longer reads as ``true`` for ``bool`` fields; binary reads of
  ``std::string`` move the decoded buffer; ``convert<std::unique_ptr<char[]>>`` NUL-terminates and
  handles null.
- **microexpr**: ``&&`` and ``||`` always consume their right operand (the rest of the expression
  was misparsed after a short circuit); no out-of-bounds read on an unclosed ``(``.
- **Utilities**: ``trim_leading``/``trim_trailing``/``trim_view`` return an empty view for
  all-whitespace input; ``is_number`` rejects ``"-"``; ``program_args::as<T>`` works for types
  other than ``bool``.
- Release (``-DNDEBUG``), ``-pedantic`` and clang-tidy build failures.

Documentation
~~~~~~~~~~~~~

- The README is a short overview with one compiled example per module.
- The tutorials were rewritten against the current API, a utilities and DSL tutorial was added,
  and the user guide, examples, performance and API reference pages now only reference content
  that exists. Doxygen expands ``OULY_API`` so exported functions appear in the API reference.
