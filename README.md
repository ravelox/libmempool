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
- **API summary**
  - `MemoryPool *pool_create(size_t size)`: allocate a pool of `size` bytes and return a handle.
  - `void *pool_alloc(MemoryPool *pool, size_t size)`: allocate `size` bytes from the pool; returns `NULL` on failure.
  - `void pool_free(MemoryPool *pool, void *ptr)`: free a block previously returned by `pool_alloc`.
  - `void pool_destroy(MemoryPool *pool)`: release all resources associated with the pool.
  - `void pool_dump(MemoryPool *pool)`: log pool stats and a 64-character-wide view of the pool contents.
  - `const char *pool_version(void)`: return the library version string.
  - `void pool_enable_guards(MemoryPool *pool, size_t guard_size)`: enable guard regions of `guard_size` bytes (default if 0) around allocations; detects overruns in debug flows.
  - `int pool_disable_guards(MemoryPool *pool)`: disable guard regions if no allocations are outstanding; returns 1 on success, 0 otherwise.

## Notes
- Block metadata comes from a preallocated slab to avoid per-split `malloc`/`free` overhead; a fallback allocation is used only if the slab is exhausted.
- Free list insertions keep blocks sorted by address and coalesce adjacent blocks immediately, keeping traversal costs low.
- Built as a shared library (`libmempool.dylib` on macOS, `libmempool.so` on Linux) and linked by `pool_test` with an rpath to its directory.
- Guard regions can be enabled at runtime with `pool_enable_guards(pool, guard_size)`; disabling is only allowed when no allocations are outstanding.
- Library version string: `pool_version()` currently returns `0.0.2`.
