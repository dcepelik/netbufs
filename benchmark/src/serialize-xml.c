#include "benchmark.h"
#include <inttypes.h>
#include <libxml/tree.h>


static char str_buf[256];


static char *print_ipv4(ipv4_t ip)
{
	snprintf(str_buf, sizeof(str_buf), "%d.%d.%d.%d",
		ipv4_get_byte(&ip, 0),
		ipv4_get_byte(&ip, 1),
		ipv4_get_byte(&ip, 2),
		ipv4_get_byte(&ip, 3));
	return str_buf;
}


static char *print_u64(uint64_t u64)
{
	snprintf(str_buf, sizeof(str_buf), "%lu", u64);
	return str_buf;
}


static char *print_time(struct tm *tm)
{
	snprintf(str_buf, sizeof(str_buf), "%i:%i:%i",
		tm->tm_hour,
		tm->tm_min,
		tm->tm_sec);
	return str_buf;
}


static const char *print_bool(bool b)
{
	return b ? "true" : "false";
}


static xmlNodePtr add_child(xmlNodePtr parent, char *name)
{
	xmlNodePtr child = xmlNewNode(NULL, BAD_CAST name);
	assert(child);
	xmlAddChild(parent, child);
	return child;
}


static void add_child_text(xmlNodePtr parent, char *text)
{
	xmlNodePtr text_node = xmlNewText(BAD_CAST text);
	assert(text_node);
	xmlAddChild(parent, text_node);
}


static void add_attr(xmlNodePtr node, char *attr_name, char *attr_value)
{
	xmlNewProp(node, BAD_CAST attr_name, BAD_CAST attr_value);
}


static void serialize_attr_bgp_as_path(xmlNodePtr attr_node, struct rte_attr *attr)
{
	size_t i;
	for (i = 0; i < array_size(attr->bgp_as_path); i++)
		add_child_text(add_child(attr_node, "as_no"),
			print_u64((uint64_t)attr->bgp_as_path[i]));
}


static void serialize_attr_bgp_community(xmlNodePtr attr_node, struct rte_attr *attr)
{
	xmlNodePtr cflag_node;
	size_t i;

	for (i = 0; i < array_size(attr->cflags); i++) {
		cflag_node = add_child(attr_node, "cflag");
		add_child_text(add_child(cflag_node, "flag"),
			print_u64(attr->cflags[i].flag));
		add_child_text(add_child(cflag_node, "as_no"),
			print_u64(attr->cflags[i].as_no));
	}
}


static void serialize_rte_attr(xmlNodePtr attrs_node, struct rte_attr *attr)
{
	xmlNodePtr attr_node;
	const char *name;

	name = get_attr_name(attr);
	attr_node = add_child(attrs_node, (char *)name);

	switch (attr->type) {
	case RTE_ATTR_TYPE_BGP_AS_PATH:
		serialize_attr_bgp_as_path(attr_node, attr);
		break;
	case RTE_ATTR_TYPE_BGP_ORIGIN:
		add_child_text(attr_node, print_u64((uint64_t)attr->bgp_origin));
		break;
	case RTE_ATTR_TYPE_BGP_NEXT_HOP:
		add_child_text(attr_node, print_ipv4(attr->bgp_next_hop));
		break;
	case RTE_ATTR_TYPE_BGP_LOCAL_PREF:
		add_child_text(attr_node, print_u64(attr->bgp_local_pref));
		break;
	case RTE_ATTR_TYPE_BGP_COMMUNITY:
		serialize_attr_bgp_community(attr_node, attr);
		break;
	case RTE_ATTR_TYPE_BGP_AGGREGATOR:
		add_attr(attr_node, "ip", print_ipv4(attr->aggr.ip));
		add_attr(attr_node, "as_no", print_u64(attr->aggr.as_no));
		break;
	case RTE_ATTR_TYPE_OTHER:
		add_child_text(add_child(attr_node, "key"), attr->other_attr.key);
		add_child_text(add_child(attr_node, "value"), attr->other_attr.value);
		break;
	}
}


static void serialize_rte(xmlNodePtr rt_node, struct rte *rte)
{
	xmlNodePtr rte_node;
	xmlNodePtr attrs_node;
	xmlNodePtr flags_node;
	size_t i;

	rte_node = xmlNewNode(NULL, BAD_CAST "rte");
	assert(rte_node);
	xmlAddChild(rt_node, rte_node);

	add_attr(rte_node, "netmask", print_u64(rte->netmask));
	add_attr(rte_node, "netaddr", print_ipv4(rte->netaddr));
	add_attr(rte_node, "gwaddr", print_ipv4(rte->gwaddr));
	add_attr(rte_node, "ifname", rte->ifname);
	add_attr(rte_node, "uplink", print_time(&rte->uplink));

	if (rte->uplink_from_valid)
		add_attr(rte_node, "uplink_from", print_ipv4(rte->uplink_from));

	if (rte->as_no_valid)
		add_attr(rte_node, "as_no", print_u64(rte->as_no));

	add_attr(rte_node, "src", (char *)rte_src_to_string(rte->src));

	flags_node = add_child(rte_node, "type_flags");
	add_attr(flags_node, "bgp", (char *)print_bool(rte->type & RTE_TYPE_BGP));
	add_attr(flags_node, "unicast", (char *)print_bool(rte->type & RTE_TYPE_UNICAST));
	add_attr(flags_node, "univ", (char *)print_bool(rte->type & RTE_TYPE_UNIV));
	add_attr(flags_node, "static", (char *)print_bool(rte->type & RTE_TYPE_STATIC));

	attrs_node = xmlNewNode(NULL, BAD_CAST "attrs");
	assert(attrs_node);
	xmlAddChild(rte_node, attrs_node);

	for (i = 0; i < array_size(rte->attrs); i++)
		serialize_rte_attr(attrs_node, &rte->attrs[i]);
}


void serialize_xml(struct rt *rt, struct nb_buffer *buf)
{
	xmlDocPtr doc;
	xmlNodePtr rt_node;
	size_t i;

	doc = xmlNewDoc(BAD_CAST "1.0");
	assert(doc != NULL);

	rt_node = xmlNewNode(NULL, BAD_CAST "rt");
	assert(rt_node != NULL);
	xmlDocSetRootElement(doc, rt_node);
	xmlNewProp(rt_node, BAD_CAST "version", BAD_CAST rt->version_str);

	for (i = 0; i < array_size(rt->entries); i++)
		serialize_rte(rt_node, &rt->entries[i]);

	xmlSaveFormatFileEnc("/tmp/out.xml", doc, "UTF-8", 1);
	xmlFreeDoc(doc);
	xmlCleanupParser();
}
