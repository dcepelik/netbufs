#include "parser.h"
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


void parser_init(struct parser *p, struct buf *buf)
{
	p->buf = buf;
	p->line_no = 1;
	p->col_no = 1;
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
}


static char parser_getc(struct parser *p)
{
	p->cur = buf_getc(p->buf);
	if (p->cur == '\n') {
		p->line_no++;
		p->col_no = 0;
	}

	p->col_no++;
	return p->cur;
}


static void parser_ungetc(struct parser *p, char c)
{
	buf_ungetc(p->buf, c);
}


static char parser_peek(struct parser *p)
{
	return buf_peek(p->buf);
}


static char parser_cur(struct parser *p)
{
	return p->cur;
}


static void eat_ws(struct parser *p)
{
	char c;

	while ((c = parser_getc(p)) != BUF_EOF) {
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

		if (c == BUF_EOF || c != str[i])
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

	while ((c = parser_getc(p)) != BUF_EOF) {
		if (c == '\n')
			break;
	}
}


static void match_eol(struct parser *p)
{
	match(p, "\n");
}


static size_t parser_try_parse_int(struct parser *p, uint32_t *n)
{
	char c;
	size_t num_digits = 0;

	eat_ws(p);

	*n = 0;
	while ((c = parser_getc(p)) != BUF_EOF) {
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


static void parse_u32(struct parser *p, uint32_t *n)
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

	while ((c = parser_getc(p)) != BUF_EOF) {
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


static char *parse_version_str(struct parser *p)
{
	return parse_key(p); /* same rules as for keys apply here */
}


static int is_not_eol(int c)
{
	return c != '\n' && c != BUF_EOF;
}


static char *parse_line(struct parser *p)
{
	eat_ws(p);
	return parser_try_accum(p, is_not_eol);
}


static void parse_ipv4(struct parser *p, ipv4_t *ip)
{
	int i;
	uint32_t b;

	ipv4_init(ip);
	eat_ws(p);

	for (i = 0; i < 4; i++) {
		parse_u32(p, &b);
		ipv4_set_byte(ip, i, b);

		if (i < 3)
			match(p, ".");
	}
}


static void parse_time(struct parser *p, struct tm *tm)
{
	uint32_t hour;
	uint32_t min;
	uint32_t sec;

	tm->tm_year = tm->tm_mon = tm->tm_mday = 0;
	tm->tm_isdst = -1;

	parse_u32(p, &hour);
	match(p, ":");
	parse_u32(p, &min);
	match(p, ":");
	parse_u32(p, &sec);

	assert(hour >= 0 && min >= 0 && sec >= 0);

	tm->tm_hour = hour;
	tm->tm_min = min;
	tm->tm_sec = sec;
}


static struct rte_attr *parse_attr_type(struct parser *p, struct rte *rte)
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
		else if (strcmp(word, "static") == 0)
			rte->type |= RTE_TYPE_STATIC;
		else
			error(p, "Unknown value of Type attribute: %s\n", word);
	}

	match_eol(p);
	return NULL;
}


static struct rte_attr *parse_attr_bgp_origin(struct parser *p, struct rte *rte)
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
	return attr;
}


static struct rte_attr *parse_attr_bgp_as_path(struct parser *p, struct rte *rte)
{
	uint32_t as_no;
	size_t num_items = 0;
	size_t size = PARSER_ARRAY_INIT_SIZE;
	struct rte_attr *attr;
	char c;

	attr = rte_add_attr(rte, RTE_ATTR_TYPE_BGP_AS_PATH);
	attr->bgp_as_path = NULL;

array_resize:
	attr->bgp_as_path = realloc_safe(attr->bgp_as_path, size * sizeof(as_no));

	while (parser_try_parse_int(p, &as_no) > 0) {
write_as_no:
		attr->bgp_as_path[num_items++] = as_no;

		if (num_items == size) {
			size *= 2;
			goto array_resize;
		}
	}

	eat_ws(p);

	c = parser_getc(p);
	if (c == '{' || c == '}') {
		as_no = (c == '{') ? AS_PATH_FLAG_LBRACE : AS_PATH_FLAG_RBRACE;
		goto write_as_no;
	}
	else {
		parser_ungetc(p, c);
	}

	/* there's room for the extra item */
	attr->bgp_as_path[num_items] = AS_PATH_FLAG_END;

	match_eol(p);
	return attr;
}


static struct rte_attr *parse_attr_bgp_next_hop(struct parser *p, struct rte *rte)
{
	struct rte_attr *attr;

	attr = rte_add_attr(rte, RTE_ATTR_TYPE_BGP_NEXT_HOP);
	parse_ipv4(p, &attr->bgp_next_hop);
	match_eol(p);
	return attr;
}


static struct rte_attr *parse_attr_bgp_local_pref(struct parser *p, struct rte *rte)
{
	struct rte_attr *attr;

	attr = rte_add_attr(rte, RTE_ATTR_TYPE_BGP_LOCAL_PREF);
	parse_u32(p, &attr->bgp_local_pref);
	match_eol(p);
	return attr;
}


static struct rte_attr *parse_attr_bgp_community(struct parser *p, struct rte *rte)
{
	struct rte_attr *attr;
	struct bgp_cflag *cflag;

	attr = rte_add_attr(rte, RTE_ATTR_TYPE_BGP_COMMUNITY);
	attr->cflags = array_new(2, sizeof(*attr->cflags));

	eat_ws(p);
	while (parser_peek(p) == '(') {
		attr->cflags = array_push(attr->cflags, 1);
		cflag = &attr->cflags[array_size(attr->cflags) - 1];

		match(p, "(");
		parse_u32(p, &cflag->flag);
		match(p, ",");
		parse_u32(p, &cflag->as_no);
		match(p, ")");

		eat_ws(p);
	}

	match_eol(p);
	return attr;
}


static struct rte_attr *parse_attr_bgp_aggregator(struct parser *p, struct rte *rte)
{
	struct rte_attr *attr;

	attr = rte_add_attr(rte, RTE_ATTR_TYPE_BGP_AGGREGATOR);
	parse_ipv4(p, &attr->aggr.ip);
	match_ws(p, "AS");
	parse_u32(p, &attr->aggr.as_no);

	match_eol(p);
	return attr;
}


static struct rte_attr *parse_attr_other(struct parser *p, char *key, struct rte *rte)
{
	struct rte_attr *attr;

	attr = rte_add_attr(rte, RTE_ATTR_TYPE_OTHER);
	attr->other_attr.key = key;
	attr->other_attr.value = parse_line(p);

	match_eol(p);
	return attr;
}


static void parse_rte_attrs(struct parser *p, struct rte *rte)
{
	struct rte_attr *attr;
	char *key;
	bool handled;
	bool tflag;
	char c;
	size_t i;

	struct {
		char *key;
		struct rte_attr *(*handler)(struct parser *p, struct rte *rte);
	} handlers[] = {
		 { .key = "Type", .handler = parse_attr_type },
		 { .key = "BGP.origin", .handler = parse_attr_bgp_origin },
		 { .key = "BGP.as_path", .handler = parse_attr_bgp_as_path },
		 { .key = "BGP.next_hop", .handler = parse_attr_bgp_next_hop },
		 { .key = "BGP.local_pref", .handler = parse_attr_bgp_local_pref },
		 { .key = "BGP.community", .handler = parse_attr_bgp_community },
		 { .key = "BGP.aggregator", .handler = parse_attr_bgp_aggregator },
	};

	while ((c = parser_getc(p)) != BUF_EOF) {
		if (c != '\t') {
			parser_ungetc(p, c);
			break;
		}

		key = parse_key(p);
		eat_ws(p);

		if (parser_peek(p) == '[') {
			match(p, "[t]:");
			tflag = true;
		}
		else {
			match(p, ":");
			tflag = false;
		}
		eat_ws(p);

		handled = false;
		for (i = 0; i < ARRAY_SIZE(handlers); i++) {
			if (strcmp(key, handlers[i].key) == 0) {
				attr = handlers[i].handler(p, rte);
				handled = true;
				break;
			}
		}

		if (!handled)
			attr = parse_attr_other(p, key, rte);

		if (!attr && tflag)
			error(p, "[t]-flag on required attribute unsupported\n");

		if (attr)
			attr->tflag = tflag;
	}
}


static void parse_as_name(struct parser *p, struct rte *rte)
{
	char c;

	if (parser_peek(p) == 'A') {
		match(p, "AS");
		parse_u32(p, &rte->as_no);
		rte->as_no_valid = true;
	}
	else {
		rte->as_no_valid = false;
	}

	c = parser_getc(p);
	switch (c) {
	case 'i':
		rte->src = RTE_SRC_INTERNAL;
		break;
	case 'e':
		rte->src = RTE_SRC_EXTERNAL;
		break;
	case 'u':
		rte->src = RTE_SRC_U;
		break;
	case '?':
		rte->src = RTE_SRC_WHO_KNOWS;  
		break;
	default:
		error(p, "Unknown AS number suffix: '%c'\n", c);
	}
}


static bool parse_rte(struct parser *p, struct rte *rte)
{
	char c;
	char *as_name;

	rte->attrs = NULL;
	rte->attrs_size = 0;
	rte->num_attrs = 0;
	rte->ifname = NULL;
	rte->uplink_from_valid = false;

	if (parser_peek(p) == BUF_EOF)
		return false;

	parse_ipv4(p, &rte->netaddr);
	match(p, "/");
	parse_u32(p, &rte->netmask);
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

		rte->uplink_from_valid = true;
		match_ws(p, "from");
		parse_ipv4(p, &rte->uplink_from);
	}

	match_ws(p, "]");
	
	/* NOTE: I'm skipping the "* (100/?)" part here */
	while ((c = parser_getc(p)) != BUF_EOF) {
		if (c == '\n') {
			parser_ungetc(p, c);
			break;
		}
		if (c == '[')
			break;
	}

	if (c == '[') {
		parse_as_name(p, rte);
		match(p, "]");
	}
	match_eol(p);

	parse_rte_attrs(p, rte);

	return true;
}


void parser_parse_rt(struct parser *p, struct rt *rt)
{
	match(p, "BIRD ");
	rt->version_str = parse_version_str(p);
	match(p, " ready.");
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
