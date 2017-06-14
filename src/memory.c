#include "memory.h"
#include "util.h"
#include <stdlib.h>


void *nb_malloc(size_t size)
{
	return nb_realloc(NULL, size);
}


void *nb_realloc(void *ptr, size_t size)
{
	return realloc(ptr, size);
}


void xfree(void *ptr)
{
	free(ptr);
}
