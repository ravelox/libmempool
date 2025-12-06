#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mempool.h"

static void pool_dump_block(MemoryBlock *);

static void
pool_add_block(MemoryBlock *parent, MemoryBlock *child)
{
	MemoryBlock *temp;
	if(! parent) return;
	if(! child) return;

	temp = parent;
	while(temp)
	{
		if( ! temp->next )
		{
			fprintf(stderr, "pool_add_block: No next\n");
			temp->next = child;
			child->prev = temp;
			child->next = NULL;
			return;
		}

		fprintf(stderr, "pool_add_block: %p %p ==> [%s]\n", child->begin, temp->begin, ( child->begin < temp->begin ? "Add above" : "Continue") );
		if( child->begin < temp->begin )
		{
			temp->prev->next = child;
			child->prev = temp->prev;
			child->next = temp;
			temp->prev = child;
			return;
		}

		temp = temp->next;
	}
}

static void
pool_add_free_block(MemoryPool *pool, MemoryBlock *block)
{
	if(! pool) return;
	if(! block) return;

	if(! pool->free_blocks )
	{
		fprintf(stderr, "pool_add_free_block: No free blocks\n");
		pool->free_blocks = block;
		return;
	}

	fprintf(stderr, "pool_add_free_block: [%p] %p %p\n", block, block->begin, pool->free_blocks->begin);
	if( block->begin < pool->free_blocks->begin )
	{
		fprintf(stderr, "pool_add_free_block: Add above\n");
		block->next = pool->free_blocks;
		pool->free_blocks->prev = block;
		pool->free_blocks = block;
		block->prev = NULL;
	} else {
		pool_add_block(pool->free_blocks, block);
	}
}

static void
pool_add_used_block(MemoryPool *pool, MemoryBlock *block)
{
	if(! pool) return;
	if(! block) return;

	if(! pool->used_blocks )
	{
		fprintf(stderr, "pool_add_used_block: No used blocks\n");
		pool->used_blocks = block;
		return;
	}

	fprintf(stderr, "pool_add_used_block: [%p] %p %p\n", block, block->begin, pool->used_blocks->begin);
	if( block->begin < pool->used_blocks->begin )
	{
		fprintf(stderr, "pool_add_used_block: Add above\n");
		block->next = pool->used_blocks;
		pool->used_blocks->prev = block;
		pool->used_blocks = block;
		block->prev = NULL;
	} else {
		pool_add_block(pool->used_blocks, block);
	}
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

static void
pool_delete_free_block(MemoryPool *pool, MemoryBlock *block)
{
	if(! pool) return;
	if(! block) return;

	if( block == pool->free_blocks )
	{
		pool->free_blocks = block->next;
		block->next = NULL;
		block->prev = NULL;
		if(pool->free_blocks) pool->free_blocks->prev = NULL;
	} else {
		pool_detach_block(block);
		pool_dump_block(pool->free_blocks);
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
		block->next = NULL;
		block->prev = NULL;
		if(pool->used_blocks) pool->used_blocks->prev = NULL;
	} else {
		pool_detach_block(block);
		pool_dump_block(pool->used_blocks);
	}

	pool->remaining += block->size;
}

static MemoryBlock *
pool_find_block_by_pointer(MemoryPool *pool, MemoryBlock *block, void *pointer)
{
	(void)pool;
	if(! pointer) return NULL;

	while (block)
	{
		if(block->begin == pointer) return block;
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

	initial = (MemoryBlock *)malloc(sizeof(MemoryBlock));
	
	if(! initial)
	{
		free(pool->allocated);
		free(pool);
		return NULL;
	}
	
	initial->size = size;
	initial->prev = NULL;
	initial->next = NULL;
	initial->begin = pool->allocated;
	initial->end = (char *)pool->allocated + size;

	pool->free_blocks = initial;
	return pool;
}

static void
pool_block_free(MemoryBlock *block)
{
	while (block)
	{
		MemoryBlock *next = block->next;
		block->next = NULL;
		block->prev = NULL;
		free(block);
		block = next;
	}
}

void
pool_destroy(MemoryPool *pool)
{
	if(! pool) return;

	pool_block_free(pool->free_blocks);
	pool->free_blocks = NULL;
	pool_block_free(pool->used_blocks);
	pool->used_blocks = NULL;

	free(pool->allocated);
	free(pool);
}

static int
pool_count_blocks(MemoryBlock *block)
{
	int count = 0;
	while (block)
	{
		++count;
		block = block->next;
	}
	return count;
}

static MemoryBlock *
pool_split_block(MemoryBlock *block, size_t size)
{
	MemoryBlock *split = NULL;

	if(! block) return NULL;

	if(size > block->size) return NULL;

	split = (MemoryBlock *)malloc(sizeof(MemoryBlock));
	if(! split) return NULL;

	block->end = (char *)block->begin + size;
	split->size = block->size - size;
	block->size = size;
	split->begin = block->end;
	split->end = (char *)block->end + split->size;
	split->prev = block;
	split->next = block->next;
	block->next = split;

	return block;
}

static int
pool_merge_block(MemoryBlock *block)
{
	MemoryBlock *current;

	if(! block) return 0;
	if(! block->next ) return 0;

	fprintf(stderr, "pool_merge_block: %p %p\n", block->end, block->next->begin);
	if( block->end != block->next->begin ) return 0;

	block->size += block->next->size;
	block->end = block->next->end;

	current = block->next;
	if(current->next) current->next->prev = block;
	block->next = current->next;
	current->prev = NULL;
	current->next = NULL;
	free(current);

	return 1;
}

static void
pool_merge_free_blocks(MemoryPool *pool)
{
	MemoryBlock *block;
	if(! pool) return;

	fprintf(stderr, "pool_merge_free_blocks\n");
	for (block = pool->free_blocks; block && block->next; )
	{
		if (pool_merge_block(block))
		{
			pool_dump_block(pool->free_blocks);
			continue;
		}
		block = block->next;
	}
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
	if(! pool) return NULL;
	if(size > pool->remaining) return NULL;

	fprintf(stderr, "pool_alloc: Requested=%zu\n", size);
	block = pool_get_free_block(pool, pool->free_blocks, size);

	if(! block) return NULL;

	if(block->size > size)
	{
		block = pool_split_block(block, size);
	}
	
	pool->remaining -= size;

	memset(block->begin, '0', size);

	pool_delete_free_block(pool, block);
	pool_add_used_block(pool, block);

	return block->begin;
}

void
pool_free(MemoryPool *pool, void *pointer)
{
	MemoryBlock *block;

	block = pool_find_block(pool, UsedBlocks, pointer);

	if(! block) return;

	memset(block->begin, '.', block->size);

	pool_delete_used_block(pool, block);
	pool_add_free_block(pool, block);

	pool_merge_free_blocks(pool);
}

static void
pool_dump_block(MemoryBlock *block)
{
	if(! block) return;
	fprintf(stderr, "\t[%p]<--([%p] %p %p [size=%zu])-->[%p]\n", block->prev, block, block->begin, block->end, block->size, block->next);
	pool_dump_block(block->next);
}

void
pool_dump(MemoryPool *pool)
{
	if(! pool) return;

	pool->num_free_blocks = pool_count_blocks(pool->free_blocks);
	pool->num_used_blocks = pool_count_blocks(pool->used_blocks);
	fprintf(stderr, "Pool: Size=%zu\tBegin=%p\tEnd=%p\tRemaining=%zu\tFree Blocks=%zu\tUsed Blocks=%zu\n", pool->size, pool->allocated, (char *)pool->allocated + pool->size, pool->remaining, pool->num_free_blocks, pool->num_used_blocks);
	fprintf(stderr, "Free Blocks [%p]\n", pool->free_blocks);
	pool_dump_block(pool->free_blocks);
	fprintf(stderr, "Used Blocks [%p]\n", pool->used_blocks);
	pool_dump_block(pool->used_blocks);
	fprintf(stderr, "-----------\n");
}
