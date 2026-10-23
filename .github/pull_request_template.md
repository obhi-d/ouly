## Summary

<!-- What does this change and why? Link related issues (e.g. "Fixes #123"). -->

## Changes

<!-- Group by module (allocators, containers, ecs, scheduler, serializers, dsl, utility, docs, build). -->

-

## Behavior and API changes

<!-- Anything a user of the library could notice: changed semantics, new or removed API, ABI or
     layout changes, performance trade-offs. Write "None" if there are none. -->

None

## Testing

- [ ] Added or updated unit tests in `unit_tests/` (new bugs fixed come with a test that fails without the fix)
- [ ] `ctest` passes locally (compiler and build type: )
- [ ] Ran with sanitizers (`ASAN_ENABLED` / Debug test targets) where memory or threading code changed
- [ ] `clang-tidy` is clean (`-DOULY_ENABLE_CLANG_TIDY=ON`) and changed lines are `clang-format`ted
- [ ] Docs updated (README, `docs/source`, doxygen comments) if behavior or API changed

<!-- For performance changes, include before/after numbers and how they were measured. -->
