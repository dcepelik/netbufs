/*
 * parse:
 * Read a BIRD dump into well-defined data structures for benchmarking.
 */

#ifndef PARSER_H
#define PARSER_H

#include "benchmark.h"

#define PARSER_RT_INIT_SIZE	256
#define PARSER_STR_INIT_SIZE	8
#define PARSER_ARRAY_INIT_SIZE	4
#define PARSER_BUF_SIZE		4096

#include <stdio.h>
#include <stdlib.h>


struct parser
{
	size_t line_no;		/* current line number */
	size_t col_no;		/* current column number */
	struct buf *buf;	/* input buffer */
	char cur;		/* current character */
};


void parser_init(struct parser *p, struct buf *buf);
void parser_parse_rt(struct parser *p, struct rt *rt);
void parser_free(struct parser *p);

#endif
