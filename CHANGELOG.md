# Changelog

## Unreleased
- `pool_dump` now emits the entire pool in 64-character lines for an at-a-glance view of contents.

## 0.1.0
- Added slab-managed storage for `MemoryBlock` headers to avoid frequent `malloc`/`free` during splits/merges, with a fallback when exhausted.
- Free/used block lists are kept sorted by address; free-block inserts coalesce adjacent ranges immediately and maintain counters as blocks move between lists.
- Added `POOL_DEBUG` flag to gate verbose logging.
- Baseline allocator and `pool_test` demo exercising allocate/free patterns.
