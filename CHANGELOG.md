# Changelog

## 0.0.2
- Added guard regions with runtime enable/disable; disabling is rejected while allocations are outstanding and tests cover the guard toggle scenarios.
- Updated `pool_version` to report 0.0.2.

## 0.0.1
- Added shared library build with OS detection for macOS (`.dylib`) and Linux (`.so`), and rpath setup for `pool_test`.
- Added slab-managed storage for `MemoryBlock` headers to avoid frequent `malloc`/`free` during splits/merges, with a fallback when exhausted.
- Free/used block lists are kept sorted by address; free-block inserts coalesce adjacent ranges immediately and maintain counters as blocks move between lists.
- Added `POOL_DEBUG` flag to gate verbose logging.
- `pool_dump` emits allocator stats plus the entire pool in 64-character lines for an at-a-glance view of contents.
- Baseline allocator and `pool_test` demo exercising allocate/free patterns.
