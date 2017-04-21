#include "memory.h"
#include "util.h"
#include <stdlib.h>


void *cbor_malloc(size_t size)
{
	return cbor_realloc(NULL, size);
}


void *cbor_realloc(void *ptr, size_t size)
{
	return realloc(ptr, size);
}


void cbor_free(void *ptr)
{
	free(ptr);
}
