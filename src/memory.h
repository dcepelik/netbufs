#ifndef MEMORY_H
#define MEMORY_H

#include <stdlib.h>

void *cbor_malloc(size_t size);
void *cbor_realloc(void *ptr, size_t size);
void cbor_free(void *ptr);

void *realloc_safe(void *ptr, size_t new_size);
void *malloc_safe(size_t size);

#endif
