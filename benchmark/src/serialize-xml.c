#include <libxml/tree.h>
#include "benchmark.h"


static void serialize_rte(xmlNodePtr rt_node, struct rte *rte)
{
	xmlNodePtr rte_node;

	rte_node = xmlNewNode(NULL, BAD_CAST "rte");
	assert(rte_node);
	xmlAddChild(rt_node, rte_node);
}


void serialize_xml(struct rt *rt, struct nb_buf *buf)
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
}
