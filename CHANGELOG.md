# Changelog

## Unreleased
- Added shared library build with OS detection for macOS (`.dylib`) and Linux (`.so`), and rpath setup for `pool_test`.

## 0.0.1
- Added slab-managed storage for `MemoryBlock` headers to avoid frequent `malloc`/`free` during splits/merges, with a fallback when exhausted.
- Free/used block lists are kept sorted by address; free-block inserts coalesce adjacent ranges immediately and maintain counters as blocks move between lists.
- Added `POOL_DEBUG` flag to gate verbose logging.
- `pool_dump` emits allocator stats plus the entire pool in 64-character lines for an at-a-glance view of contents.
- Baseline allocator and `pool_test` demo exercising allocate/free patterns.
