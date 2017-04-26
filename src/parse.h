/*
 * parse:
 * Read a BIRD dump into well-defined data structures for benchmarking.
 */

#ifndef PARSE_H
#define PARSE_H

#include "serialize.h"

#define PARSER_RT_INIT_SIZE	256
#define PARSER_STR_INIT_SIZE	8
#define PARSER_ARRAY_INIT_SIZE	4
#define PARSER_BUF_SIZE		4096

#include <stdio.h>
#include <stdlib.h>


struct parser
{
	char *filename;		/* filename of an open file */
	FILE *file;		/* open file */
	size_t line_no;		/* current line number */
	size_t col_no;		/* current column number */
	char *buf;		/* input buffer */
	size_t offset;		/* offset within buf */
	size_t avail;		/* length of content in buf */
	char unget;		/* character to return on next parser_getc */
	char cur;		/* current character */
};


void parser_init(struct parser *p, char *filename);
void parser_parse_rt(struct parser *p, struct rt *rt);
void parser_free(struct parser *p);

#endif
