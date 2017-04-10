#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>

#define ARRAY_SIZE(arr)		(sizeof(arr) / sizeof(*(arr)))

void die(char *fmt, ...);
void *realloc_safe(void *ptr, size_t new_size);
void *malloc_safe(size_t size);

#endif
