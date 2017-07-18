/*
 * memory:
 * Default Memory Bridge
 */

#include "memory.h"
#include "util.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>


void *nb_malloc(size_t size)
{
	return nb_realloc(NULL, size);
}


void *nb_realloc(void *ptr, size_t size)
{
	ptr = realloc(ptr, size);
	if (!ptr) {
		assert(false);
		nb_die("Cannot allocate %zu bytes of memory.\n", size);
	}

	return ptr;
}


void xfree(void *ptr)
{
	free(ptr);
}
