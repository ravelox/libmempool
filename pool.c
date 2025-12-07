#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mempool.h"

#ifndef POOL_DEBUG
#define POOL_DEBUG 0
#endif

#define POOL_GUARD_DEFAULT_SIZE 16
#define POOL_GUARD_BYTE 0xAB

#define LOGF(...) do { if(POOL_DEBUG) fprintf(stderr, __VA_ARGS__); } while(0)

static void pool_dump_block(MemoryBlock *);
static MemoryBlock * pool_block_acquire(MemoryPool *pool);
static void pool_block_release(MemoryPool *pool, MemoryBlock *block);
static void pool_write_guards(MemoryPool *pool, MemoryBlock *block);
static int pool_check_guards(MemoryPool *pool, MemoryBlock *block);
static size_t pool_calc_block_capacity(size_t size)
{
	size_t capacity = size / 32; /* heuristic: one block header per 32 bytes of payload */
	if(capacity < 8) capacity = 8;
	return capacity;
}

static int
pool_block_from_slab(MemoryPool *pool, MemoryBlock *block)
{
	if(! pool || ! pool->block_slab) return 0;
	return block >= pool->block_slab && block < (pool->block_slab + pool->block_capacity);
}

static MemoryBlock *
pool_block_acquire(MemoryPool *pool)
{
	MemoryBlock *block;

	if(! pool) return NULL;

	if(pool->block_free_list)
	{
		block = pool->block_free_list;
		pool->block_free_list = block->next;
		block->next = NULL;
		block->prev = NULL;
		return block;
	}

	if(pool->block_used < pool->block_capacity)
	{
		block = &pool->block_slab[pool->block_used++];
		block->next = NULL;
		block->prev = NULL;
		return block;
	}

	/* Fallback for extreme fragmentation cases */
	block = (MemoryBlock *)malloc(sizeof(MemoryBlock));
	if(block)
	{
		block->next = NULL;
		block->prev = NULL;
	}
	return block;
}

static void
pool_block_release(MemoryPool *pool, MemoryBlock *block)
{
	if(! pool || ! block) return;

	if(pool_block_from_slab(pool, block))
	{
		block->next = pool->block_free_list;
		block->prev = NULL;
		pool->block_free_list = block;
		return;
	}

	free(block);
}

static void
pool_detach_block(MemoryBlock *block)
{
	if(! block) return;
	
	if(block->next) block->next->prev = block->prev;
	if(block->prev) block->prev->next = block->next;
	block->next = NULL;
	block->prev = NULL;
}

static MemoryBlock *
pool_insert_block_sorted(MemoryBlock **head, MemoryBlock *block)
{
	MemoryBlock *current;

	if(! head) return NULL;
	if(! block) return NULL;

	if(! *head)
	{
		block->prev = NULL;
		block->next = NULL;
		*head = block;
		return block;
	}

	current = *head;
	if( block->begin < current->begin )
	{
		block->next = current;
		block->prev = NULL;
		current->prev = block;
		*head = block;
		return block;
	}

	while(current->next && current->next->begin < block->begin)
	{
		current = current->next;
	}

	block->next = current->next;
	block->prev = current;
	if(current->next) current->next->prev = block;
	current->next = block;

	return block;
}

static void *
pool_block_user_begin(MemoryPool *pool, MemoryBlock *block)
{
	if(! block) return NULL;
	if(pool && pool->guard_enabled) return (char *)block->begin + pool->guard_size;
	return block->begin;
}

static void
pool_add_free_block(MemoryPool *pool, MemoryBlock *block)
{
	if(! pool) return;
	if(! block) return;

	if(! pool->free_blocks )
	{
		LOGF("pool_add_free_block: No free blocks\n");
		block->prev = NULL;
		block->next = NULL;
		pool->free_blocks = block;
		pool->num_free_blocks++;
	} else {
		LOGF("pool_add_free_block: [%p] %p %p\n", block, block->begin, pool->free_blocks->begin);
		block = pool_insert_block_sorted(&pool->free_blocks, block);
		pool->num_free_blocks++;
	}

	/* Merge with neighbors immediately to keep the list compact */
	if(block && block->prev && block->prev->end == block->begin)
	{
		MemoryBlock *prev = block->prev;
		prev->size += block->size;
		prev->end = block->end;
		prev->next = block->next;
		if(block->next) block->next->prev = prev;
		pool_block_release(pool, block);
		pool->num_free_blocks--;
		block = prev;
	}

	if(block && block->next && block->end == block->next->begin)
	{
		MemoryBlock *next = block->next;
		block->size += next->size;
		block->end = next->end;
		block->next = next->next;
		if(next->next) next->next->prev = block;
		pool_block_release(pool, next);
		pool->num_free_blocks--;
	}

	/* free-list blocks don't track payload */
	block->payload_size = 0;
}

static void
pool_add_used_block(MemoryPool *pool, MemoryBlock *block)
{
	if(! pool) return;
	if(! block) return;

	if(! pool->used_blocks )
	{
		LOGF("pool_add_used_block: No used blocks\n");
		pool->used_blocks = block;
		block->prev = NULL;
		block->next = NULL;
		pool->num_used_blocks++;
	} else {
		LOGF("pool_add_used_block: [%p] %p %p\n", block, block->begin, pool->used_blocks->begin);
		pool_insert_block_sorted(&pool->used_blocks, block);
		pool->num_used_blocks++;
	}
}

static void
pool_delete_free_block(MemoryPool *pool, MemoryBlock *block)
{
	if(! pool) return;
	if(! block) return;

	if( block == pool->free_blocks )
	{
		pool->free_blocks = block->next;
		pool->num_free_blocks--;
		block->next = NULL;
		block->prev = NULL;
		if(pool->free_blocks) pool->free_blocks->prev = NULL;
	} else {
		pool_detach_block(block);
		pool->num_free_blocks--;
	}
}

static void
pool_delete_used_block(MemoryPool *pool, MemoryBlock *block)
{
	if(! pool) return;
	if(! block) return;

	if( block == pool->used_blocks )
	{
		pool->used_blocks = block->next;
		pool->num_used_blocks--;
		block->next = NULL;
		block->prev = NULL;
		if(pool->used_blocks) pool->used_blocks->prev = NULL;
	} else {
		pool_detach_block(block);
		pool->num_used_blocks--;
	}

	pool->remaining += block->payload_size;
}

static MemoryBlock *
pool_find_block_by_pointer(MemoryPool *pool, MemoryBlock *block, void *pointer)
{
	if(! pointer) return NULL;

	while (block)
	{
		void *user_ptr = pool_block_user_begin(pool, block);
		if(user_ptr == pointer) return block;
		block = block->next;
	}

	return NULL;
}

static MemoryBlock *
pool_find_block(MemoryPool *pool, MemoryBlockType type, void *pointer)
{
	if(! pool) return NULL;
	if(! pointer) return NULL;

	if( type == FreeBlocks ) return pool_find_block_by_pointer(pool, pool->free_blocks, pointer);
	if( type == UsedBlocks ) return pool_find_block_by_pointer(pool, pool->used_blocks, pointer);

	return NULL;
}

MemoryPool *
pool_create(size_t size)
{
	MemoryPool *pool = NULL;
	MemoryBlock *initial = NULL;

	if(size <= 0) return NULL;

	pool = (MemoryPool *)malloc(sizeof(MemoryPool));

	if(! pool) return NULL;

	/* Allocate the memory */
	pool->allocated = malloc(size);

	if(! pool->allocated)
	{
		free(pool);
		return NULL;
	}

	memset(pool->allocated, '.', size);
	pool->size = size;
	pool->remaining = size;
	pool->free_blocks = NULL;
	pool->used_blocks = NULL;
	pool->num_free_blocks = 0;
	pool->num_used_blocks = 0;
	pool->block_capacity = pool_calc_block_capacity(size);
	pool->block_used = 0;
	pool->block_free_list = NULL;
	pool->block_slab = (MemoryBlock *)malloc(sizeof(MemoryBlock) * pool->block_capacity);
	pool->guard_enabled = 0;
	pool->guard_size = 0;
	if(! pool->block_slab)
	{
		free(pool->allocated);
		free(pool);
		return NULL;
	}

	initial = pool_block_acquire(pool);
	
	if(! initial)
	{
		free(pool->block_slab);
		free(pool->allocated);
		free(pool);
		return NULL;
	}
	
	initial->size = size;
	initial->payload_size = 0;
	initial->prev = NULL;
	initial->next = NULL;
	initial->begin = pool->allocated;
	initial->end = (char *)pool->allocated + size;

	pool->free_blocks = initial;
	pool->num_free_blocks = 1;
	return pool;
}

static void
pool_block_free(MemoryPool *pool, MemoryBlock *block)
{
	while (block)
	{
		MemoryBlock *next = block->next;
		block->next = NULL;
		block->prev = NULL;
		pool_block_release(pool, block);
		block = next;
	}
}

void
pool_destroy(MemoryPool *pool)
{
	if(! pool) return;

	pool_block_free(pool, pool->free_blocks);
	pool->free_blocks = NULL;
	pool_block_free(pool, pool->used_blocks);
	pool->used_blocks = NULL;

	free(pool->allocated);
	if(pool->block_slab) free(pool->block_slab);
	free(pool);
}

static MemoryBlock *
pool_split_block(MemoryPool *pool, MemoryBlock *block, size_t size)
{
	MemoryBlock *split = NULL;

	if(! pool) return NULL;
	if(! block) return NULL;

	if(size > block->size) return NULL;

	split = pool_block_acquire(pool);
	if(! split) return NULL;

	block->end = (char *)block->begin + size;
	split->size = block->size - size;
	block->size = size;
	split->begin = block->end;
	split->end = (char *)block->end + split->size;
	split->prev = block;
	split->next = block->next;
	split->payload_size = 0;
	block->next = split;
	if(split->next) split->next->prev = split;

	return block;
}

static MemoryBlock *
pool_get_free_block(MemoryPool *pool, MemoryBlock *block, size_t size)
{
	if(! pool) return NULL;

	while (block)
	{
		if( block->size >= size ) return block;
		block = block->next;
	}

	return NULL;
}

void *
pool_alloc(MemoryPool *pool, size_t size)
{
	MemoryBlock *block;
	size_t guard_overhead = 0;
	if(! pool) return NULL;
	if(pool->guard_enabled) guard_overhead = pool->guard_size * 2;
	if(size + guard_overhead > pool->remaining) return NULL;

	LOGF("pool_alloc: Requested=%zu\n", size);
	block = pool_get_free_block(pool, pool->free_blocks, size + guard_overhead);

	if(! block) return NULL;

	if(block->size > size + guard_overhead)
	{
		block = pool_split_block(pool, block, size + guard_overhead);
		if(block) pool->num_free_blocks++;
	}
	
	pool->remaining -= size;

	block->payload_size = size;

	/* initialize guards and payload */
	if(pool->guard_enabled)
	{
		pool_write_guards(pool, block);
	}

	memset(pool_block_user_begin(pool, block), '0', size);

	pool_delete_free_block(pool, block);
	pool_add_used_block(pool, block);

	return pool_block_user_begin(pool, block);
}

void
pool_free(MemoryPool *pool, void *pointer)
{
	MemoryBlock *block;

	block = pool_find_block(pool, UsedBlocks, pointer);

	if(! block) return;

	if(pool->guard_enabled && !pool_check_guards(pool, block))
	{
		fprintf(stderr, "pool_free: guard region corrupted for block %p\n", pointer);
	}

	memset(pool_block_user_begin(pool, block), '.', block->payload_size);

	pool_delete_used_block(pool, block);
	pool_add_free_block(pool, block);
}

static void
pool_dump_block(MemoryBlock *block)
{
	if(! block) return;
	LOGF("\t[%p]<--([%p] %p %p [size=%zu])-->[%p]\n", block->prev, block, block->begin, block->end, block->size, block->next);
	pool_dump_block(block->next);
}

void
pool_dump(MemoryPool *pool)
{
	if(! pool) return;

	fprintf(stderr, "Pool: Size=%zu\tBegin=%p\tEnd=%p\tRemaining=%zu\tFree Blocks=%zu\tUsed Blocks=%zu\n", pool->size, pool->allocated, (char *)pool->allocated + pool->size, pool->remaining, pool->num_free_blocks, pool->num_used_blocks);
	fprintf(stderr, "Free Blocks [%p]\n", pool->free_blocks);
	LOGF("Free block chain:\n");
	pool_dump_block(pool->free_blocks);
	fprintf(stderr, "Used Blocks [%p]\n", pool->used_blocks);
	LOGF("Used block chain:\n");
	pool_dump_block(pool->used_blocks);
		fprintf(stderr, "-----------\n");

	/* Dump the raw pool in 64-character lines */
	if(pool->allocated && pool->size)
	{
		size_t offset = 0;
		char *mem = (char *)pool->allocated;
		while(offset < pool->size)
		{
			size_t chunk = pool->size - offset;
			if(chunk > 64) chunk = 64;
			fwrite(mem + offset, 1, chunk, stderr);
			fputc('\n', stderr);
			offset += chunk;
		}
		fprintf(stderr, "-----------\n");
	}

}

const char *
pool_version(void)
{
	return "0.0.2";
}

void
pool_enable_guards(MemoryPool *pool, size_t guard_size)
{
	if(! pool) return;
	if(guard_size == 0) guard_size = POOL_GUARD_DEFAULT_SIZE;
	pool->guard_enabled = 1;
	pool->guard_size = guard_size;
}

int
pool_disable_guards(MemoryPool *pool)
{
	if(! pool) return 0;
	if(pool->num_used_blocks > 0) return 0;
	pool->guard_enabled = 0;
	pool->guard_size = 0;
	return 1;
}

static void
pool_write_guards(MemoryPool *pool, MemoryBlock *block)
{
	size_t guard;
	if(! pool || ! block) return;
	if(! pool->guard_enabled) return;
	guard = pool->guard_size;
	memset(block->begin, POOL_GUARD_BYTE, guard);
	memset((char *)pool_block_user_begin(pool, block) + block->payload_size, POOL_GUARD_BYTE, guard);
}

static int
pool_check_guards(MemoryPool *pool, MemoryBlock *block)
{
	size_t guard, i;
	char *start, *end;
	if(! pool || ! block) return 1;
	if(! pool->guard_enabled) return 1;
	guard = pool->guard_size;
	start = (char *)block->begin;
	end = (char *)pool_block_user_begin(pool, block) + block->payload_size;
	for(i = 0; i < guard; ++i)
	{
		if((unsigned char)start[i] != POOL_GUARD_BYTE) return 0;
		if((unsigned char)end[i] != POOL_GUARD_BYTE) return 0;
	}
	return 1;
}
