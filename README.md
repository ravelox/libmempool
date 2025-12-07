# Mempool

A minimal memory pool allocator with separate free/used block lists and a small demo driver.

## Building
- Requirements: `gcc`/`clang`, `make`.
- Run `make` to build `pool_test`.
- Clean artifacts with `make clean` (if present) or manually remove `pool.o`/`pool_test.o`.

## Usage
- See `pool_test.c` for an example: create a pool with `pool_create(size)`, call `pool_alloc`/`pool_free`, then `pool_destroy`.
- `pool_dump(pool)` prints allocator stats plus a 64-character-per-line view of the raw pool contents.
- To enable verbose internal logging, compile with `-DPOOL_DEBUG=1`.

## Notes
- Block metadata comes from a preallocated slab to avoid per-split `malloc`/`free` overhead; a fallback allocation is used only if the slab is exhausted.
- Free list insertions keep blocks sorted by address and coalesce adjacent blocks immediately, keeping traversal costs low.
