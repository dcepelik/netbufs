#ifndef DEBUG_H
#define DEBUG_H

#include <assert.h>
#include <stdio.h>

#undef NB_DEBUG_THIS

#ifndef NB_DEBUG
#define NB_DEBUG	0
#endif

#define NB_DEBUG_PRINTF(fmt, ...) do { \
	if (NB_DEBUG && NB_DEBUG_THIS) fprintf(stderr, "*** NB_DEBUG ***\t%*s % 4d    " fmt "\n", \
				10, __FILE__, __LINE__, __VA_ARGS__); \
} while (0);

#define NB_DEBUG_MSG(msg)		NB_DEBUG_PRINTF("%s", msg)
#define NB_DEBUG_EXPR(formatter, expr)	NB_DEBUG_PRINTF(#expr " = " formatter, (expr))
#define NB_DEBUG_TRACE			NB_DEBUG_PRINTF("Control reached %s", __func__)

#define TEMP_ASSERT(expr)		assert(expr)

#endif
