#include "benchmark.h"
#include "debug.h"

#include <ctype.h>
#include <libxml/tree.h>
#include <string.h>

#define NB_DEBUG_THIS	1


static xmlDoc *doc;


static char *get_attr(xmlNode *node, char *attr_name)
{
	return (char *)xmlGetProp(node, BAD_CAST attr_name);
}


static ipv4_t decode_ipv4(char *ipv4_str)
{
	assert(ipv4_str != NULL);

	char *c;
	size_t n;
	nb_byte_t byte = 0;
	ipv4_t ip = 0;

	for (c = ipv4_str, n = 0; *c != '\0'; c++) {
		if (*c == '.') {
			ipv4_set_byte(&ip, n, byte);
			n++;
			byte = 0;
		}
		else {
			assert(isdigit(*c));
			byte *= 10;
			byte += *c - '0';
		}
	}
	assert(n == 3);
	ipv4_set_byte(&ip, n, byte);

	return ip;
}


static uint64_t decode_u64(char *num_str)
{
	char *end;
	return strtoul(num_str, &end, 10);
}


static void decode_time(char *str_time, struct tm *tm)
{
	
}


static bool decode_bool(char *str_bool)
{
	bool b = !strcmp(str_bool, "true");
	assert(b || strcmp(str_bool, "false") == 0);
	return b;
}


char *extract_text(xmlNode *node)
{
	return (char *)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
}


static void deserialize_attr_bgp_as_path(xmlNode *attr_node, struct rte_attr *attr)
{
	xmlNode *as_node;
	size_t i;

	attr->bgp_as_path = array_new(2, sizeof(*attr->bgp_as_path));
	i = 0;
	for (as_node = attr_node->xmlChildrenNode; as_node; as_node = as_node->next) {
		if (as_node->type == XML_TEXT_NODE)
			continue;
		attr->bgp_as_path = array_push(attr->bgp_as_path, 1);
		attr->bgp_as_path[i] = decode_u64(extract_text(as_node));
		i++;
	}
}


static void deserialize_attr_bgp_community(xmlNode *attr_node, struct rte_attr *attr)
{
	xmlNode *cf_node;
	xmlNode *n;
	size_t i;
	char *name;

	attr->cflags = array_new(2, sizeof(*attr->cflags));
	i = 0;
	for (cf_node = attr_node->xmlChildrenNode; cf_node; cf_node = cf_node->next) {
		attr->cflags = array_push(attr->cflags, 1);
		for (n = cf_node->xmlChildrenNode; n; n = n->next) {
			name = (char *)n->name;
			if (strcmp(name, "flag") == 0)
				attr->cflags[i].flag = decode_u64(extract_text(n));
			else if (strcmp(name, "as_no") == 0)
				attr->cflags[i].as_no = decode_u64(extract_text(n));
		}
		i++;
	}
}


static void deserialize_rte_attrs(xmlNode *attrs_node, struct rte *rte)
{
	xmlNode *attr_node;
	xmlNode *n;
	struct rte_attr *attr;
	char *name;
	size_t i;

	rte->attrs = array_new(4, sizeof(*rte->attrs));

	i = 0;
	for (attr_node = attrs_node->xmlChildrenNode; attr_node; attr_node = attr_node->next) {
		name = (char *)attr_node->name;
		rte->attrs = array_push(rte->attrs, 1);
		attr = &rte->attrs[i];

		if (strcmp(name, "bgp_as_path") == 0) {
			attr->type = RTE_ATTR_TYPE_BGP_AS_PATH;
			deserialize_attr_bgp_as_path(attr_node, attr);
		}
		else if (strcmp(name, "bgp_origin") == 0) {
			attr->type = RTE_ATTR_TYPE_BGP_ORIGIN;
			attr->bgp_origin = decode_u64(extract_text(attr_node));
		}
		else if (strcmp(name, "bgp_next_hop") == 0) {
			attr->type = RTE_ATTR_TYPE_BGP_NEXT_HOP;
			attr->bgp_next_hop = decode_ipv4(extract_text(attr_node));
		}
		else if (strcmp(name, "bgp_local_pref") == 0) {
			attr->type = RTE_ATTR_TYPE_BGP_LOCAL_PREF;
			attr->bgp_local_pref = decode_u64(extract_text(attr_node));
		}
		else if (strcmp(name, "bgp_community") == 0) {
			attr->type = RTE_ATTR_TYPE_BGP_COMMUNITY;
			attr->cflags = array_new(2, sizeof(*attr->cflags));
		}
		else if (strcmp(name, "bgp_aggregator") == 0) {
			attr->type = RTE_ATTR_TYPE_BGP_AGGREGATOR;
			attr->aggr.ip = decode_ipv4(get_attr(attr_node, "ip"));
			attr->aggr.as_no = decode_u64(get_attr(attr_node, "as_no"));
		}
		else if (strcmp(name, "other") == 0) {
			attr->type = RTE_ATTR_TYPE_OTHER;
		}

		i++;
	}
}


static void deserialize_rte_flags(xmlNode *flags_node, struct rte *rte)
{
	rte->type = 0;
	if (decode_bool(get_attr(flags_node, "bgp")))
		rte->type |= RTE_TYPE_BGP;
	if (decode_bool(get_attr(flags_node, "unicast")))
		rte->type |= RTE_TYPE_UNICAST;
	if (decode_bool(get_attr(flags_node, "univ")))
		rte->type |= RTE_TYPE_UNIV;
	if (decode_bool(get_attr(flags_node, "static")))
		rte->type |= RTE_TYPE_STATIC;
}


static void deserialize_rte(xmlNode *rte_node, struct rte *rte)
{
	xmlNode *child;
	char *uplink_from_str;
	char *as_no_str;
	char *src_str;

	rte->netaddr = decode_ipv4(get_attr(rte_node, "netaddr"));
	rte->netmask = (uint32_t)decode_u64(get_attr(rte_node, "netmask"));
	rte->gwaddr = decode_ipv4(get_attr(rte_node, "gwaddr"));
	rte->ifname = get_attr(rte_node, "ifname");
	decode_time(get_attr(rte_node, "uplink"), &rte->uplink);

	uplink_from_str = get_attr(rte_node, "uplink_from");
	if (uplink_from_str)
		rte->uplink_from = decode_ipv4(uplink_from_str);

	as_no_str = get_attr(rte_node, "as_no");
	if (as_no_str)
		rte->as_no = decode_u64(as_no_str);

	src_str = get_attr(rte_node, "src");
	if (strcmp(src_str, rte_src_to_string(RTE_SRC_INTERNAL)) == 0)
		rte->src = RTE_SRC_INTERNAL;
	else if (strcmp(src_str, rte_src_to_string(RTE_SRC_EXTERNAL)) == 0)
		rte->src = RTE_SRC_EXTERNAL;
	else if (strcmp(src_str, rte_src_to_string(RTE_SRC_U)) == 0)
		rte->src = RTE_SRC_U;
	else if(strcmp(src_str, rte_src_to_string(RTE_SRC_WHO_KNOWS)) == 0)
		rte->src = RTE_SRC_WHO_KNOWS;

	for (child = rte_node->xmlChildrenNode; child; child = child->next) {
		if (strcmp((char *)child->name, "attrs") == 0)
			deserialize_rte_attrs(child, rte);
		else if (strcmp((char *)child->name, "flags") == 0)
			deserialize_rte_flags(child, rte);
	}
}


struct rt *deserialize_xml(struct nb_buffer *buf)
{
	xmlNode *rt_node;
	xmlNode *rte_node;
	struct rt *rt;
	size_t i;

	doc = xmlReadFile("/tmp/out.xml", NULL, 0);
	assert(doc != NULL);

	rt_node = xmlDocGetRootElement(doc);
	assert(rt_node);

	rt = malloc(sizeof(*rt));
	rt->version_str = get_attr(rt_node, "version");
	rt->entries = array_new(32, sizeof(*rt->entries));
	
	i = 0;
	for (rte_node = rt_node->xmlChildrenNode; rte_node; rte_node = rte_node->next) {
		if (rte_node->type == XML_TEXT_NODE)
			continue;
		rt->entries = array_push(rt->entries, 1);
		deserialize_rte(rte_node, &rt->entries[i]);
		i++;
	}

	xmlFreeDoc(doc);
	xmlCleanupParser();
	return rt;
}
