/*
 * mempool:
 * Default Implementation of a Memory Pool
 *
 * Taken from the MCC project (c) 2016 David Čepelík <d@dcepelik.cz>
 * See github.com/dcepelik/mcc.
 */

#include "common.h"
#include "debug.h"
#include "memory.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#define DEBUG_THIS	0


struct mempool_block
{
	struct mempool_block *prev;	/* previous block in the chain */
	size_t alloc_size;		/* allocated size */
	size_t size;			/* usable size */
};

struct mempool_chain
{
	struct mempool_block *last;	/* last block */
	size_t last_free;		/* free memory in last block */
	size_t total_size;		/* total size of the chain */
	size_t num_blocks;		/* number of blocks in the chain */
};

struct mempool
{
	struct mempool_chain small;	/* chain of blocks for small objects */
	struct mempool_chain big;	/* chain of blocks for big objects */
	struct mempool_chain unused;	/* chain of unused blocks */
	size_t block_size;		/* size of small objects block */
	size_t small_treshold;		/* maximum size of a small object */
};


static void mempool_init_chain(struct mempool_chain *chain)
{
	chain->last = NULL;
	chain->last_free = 0;
	chain->num_blocks = 0;
	chain->total_size = 0;
}


static void mempool_free_chain(struct mempool_chain *chain)
{
	struct mempool_block *block;
	void *mem;

	while (chain->last) {
		block = chain->last;
		chain->last = chain->last->prev;
		
		mem = (unsigned char *)block - block->size;
		DEBUG_PRINTF("About to free raw block at %p", mem);

		chain->num_blocks--;
		chain->total_size -= block->alloc_size;
		free(mem);
	}
}


struct mempool_block *mempool_new_block(struct mempool_chain *chain, size_t size)
{
	// TODO Alignment

	struct mempool_block *new_block;
	size_t alloc_size;
	void *mem;

	alloc_size = size + sizeof(*new_block);
	
	mem = nb_malloc(alloc_size);

	DEBUG_PRINTF("Alocated new block, alloc_size = %zu B and size = %zu B",
		alloc_size, size);
	DEBUG_PRINTF("Raw block starts at %p", mem);

	new_block = (struct mempool_block *)((unsigned char *)mem + size);

	DEBUG_PRINTF("Block trailer resides at %p", (void *)new_block);

	new_block->alloc_size = alloc_size;
	new_block->size = size;

	new_block->prev = chain->last;
	chain->last = new_block;

	chain->total_size += alloc_size;
	chain->num_blocks++;
	chain->last_free = size;

	return new_block;
}


static inline void *mempool_alloc_chain(struct mempool_chain *chain, size_t size)
{
	size_t free_size;

	free_size = chain->last_free;
	assert(free_size >= size);

	chain->last_free -= size;
	assert(chain->last_free >= 0);

	return (unsigned char *)chain->last - free_size;
}


void *mempool_malloc(struct mempool *pool, size_t size)
{
	DEBUG_PRINTF("Alloc request, size = %zu B", size);

	if (size <= pool->small_treshold) {
		if (pool->small.last_free < size) {
			DEBUG_MSG("Allocating block in small chain");
			mempool_new_block(&pool->small, pool->block_size);
		}

		return mempool_alloc_chain(&pool->small, size);
	}
	else {
		DEBUG_MSG("Allocating block in big chain");
		mempool_new_block(&pool->big, size);
		return mempool_alloc_chain(&pool->big, size);
	}
}


void *mempool_memcpy(struct mempool *pool, void *src, size_t len)
{
	char *dst;

	if (!src)
		return NULL;

	dst = mempool_malloc(pool, len);

	DEBUG_EXPR("%lu", len);

	memcpy(dst, src, len);
	return dst;
}


char *mempool_strdup(struct mempool *pool, char *orig)
{
	char *dup;
	size_t len;

	if (!orig)
		return NULL;

	len = strlen(orig);
	dup = mempool_malloc(pool, len + 1);

	memcpy(dup, orig, len + 1);
	dup[len] = '\0';

	return dup;	
}


static void mempool_init(struct mempool *pool, size_t block_size)
{
	pool->block_size = block_size;
	pool->small_treshold = block_size / 2;

	mempool_init_chain(&pool->small);
	mempool_init_chain(&pool->big);
}


struct mempool *mempool_new(size_t block_size)
{
	struct mempool pool;
	mempool_init(&pool, block_size);
	return mempool_memcpy(&pool, &pool, sizeof(pool));
}


void mempool_delete(struct mempool *pool)
{
	struct mempool pool_copy = *pool;
	mempool_free_chain(&pool_copy.small);
	mempool_free_chain(&pool_copy.big);
}


static void print_chain_stats(struct mempool_chain *chain)
{
	printf("%lu blocks, %lu B total\n", chain->num_blocks, chain->total_size);
}


void mempool_print_stats(struct mempool *pool)
{
	printf("Mempool stats:\n");
	printf("\tBig objects chain: ");
	print_chain_stats(&pool->big);
	printf("\tSmall objects chain: ");
	print_chain_stats(&pool->small);
}
