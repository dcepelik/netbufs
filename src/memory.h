/*
 * memory:
 * Memory Management
 *
 * This header file is an interface between NetBufs and user-provided
 * memory-management routines.
 *
 * There is, however, a default implementation in memory.c and mempool.c.
 */

#ifndef MEMORY_H
#define MEMORY_H

#include <stdlib.h>

struct mempool;
typedef struct mempool *mempool_t; /* shan't be here */

void *nb_malloc(size_t size);
void *nb_realloc(void *ptr, size_t size);
void xfree(void *ptr);

void *realloc_safe(void *ptr, size_t new_size);
void *malloc_safe(size_t size);

mempool_t mempool_new(size_t block_size);
void mempool_delete(mempool_t pool);

void *mempool_malloc(mempool_t pool, size_t size);
void *mempool_realloc(mempool_t pool, void *mem, size_t size);
void mempool_free(void *mem);

#endif
