#ifndef PARSE_H
#define PARSE_H

#define PARSER_RT_INIT_SIZE	256
#define PARSER_STR_INIT_SIZE	8
#define PARSER_ARRAY_INIT_SIZE	4
#define PARSER_BUF_SIZE		4096

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef uint32_t ipv4_t;


enum bgp_origin
{
	BGP_ORIGIN_IGP,
	BGP_ORIGIN_EGP,
	BGP_ORIGIN_INCOMPLETE,
};


struct rte
{
	ipv4_t netaddr;
	int netmask;
	ipv4_t gwaddr;
	char *ifname;
	struct tm uplink;
	ipv4_t uplink_from;

	struct {
		struct {
			int *as_path;
			enum bgp_origin origin;
		} bgp;
	} attrs;
};


struct rt
{
	struct rte *entries;	/* entries of the routing table */
	size_t size;		/* size of *entries */
	size_t count;		/* number of entries in *entries */
};


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
void parser_free(struct parser *p);

void parser_parse_rt(struct parser *p, struct rt *rt);

#endif
