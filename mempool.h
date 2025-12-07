#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
	FreeBlocks,
	UsedBlocks
} MemoryBlockType;

typedef struct _block_t {
	size_t size;
	void *begin, *end;
	struct _block_t *prev, *next;
} MemoryBlock;

typedef struct _pool_t {
	size_t size, remaining;
	void *allocated;
	size_t num_free_blocks, num_used_blocks;
	MemoryBlock *free_blocks;
	MemoryBlock *used_blocks;
	MemoryBlock *block_slab;
	MemoryBlock *block_free_list;
	size_t block_capacity;
	size_t block_used;
} MemoryPool;

MemoryPool * pool_create(size_t );
void * pool_alloc(MemoryPool *, size_t );
void pool_free(MemoryPool *, void *);
void pool_destroy(MemoryPool *);
void pool_dump(MemoryPool *);
