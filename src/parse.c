#include "parse.h"
#include "util.h"
#include "memory.h"
#include "debug.h"

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


static void error(struct parser *p, char *fmt, ...)
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


static void eat_ws(struct parser *p)
{
	char c;

	while ((c = parser_getc(p)) != '\0') {
		if (c == ' ' || c == '\t')
			continue;

		parser_ungetc(p, c);
		break;
	}
}


static void match(struct parser *p, char *str)
{
	char c;
	size_t i;

	for (i = 0; str[i] != '\0'; i++) {
		c = parser_getc(p);

		if (c == '\0' || c != str[i])
			error(p, "unexpected '%c', '%c' was expected\n",
				c, str[i]);
	}
}


static void match_ws(struct parser *p, char *keyword)
{
	eat_ws(p);
	match(p, keyword);
}


static void parser_skip_line(struct parser *p)
{
	char c;

	while ((c = parser_getc(p)) != '\0') {
		if (c == '\n')
			break;
	}
}


static void match_eol(struct parser *p)
{
	match(p, "\n");
}


static size_t parser_try_parse_int(struct parser *p, int *n)
{
	char c;
	size_t num_digits = 0;

	eat_ws(p);

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


static void parse_int(struct parser *p, int *n)
{
	size_t num_digits;

	num_digits = parser_try_parse_int(p, n);

	if (!num_digits)
		error(p, "integer was expected, got '%c'\n",
			parser_cur(p));
}


static char *parser_try_accum(struct parser *p, int (*predicate)(int c))
{
	size_t len = 0;
	size_t size = PARSER_STR_INIT_SIZE;
	char c;
	char *word = NULL;

	eat_ws(p);

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

	if (len == 0) {
		free(word);
		return NULL;
	}

	word[len] = '\0';
	return word;
}


static char *parser_accum(struct parser *p, int (*predicate)(int c))
{
	char *word = parser_try_accum(p, predicate);
	if (word == NULL)
		error(p, "string was expected\n");
	return word;
}


static char *parser_try_parse_word(struct parser *p)
{
	return parser_try_accum(p, isalnum);
}


static char *parse_word(struct parser *p)
{
	return parser_accum(p, isalnum);
}


static int is_valid_key_char(int c)
{
	return isalnum(c) || c == '.' || c == '_';
}


static char *parse_key(struct parser *p)
{
	return parser_accum(p, is_valid_key_char);
}


static void parse_ipv4(struct parser *p, ipv4_t *ip)
{
	int i;
	int b;

	ipv4_init(ip);
	eat_ws(p);

	for (i = 0; i < 4; i++) {
		parse_int(p, &b);
		ipv4_set_byte(ip, i, b);

		if (i < 3)
			match(p, ".");
	}
}


static void parse_time(struct parser *p, struct tm *tm)
{
	tm->tm_year = tm->tm_mon = tm->tm_mday = 0;
	tm->tm_isdst = -1;

	parse_int(p, &tm->tm_hour);
	match(p, ":");
	parse_int(p, &tm->tm_min);
	match(p, ":");
	parse_int(p, &tm->tm_sec);
}


static void parser_attr_ignore(struct parser *p, struct rte *rte)
{
	(void) p;
	(void) rte;

	parser_skip_line(p);
}


static void parse_attr_type(struct parser *p, struct rte *rte)
{
	char *word;

	rte->type = 0;
	while ((word = parser_try_parse_word(p)) != NULL) {
		if (strcmp(word, "BGP") == 0)
			rte->type |= RTE_TYPE_BGP;
		else if (strcmp(word, "unicast") == 0)
			rte->type |= RTE_TYPE_UNICAST;
		else if (strcmp(word, "univ") == 0)
			rte->type |= RTE_TYPE_UNIV;
		else
			error(p, "Unknown value of Type attribute: %s\n", word);
	}
	parser_skip_line(p);
}


static void parse_attr_bgp_origin(struct parser *p, struct rte *rte)
{
	struct rte_attr *attr;
	char *origin;

	attr = rte_add_attr(rte, RTE_ATTR_TYPE_BGP_ORIGIN);

	origin = parse_word(p);
	if (strcmp(origin, "IGP") == 0)
		attr->bgp_origin = BGP_ORIGIN_IGP;
	else if (strcmp(origin, "EGP") == 0)
		attr->bgp_origin = BGP_ORIGIN_EGP;
	else if (strcmp(origin, "Incomplete") == 0)
		attr->bgp_origin = BGP_ORIGIN_INCOMPLETE;
	else
		error(p, "unexpected '%s', 'IGP' or 'Incomplete' expected\n",
			origin);

	match_eol(p);
}


static void parse_attr_bgp_as_path(struct parser *p, struct rte *rte)
{
	int as_no;
	size_t num_items = 0;
	size_t size = PARSER_ARRAY_INIT_SIZE;
	struct rte_attr *attr;

	attr = rte_add_attr(rte, RTE_ATTR_TYPE_BGP_AS_PATH);
	attr->bgp_as_path = NULL;

array_resize:
	attr->bgp_as_path = realloc_safe(attr->bgp_as_path, size * sizeof(as_no));

	while (parser_try_parse_int(p, &as_no) > 0) {
		attr->bgp_as_path[num_items++] = as_no;

		if (num_items == size) {
			size *= 2;
			goto array_resize;
		}
	}

	/* there's room for the extra item */
	attr->bgp_as_path[num_items] = -1;

	match_eol(p);
}


static void parse_attr_bgp_next_hop(struct parser *p, struct rte *rte)
{
	struct rte_attr *attr;
	attr = rte_add_attr(rte, RTE_ATTR_TYPE_BGP_NEXT_HOP);
	parse_ipv4(p, &attr->bgp_next_hop);
	match_eol(p);
}


static void parse_attr_bgp_local_pref(struct parser *p, struct rte *rte)
{
	struct rte_attr *attr;
	attr = rte_add_attr(rte, RTE_ATTR_TYPE_BGP_LOCAL_PREF);
	parse_int(p, &attr->bgp_local_pref);
	match_eol(p);
}


static void parse_attr_bgp_community(struct parser *p, struct rte *rte)
{
	struct bgp_cflag *cflag;
	struct rte_attr *attr;

	attr = rte_add_attr(rte, RTE_ATTR_TYPE_BGP_COMMUNITY);
	attr->cflags = array_new(2, sizeof(*attr->cflags));

	eat_ws(p);
	while (parser_peek(p) == '(') {
		attr->cflags = array_push(attr->cflags, 1);
		cflag = &attr->cflags[array_size(attr->cflags) - 1];

		match(p, "(");
		parse_int(p, &cflag->flag);
		match(p, ",");
		parse_int(p, &cflag->as_no);
		match(p, ")");

		eat_ws(p);
	}

	match_eol(p);
}


static void parse_attr_bgp_aggregator(struct parser *p, struct rte *rte)
{
	struct rte_attr *attr;

	attr = rte_add_attr(rte, RTE_ATTR_TYPE_BGP_AGGREGATOR);
	parse_ipv4(p, &attr->aggr.ip);
	match_ws(p, "AS");
	parse_int(p, &attr->aggr.as_no);


	match_eol(p);
}


static void parse_rte_attrs(struct parser *p, struct rte *rte)
{
	char c;
	char *key;
	size_t i;
	struct rte_attr *attr;
	bool handled;

	struct {
		char *key;
		void (*handler)(struct parser *p, struct rte *rte);
	} handlers[] = {
		 { .key = "Type", .handler = parse_attr_type },
		 { .key = "BGP.origin", .handler = parse_attr_bgp_origin },
		 { .key = "BGP.as_path", .handler = parse_attr_bgp_as_path },
		 { .key = "BGP.next_hop", .handler = parse_attr_bgp_next_hop },
		 { .key = "BGP.local_pref", .handler = parse_attr_bgp_local_pref },
		 { .key = "BGP.community", .handler = parse_attr_bgp_community },
		 { .key = "BGP.aggregator", .handler = parse_attr_bgp_aggregator },
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

		key = parse_key(p);
		eat_ws(p);

		if (parser_peek(p) == '[')
			match(p, "[t]");
		else
			match(p, ":");

		handled = false;
		for (i = 0; i <= ARRAY_SIZE(handlers); i++) {
			if (strcmp(key, handlers[i].key) == 0) {
				handlers[i].handler(p, rte);
				handled = true;
				break;
			}
		}

		if (!handled)
			error(p, "attr '%s' not supported\n", key);
	}
}


static bool parse_rte(struct parser *p, struct rte *rte)
{
	rte->attrs = NULL;
	rte->attrs_size = 0;
	rte->num_attrs = 0;
	rte->ifname = NULL;

	if (parser_peek(p) == '\0')
		return false;

	parse_ipv4(p, &rte->netaddr);
	match(p, "/");
	parse_int(p, &rte->netmask);
	eat_ws(p);

	match_ws(p, "via");
	parse_ipv4(p, &rte->gwaddr);

	match_ws(p, "on");
	rte->ifname = parse_word(p);

	match_ws(p, "[");

	if (parser_peek(p) == 's') {
		match_ws(p, "static1");
		eat_ws(p);
		parse_time(p, &rte->uplink);
	}
	else {
		match_ws(p, "uplink");
		eat_ws(p);
		parse_time(p, &rte->uplink);

		match_ws(p, "from");
		parse_ipv4(p, &rte->uplink_from);
	}

	match_ws(p, "]");
	
	/* TODO I don't know the meaning of remaining data */
	parser_skip_line(p);

	parse_rte_attrs(p, rte);

	return true;
}


void parser_parse_rt(struct parser *p, struct rt *rt)
{
	match(p, "BIRD 1.5.0 ready.");
	match_eol(p);

	rt->entries = NULL;
	rt->count = 0;
	rt->size = PARSER_RT_INIT_SIZE;

resize_table:
	rt->entries = realloc_safe(rt->entries, rt->size * sizeof(rt->entries[0]));

	while (parse_rte(p, &rt->entries[rt->count])) {
		rt->count++;
		if (rt->count == rt->size) {
			rt->size *= 2;
			goto resize_table;
		}
	}
}
