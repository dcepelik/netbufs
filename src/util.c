#include "util.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>


void die(char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
	exit(EXIT_FAILURE);
}
