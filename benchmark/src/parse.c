#include "parse.h"
#include "util.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>


static void parser_fill_buffer(struct parser *p)
{
	p->avail = fread(p->buf, sizeof(char), PARSER_BUF_SIZE, p->file);
	p->offset = 0;
}


void parser_init(struct parser *p, char *filename)
{
	p->file = fopen(filename, "r");
	if (!p->file) {
		die("cannot open input file '%s' for reading: %s\n",
			filename, strerror(errno));
	}

	p->filename = filename;
	p->line_no = 1;
	p->col_no = 1;
	p->buf = malloc(PARSER_BUF_SIZE);
	p->unget = '\0';

	parser_fill_buffer(p);
}


static void parser_error(struct parser *p, char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	fprintf(stderr, "parse_error: %lu:%lu: ", p->line_no, p->col_no - 1);
	vfprintf(stderr, fmt, args);

	va_end(args);
	exit(EXIT_FAILURE);
}


void parser_free(struct parser *p)
{
	fclose(p->file);
}


static char parser_getc(struct parser *p)
{
	char c;

	if (p->unget != '\0') {
		c = p->unget;
		p->unget = '\0';
		goto out;
	}

	if (p->offset >= p->avail) {
		parser_fill_buffer(p);

		if (p->avail == 0)
			return '\0';
	}

	c = p->buf[p->offset++];

	if (c == '\n') {
		p->line_no++;
		p->col_no = 0;
	}

	p->col_no++;

out:
	return p->cur = c;
}


static void parser_ungetc(struct parser *p, char c)
{
	assert(p->unget == '\0');

	if (c != '\0') {
		if (c == '{') {
			fprintf(stderr, "%lu\n", p->line_no);
			assert(false);
		}

		p->unget = c;
	}
}


static char parser_peek(struct parser *p)
{
	char c;

	c = parser_getc(p);
	parser_ungetc(p, c);

	return c;
}


static char parser_cur(struct parser *p)
{
	return p->cur;
}


static void parser_eat_ws(struct parser *p)
{
	char c;

	while ((c = parser_getc(p)) != '\0') {
		if (c == ' ' || c == '\t')
			continue;

		parser_ungetc(p, c);
		break;
	}
}


static void parser_match(struct parser *p, char *str)
{
	char c;
	size_t i;

	for (i = 0; str[i] != '\0'; i++) {
		c = parser_getc(p);

		if (c == '\0' || c != str[i])
			parser_error(p, "unexpected '%c', '%c' was expected\n",
				c, str[i]);
	}
}


static void parser_match_kw(struct parser *p, char *keyword)
{
	parser_eat_ws(p);
	parser_match(p, keyword);
}


static void parser_skip_line(struct parser *p)
{
	char c;

	while ((c = parser_getc(p)) != '\0') {
		if (c == '\n')
			break;
	}
}


static void parser_match_eol(struct parser *p)
{
	parser_match(p, "\n");
}


static size_t parser_try_parse_int(struct parser *p, int *n)
{
	char c;
	size_t num_digits = 0;

	parser_eat_ws(p);

	*n = 0;
	while ((c = parser_getc(p)) != '\0') {
		if (c >= '0' && c <= '9') {
			*n = 10 * (*n) + (c - '0');
			num_digits++;
		}
		else {
			parser_ungetc(p, c);
			break;
		}
	}

	return num_digits;
}


static void parser_parse_int(struct parser *p, int *n)
{
	size_t num_digits;

	num_digits = parser_try_parse_int(p, n);

	if (!num_digits)
		parser_error(p, "integer was expected, got '%c'\n",
			parser_cur(p));
}


static char *parser_accum(struct parser *p, int (*predicate)(int c))
{
	size_t len = 0;
	size_t size = PARSER_STR_INIT_SIZE;
	char c;
	char *word = NULL;

	parser_eat_ws(p);

resize_word:
	word = realloc_safe(word, size);

	while ((c = parser_getc(p)) != '\0') {
		if (predicate(c)) {
			word[len++] = c;
			if (len == size) {
				size *= 2;
				goto resize_word;
			}
		}
		else {
			parser_ungetc(p, c);
			break;
		}
	}

	if (len == 0)
		parser_error(p, "string was expected\n");

	word[len] = '\0';
	return word;
}


static char *parser_parse_word(struct parser *p)
{
	return parser_accum(p, isalnum);
}


static int is_valid_key_char(int c)
{
	return isalnum(c) || c == '.' || c == '_';
}


static char *parser_parse_key(struct parser *p)
{
	return parser_accum(p, is_valid_key_char);
}


static void parser_parse_ipv4(struct parser *p, ipv4_t *ip)
{
	int i;
	int b;

	parser_eat_ws(p);

	for (i = 1; i <= 4; i++) {
		parser_parse_int(p, &b);
		*ip <<= 8;
		*ip += b;

		if (i < 4)
			parser_match(p, ".");
	}
}


static void parser_parse_time(struct parser *p, struct tm *tm)
{
	tm->tm_year = tm->tm_mon = tm->tm_mday = 0;
	tm->tm_isdst = -1;

	parser_parse_int(p, &tm->tm_hour);
	parser_match(p, ":");
	parser_parse_int(p, &tm->tm_min);
	parser_match(p, ":");
	parser_parse_int(p, &tm->tm_sec);
}


static void parser_attr_ignore(struct parser *p, struct rte *rte)
{
	(void) p;
	(void) rte;

	parser_skip_line(p);
}


static void parser_parse_attr_type(struct parser *p, struct rte *rte)
{
	parser_skip_line(p);
}


static void parser_parse_attr_bgp_origin(struct parser *p, struct rte *rte)
{
	char *origin;

	origin = parser_parse_word(p);
	if (strcmp(origin, "IGP") == 0)
		rte->attrs.bgp.origin = BGP_ORIGIN_IGP;
	else if (strcmp(origin, "EGP") == 0)
		rte->attrs.bgp.origin = BGP_ORIGIN_EGP;
	else if (strcmp(origin, "Incomplete") == 0)
		rte->attrs.bgp.origin = BGP_ORIGIN_INCOMPLETE;
	else
		parser_error(p, "unexpected '%s', 'IGP' or 'Incomplete' expected\n",
			origin);

	parser_match_eol(p);
}


static void parser_parse_attr_bgp_as_path(struct parser *p, struct rte *rte)
{
//	int as_no;
//	size_t num_items = 0;
//	size_t size = PARSER_ARRAY_INIT_SIZE;
//
//array_resize:
//	rte->attrs.bgp.as_path = realloc_safe(rte->attrs.bgp.as_path,
//		size * sizeof(as_no));
//
//	while (parser_try_parse_int(p, &as_no) > 0) {
//		rte->attrs.bgp.as_path[num_items++] = as_no;
//
//		if (num_items == size) {
//			size *= 2;
//			goto array_resize;
//		}
//	}
//
//	parser_match_eol(p);
	parser_attr_ignore(p, rte);
}


static void parser_parse_rte_attrs(struct parser *p, struct rte *rte)
{
	char c;
	char *key;
	size_t i;
	bool handled;

	struct {
		char *key;
		void (*handler)(struct parser *p, struct rte *rte);
	} attrs[] = {
		 { .key = "Type", .handler = parser_parse_attr_type },
		 { .key = "BGP.origin", .handler = parser_parse_attr_bgp_origin },
		 { .key = "BGP.as_path", .handler = parser_parse_attr_bgp_as_path },
		 { .key = "BGP.next_hop", .handler = parser_attr_ignore },
		 { .key = "BGP.local_pref", .handler = parser_attr_ignore },
		 { .key = "BGP.community", .handler = parser_attr_ignore },
		 { .key = "BGP.aggregator", .handler = parser_attr_ignore },
		 { .key = "BGP.atomic_aggr", .handler = parser_attr_ignore },
		 { .key = "BGP.med", .handler = parser_attr_ignore },
		 { .key = "BGP.80", .handler = parser_attr_ignore },
		 { .key = "BGP.20", .handler = parser_attr_ignore },
		 { .key = "BGP.15", .handler = parser_attr_ignore },
		 { .key = "BGP.ext_community", .handler = parser_attr_ignore },
	};

	while ((c = parser_getc(p)) != '\0') {
		if (c != '\t') {
			parser_ungetc(p, c);
			break;
		}

		key = parser_parse_key(p);

		parser_eat_ws(p);

		if (parser_peek(p) == '[')
			parser_match(p, "[t]");
		else
			parser_match(p, ":");

		handled = false;
		for (i = 0; i <= ARRAY_SIZE(attrs); i++) {
			if (strcmp(key, attrs[i].key) == 0) {
				attrs[i].handler(p, rte);
				handled = true;
				break;
			}
		}

		if (!handled)
			parser_error(p, "attr '%s' not supported\n", key);
	}
}


static bool parser_parse_rte(struct parser *p, struct rte *rte)
{
	rte->attrs.bgp.as_path = NULL;

	if (parser_peek(p) == '\0')
		return false;

	parser_parse_ipv4(p, &rte->netaddr);
	parser_match(p, "/");
	parser_parse_int(p, &rte->netmask);
	parser_eat_ws(p);

	parser_match_kw(p, "via");
	parser_parse_ipv4(p, &rte->gwaddr);

	parser_match_kw(p, "on");
	rte->ifname = parser_parse_word(p);

	parser_match_kw(p, "[");

	if (parser_peek(p) == 's') {
		parser_match_kw(p, "static1");
		parser_eat_ws(p);
		parser_parse_time(p, &rte->uplink);
	}
	else {
		parser_match_kw(p, "uplink");
		parser_eat_ws(p);
		parser_parse_time(p, &rte->uplink);

		parser_match_kw(p, "from");
		parser_parse_ipv4(p, &rte->uplink_from);
	}

	parser_match_kw(p, "]");
	
	/* TODO I don't know the meaning of remaining data */
	parser_skip_line(p);

	parser_parse_rte_attrs(p, rte);

	return true;
}


void parser_parse_rt(struct parser *p, struct rt *rt)
{
	parser_match(p, "BIRD 1.5.0 ready.");
	parser_match_eol(p);

	rt->entries = NULL;
	rt->count = 0;
	rt->size = PARSER_RT_INIT_SIZE;

resize_table:
	rt->entries = realloc_safe(rt->entries, rt->size * sizeof(rt->entries[0]));

	while (parser_parse_rte(p, &rt->entries[rt->count++])) {
		if (rt->count == rt->size) {
			rt->size *= 2;
			goto resize_table;
		}
	}
}
