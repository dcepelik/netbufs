#include "util.h"

#include <stdio.h>
#include <stdarg.h>


void die(char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	vfprintf(stderr, fmt, args);

	va_end(args);
	exit(EXIT_FAILURE);
}


void *realloc_safe(void *ptr, size_t new_size)
{
	void *x = realloc(ptr, new_size);
	if (!x) {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}

	return x;
}


void *malloc_safe(size_t size)
{
	return realloc_safe(NULL, size);
}
