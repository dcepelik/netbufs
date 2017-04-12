#ifndef MEMORY_H
#define MEMORY_H

#include <stdlib.h>

void *cbor_aloc(size_t size);
void *cbor_realloc(void *ptr, size_t size);

#endif
