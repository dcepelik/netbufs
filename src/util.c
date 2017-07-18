#include "util.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>


void nb_die(char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	abort();
	exit(EXIT_FAILURE);
}
