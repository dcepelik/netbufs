#include "parse.h"
#include "serialize.h"
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


static size_t print_ipv4_nm(struct strbuf *sb, ipv4_t *ip, byte_t netmask)
{
	size_t len;
	len = print_ipv4(sb, ip);
	len += strbuf_printf(sb, "/%u", netmask);
	return len;
}


static void print_time(struct strbuf *sb, struct tm *tm)
{
	strbuf_printf(sb, "%u:%u:%u", tm->tm_hour, tm->tm_min, tm->tm_sec);
}


static void print_attr_bgp_origin(struct strbuf *sb, struct rte_attr *attr)
{
	strbuf_printf(sb, "\tBGP.origin: ");
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
	strbuf_printf(sb, "\tBGP.as_path: ");
	for (i = 0; attr->bgp_as_path[i] >= 0; i++)
		strbuf_printf(sb, "%u ", attr->bgp_as_path[i]);
	strbuf_putc(sb, '\n');
}


static void print_attr_bgp_next_hop(struct strbuf *sb, struct rte_attr *attr)
{
	strbuf_printf(sb, "\tBGP.next_hop: ");
	print_ipv4(sb, &attr->bgp_next_hop);
	strbuf_putc(sb, '\n');
}


static void print_attr_bgp_local_pref(struct strbuf *sb, struct rte_attr *attr)
{
	strbuf_printf(sb, "\tBGP.local_pref: %i\n", attr->bgp_local_pref);
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
	default:
		die("Unsupported attribute type.\n");
	}
}


static void print_rte(struct strbuf *sb, struct rte *rte)
{
	size_t i;

	i = print_ipv4_nm(sb, &rte->netaddr, rte->netmask);

	/* pad IP and netmask to the left */
	for (; i < 19; i++)
		strbuf_putc(sb, ' ');

	strbuf_puts(sb, " via ");
	print_ipv4(sb, &rte->gwaddr);
	strbuf_printf(sb, " on %s", rte->ifname);
	strbuf_puts(sb, " [uplink ");
	print_time(sb, &rte->uplink);
	strbuf_puts(sb, " from ");
	print_ipv4(sb, &rte->uplink_from);
	strbuf_puts(sb, "]");
	strbuf_puts(sb, " * (100/?) ...\n"); /* TODO */

	strbuf_printf(sb, "\tType: ");
	if (rte->type & RTE_TYPE_BGP)
		strbuf_printf(sb, "BGP ");
	if (rte->type & RTE_TYPE_UNICAST)
		strbuf_printf(sb, "unicast ");
	if (rte->type & RTE_TYPE_UNIV)
		strbuf_printf(sb, "univ ");
	strbuf_putc(sb, '\n');

	for (i = 0; i < rte->num_attrs; i++)
		print_rte_attr(sb, &rte->attrs[i]);
}


void serialize_bird(struct rt *rt)
{
	struct rte rte;
	struct strbuf sb;
	size_t i;

	strbuf_init(&sb, 1024);

	for (i = 0; i < rt->count; i++)
		print_rte(&sb, &rt->entries[i]);

	fprintf(stderr, strbuf_get_string(&sb));
}
