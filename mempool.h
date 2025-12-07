#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
	FreeBlocks,
	UsedBlocks
} MemoryBlockType;

typedef struct _block_t {
	size_t size;
	size_t payload_size;
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
	int guard_enabled;
	size_t guard_size;
} MemoryPool;

MemoryPool * pool_create(size_t );
void * pool_alloc(MemoryPool *, size_t );
void pool_free(MemoryPool *, void *);
void pool_destroy(MemoryPool *);
void pool_dump(MemoryPool *);
const char * pool_version(void);
void pool_enable_guards(MemoryPool *, size_t guard_size);
int pool_disable_guards(MemoryPool *);
