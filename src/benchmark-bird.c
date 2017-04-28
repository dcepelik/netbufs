#include "benchmark.h"
#include "strbuf.h"
#include "util.h"


static size_t print_ipv4(struct strbuf *sb, ipv4_t *ip)
{
	size_t i;
	size_t len = 0;

	for (i = 0; i < 4; i++) {
		if (i > 0)
			len += strbuf_putc(sb, '.');
		len += strbuf_printf(sb, "%u", ipv4_get_byte(ip, i));
	}
	return len;
}


static size_t print_ipv4_net(struct strbuf *sb, ipv4_t *ip, byte_t netmask)
{
	size_t len;
	len = print_ipv4(sb, ip);
	len += strbuf_printf(sb, "/%u", netmask);
	return len;
}


static void print_tm(struct strbuf *sb, struct tm *tm)
{
	strbuf_printf(sb, "%02u:%02u:%02u", tm->tm_hour, tm->tm_min, tm->tm_sec);
}


static const char *attr_key(struct rte_attr *attr)
{
	switch (attr->type) {
	case RTE_ATTR_TYPE_BGP_AS_PATH:
		return "BGP.as_path";
	case RTE_ATTR_TYPE_BGP_ORIGIN:
		return "BGP.origin";
	case RTE_ATTR_TYPE_BGP_NEXT_HOP:
		return "BGP.next_hop";
	case RTE_ATTR_TYPE_BGP_LOCAL_PREF:
		return "BGP.local_pref";
	case RTE_ATTR_TYPE_BGP_COMMUNITY:
		return "BGP.community";
	case RTE_ATTR_TYPE_BGP_AGGREGATOR:
		return "BGP.aggregator";
	case RTE_ATTR_TYPE_OTHER:
		return (const char *)attr->other_attr.key;
	}
	return NULL;
}


static void print_attr_key(struct strbuf *sb, struct rte_attr *attr)
{
	strbuf_printf(sb, "\t%s%s: ", attr_key(attr), attr->tflag ? " [t]" : "");
}


static void print_attr_bgp_origin(struct strbuf *sb, struct rte_attr *attr)
{
	print_attr_key(sb, attr);
	switch (attr->bgp_origin) {
	case BGP_ORIGIN_IGP:
		strbuf_printf(sb, "IGP");
		break;
	case BGP_ORIGIN_EGP:
		strbuf_printf(sb, "EGP");
		break;
	case BGP_ORIGIN_INCOMPLETE:
		strbuf_printf(sb, "Incomplete");
		break;
	}
	strbuf_putc(sb, '\n');
}


static void print_attr_bgp_as_path(struct strbuf *sb, struct rte_attr *attr)
{
	size_t i;

	print_attr_key(sb, attr);
	for (i = 0; attr->bgp_as_path[i] != AS_PATH_FLAG_END; i++) {
		switch (attr->bgp_as_path[i]) {
		case AS_PATH_FLAG_LBRACE:
			strbuf_puts(sb, "{ ");
			break;
		case AS_PATH_FLAG_RBRACE:
			strbuf_puts(sb, "} ");
			break;
		default:
			strbuf_printf(sb, "%u ", attr->bgp_as_path[i]);
		}
	}
	strbuf_putc(sb, '\n');
}


static void print_attr_bgp_next_hop(struct strbuf *sb, struct rte_attr *attr)
{
	print_attr_key(sb, attr);
	print_ipv4(sb, &attr->bgp_next_hop);
	strbuf_putc(sb, '\n');
}


static void print_attr_bgp_local_pref(struct strbuf *sb, struct rte_attr *attr)
{
	print_attr_key(sb, attr);
	strbuf_printf(sb, "%i\n", attr->bgp_local_pref);
}


static void print_attr_bgp_community(struct strbuf *sb, struct rte_attr *attr)
{
	size_t i;
	print_attr_key(sb, attr);
	for (i = 0; i < array_size(attr->cflags); i++)
		strbuf_printf(sb, "(%u,%u) ", attr->cflags[i].flag, attr->cflags[i].as_no);
	strbuf_putc(sb, '\n');
}


static void print_attr_bgp_aggregator(struct strbuf *sb, struct rte_attr *attr)
{
	print_attr_key(sb, attr);
	print_ipv4(sb, &attr->aggr.ip);
	strbuf_printf(sb, " AS%u\n", attr->aggr.as_no);
}


static void print_attr_other(struct strbuf *sb, struct rte_attr *attr)
{
	char *value;
	
	value = attr->other_attr.value;
	if (!value)
		value = ""; /* avoid (null) output by printf */

	print_attr_key(sb, attr);
	strbuf_puts(sb, value);
	strbuf_putc(sb, '\n');
}


static void print_rte_attr(struct strbuf *sb, struct rte_attr *attr)
{
	switch (attr->type) {
	case RTE_ATTR_TYPE_BGP_ORIGIN:
		print_attr_bgp_origin(sb, attr);
		break;
	case RTE_ATTR_TYPE_BGP_AS_PATH:
		print_attr_bgp_as_path(sb, attr);
		break;
	case RTE_ATTR_TYPE_BGP_NEXT_HOP:
		print_attr_bgp_next_hop(sb, attr);
		break;
	case RTE_ATTR_TYPE_BGP_LOCAL_PREF:
		print_attr_bgp_local_pref(sb, attr);
		break;
	case RTE_ATTR_TYPE_BGP_COMMUNITY:
		print_attr_bgp_community(sb, attr);
		break;
	case RTE_ATTR_TYPE_BGP_AGGREGATOR:
		print_attr_bgp_aggregator(sb, attr);
		break;
	case RTE_ATTR_TYPE_OTHER:
		print_attr_other(sb, attr);
		break;
	default:
		die("Unsupported attribute type.\n");
	}
}


static char rte_src_to_char(enum rte_src src)
{
	switch (src) {
	case RTE_SRC_INTERNAL:
		return 'i';
	case RTE_SRC_EXTERNAL:
		return 'e';
	case RTE_SRC_U:
		return 'u';
	case RTE_SRC_WHO_KNOWS:
		return '?';
	}

	die("Unknown enum rte_src value: %i\n", src);
	return '\0'; /* shut GCC up */
}


static void print_rte_src_as(struct strbuf *sb, struct rte *rte)
{
	char src_flag;
	
	src_flag = rte_src_to_char(rte->src);
	if (rte->as_no_valid)
		strbuf_printf(sb, "[AS%u%c]", rte->as_no, src_flag);
	else
		strbuf_printf(sb, "[%c]", src_flag);
}


static void print_attr_type(struct strbuf *sb, struct rte *rte)
{
	/* Type attribute, write flags */
	strbuf_printf(sb, "\tType: ");
	if (rte->type & RTE_TYPE_BGP)
		strbuf_printf(sb, "BGP ");
	if (rte->type & RTE_TYPE_UNICAST)
		strbuf_printf(sb, "unicast ");
	if (rte->type & RTE_TYPE_UNIV)
		strbuf_printf(sb, "univ ");
	if (rte->type & RTE_TYPE_STATIC)
		strbuf_printf(sb, "static ");
	strbuf_putc(sb, '\n');
}


static void print_rte(struct strbuf *sb, struct rte *rte)
{
	size_t i;

	/* print dst network (left-align with spaces) */
	i = print_ipv4_net(sb, &rte->netaddr, rte->netmask);
	for (; i <= 17; i++)
		strbuf_putc(sb, ' ');

	/* via ... */
	strbuf_puts(sb, " via ");
	print_ipv4(sb, &rte->gwaddr);
	strbuf_printf(sb, " on %s", rte->ifname);

	/* [uplink ... from ...] */
	strbuf_puts(sb, " [uplink ");
	print_tm(sb, &rte->uplink);
	if (rte->uplink_from_valid) {
		strbuf_puts(sb, " from ");
		print_ipv4(sb, &rte->uplink_from);
	}
	strbuf_putc(sb, ']');

	 /* this part is a fixed string */
	strbuf_puts(sb, " * (100/?) ");

	/* [AS...] */
	print_rte_src_as(sb, rte);
	strbuf_putc(sb, '\n');

	/* Type system attribute */
	print_attr_type(sb, rte);

	/* all other attributes */
	for (i = 0; i < rte->num_attrs; i++)
		print_rte_attr(sb, &rte->attrs[i]);
}


void serialize_bird(struct rt *rt)
{
	struct rte rte;
	struct strbuf sb;
	size_t i;

	strbuf_init(&sb, 1024);

	strbuf_printf(&sb, "BIRD %s ready.\n", rt->version_str);
	for (i = 0; i < rt->count; i++)
		print_rte(&sb, &rt->entries[i]);

	fprintf(stderr, strbuf_get_string(&sb));
}


struct rt *deserialize_bird(struct buf *buf)
{
	return NULL;
}
