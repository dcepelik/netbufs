#include "memory.h"
#include "util.h"
#include <stdlib.h>


void *cbor_malloc(size_t size)
{
	return cbor_realloc(NULL, size);
}


void *cbor_realloc(void *ptr, size_t size)
{
	void *new_ptr;

	new_ptr = realloc(ptr, size);
	if (!new_ptr)
		die("Cannot allocate %zu bytes of memory.");

	return new_ptr;
}


void cbor_free(void *ptr)
{
	free(ptr);
}
