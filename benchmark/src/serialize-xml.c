#include "benchmark.h"
#include "strbuf.h"
#include <inttypes.h>
#include <libxml/tree.h>
#include "debug.h"

#define NB_DEBUG_THIS	1


struct strbuf xml_buf;


#define print_attr(name, fmt, value) \
	strbuf_printf(&xml_buf, " %s=\"" fmt "\"", name, value);

#define print_attr_ip(name, ip) \
	strbuf_printf(&xml_buf, " %s=\"%u.%u.%u.%u\"", \
		name, \
		ipv4_get_byte(&ip, 0), \
		ipv4_get_byte(&ip, 1), \
		ipv4_get_byte(&ip, 2), \
		ipv4_get_byte(&ip, 3));

#define print_attr_time(name, time) \
	strbuf_printf(&xml_buf, " %s=\"%u:%u:%u\"", \
		name, \
		(time)->tm_hour, \
		(time)->tm_min, \
		(time)->tm_sec);

#define print_attr_bool(name, b) \
	strbuf_printf(&xml_buf, " %s=\"%s\"", \
	name, \
	(b) ? "true" : "false");


static const char *print_bool(bool b)
{
	return b ? "true" : "false";
}


static void serialize_attr_bgp_as_path(struct rte_attr *attr)
{
	size_t i;
	for (i = 0; i < array_size(attr->bgp_as_path); i++)
		strbuf_printf(&xml_buf, "<as_no>%lu</as_no>", attr->bgp_as_path[i]);
}


static void serialize_attr_bgp_community(struct rte_attr *attr)
{
	size_t i;

	for (i = 0; i < array_size(attr->cflags); i++) {
		strbuf_printf(&xml_buf, "<cflag>");
		strbuf_printf(&xml_buf, "<flag>%lu</flag>", attr->cflags[i].flag);
		strbuf_printf(&xml_buf, "<as_no>%lu</as_no>", attr->cflags[i].as_no);
		strbuf_printf(&xml_buf, "</cflag>");
	}
}


static void serialize_rte_attr(struct rte_attr *attr)
{
	const char *name;
	name = get_attr_name(attr);

	switch (attr->type) {
	case RTE_ATTR_TYPE_BGP_AGGREGATOR:
		strbuf_printf(&xml_buf, "<%s", name);
		print_attr_ip("ip", attr->aggr.ip);
		print_attr("as_no", "%lu", attr->aggr.as_no);
		strbuf_printf(&xml_buf, "/>");
		return;
	default:
		break;
	}
	strbuf_printf(&xml_buf, "<%s>", name);

	switch (attr->type) {
	case RTE_ATTR_TYPE_BGP_AS_PATH:
		serialize_attr_bgp_as_path(attr);
		break;
	case RTE_ATTR_TYPE_BGP_ORIGIN:
		strbuf_printf(&xml_buf, "%lu", attr->bgp_origin);
		break;
	case RTE_ATTR_TYPE_BGP_NEXT_HOP:
		strbuf_printf(&xml_buf, "%u.%u.%u.%u",
			ipv4_get_byte(&attr->bgp_next_hop, 0),
			ipv4_get_byte(&attr->bgp_next_hop, 1),
			ipv4_get_byte(&attr->bgp_next_hop, 2),
			ipv4_get_byte(&attr->bgp_next_hop, 3));
		break;
	case RTE_ATTR_TYPE_BGP_LOCAL_PREF:
		strbuf_printf(&xml_buf, "%i", attr->bgp_local_pref);
		break;
	case RTE_ATTR_TYPE_BGP_COMMUNITY:
		serialize_attr_bgp_community(attr);
		break;
	case RTE_ATTR_TYPE_OTHER:
		strbuf_printf(&xml_buf, "<key>%s</key>", attr->other_attr.key);
		strbuf_printf(&xml_buf, "<value>%s</value>", attr->other_attr.value);
		break;
	default:
		break;
	}

	strbuf_printf(&xml_buf, "</%s>", name);
}


static void serialize_rte(struct rte *rte, struct nb_buffer *buf)
{
	size_t i;

	strbuf_printf(&xml_buf, "<rte");

	print_attr_ip("netaddr", rte->netaddr);
	print_attr("netmask", "%u", rte->netmask);
	print_attr_ip("gwaddr", rte->gwaddr);
	print_attr("ifname", "%s", rte->ifname);
	print_attr_time("uplink", &rte->uplink);

	if (rte->uplink_from_valid)
		print_attr_ip("uplink_from", rte->uplink_from);

	if (rte->as_no_valid)
		print_attr("as_no", "%lu", rte->as_no);

	print_attr("src", "%s", (char *)rte_src_to_string(rte->src));
	strbuf_printf(&xml_buf, ">");

	strbuf_printf(&xml_buf, "<type_flags");
	print_attr_bool("bgp", rte->type & RTE_TYPE_BGP);
	print_attr_bool("unicast", rte->type & RTE_TYPE_UNICAST);
	print_attr_bool("univ", rte->type & RTE_TYPE_UNIV);
	print_attr_bool("static", rte->type & RTE_TYPE_STATIC);
	strbuf_printf(&xml_buf, "/>");

	strbuf_printf(&xml_buf, "<attrs>");
	for (i = 0; i < array_size(rte->attrs); i++) {
		serialize_rte_attr(&rte->attrs[i]);
		nb_buffer_write(buf, (nb_byte_t *)strbuf_get_string(&xml_buf), strbuf_strlen(&xml_buf));
		strbuf_reset(&xml_buf);
	}

	strbuf_printf(&xml_buf, "</attrs>");
	strbuf_printf(&xml_buf, "</rte>");
}


void serialize_xml(struct rt *rt, struct nb_buffer *buf)
{
	size_t i;

	strbuf_init(&xml_buf, 4096);

	strbuf_printf(&xml_buf, "<rt version=\"%s\">", rt->version_str);
	for (i = 0; i < array_size(rt->entries); i++)
		serialize_rte(&rt->entries[i], buf);
	strbuf_printf(&xml_buf, "</rt>", rt->version_str);

	nb_buffer_write(buf, (nb_byte_t *)strbuf_get_string(&xml_buf), strbuf_strlen(&xml_buf));
	nb_buffer_flush(buf);
	strbuf_free(&xml_buf);
}
