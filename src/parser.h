/*
 * parse:
 * Read a BIRD dump into well-defined data structures for benchmarking.
 *
 * TODO Make this reusable.
 */

#ifndef PARSER_H
#define PARSER_H

#include "benchmark.h"

#include <stdio.h>
#include <stdlib.h>


struct parser
{
	size_t line_no;		/* current line number */
	size_t col_no;		/* current column number */
	struct nb_buf *buf;	/* input buffer */
	char cur;		/* current character */
};


void parser_init(struct parser *p, struct nb_buf *buf);
struct rt *parser_parse_rt(struct parser *p);
void parser_free(struct parser *p);

#endif
