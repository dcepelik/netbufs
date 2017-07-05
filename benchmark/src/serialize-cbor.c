#include "benchmark.h"
#include "cbor.h"
#include "debug.h"
#include "string.h"


void serialize_ipv4(struct cbor_stream *cs, ipv4_t *ip)
{
	size_t i;
	for (i = 0; i < 4; i++)
		cbor_encode_uint32(cs, ipv4_get_byte(ip, i));
}


void serialize_ipv4_net(struct cbor_stream *cs, ipv4_t *ip, uint32_t netmask)
{
	serialize_ipv4(cs, ip);
	cbor_encode_uint32(cs, netmask);
}


void serialize_time(struct cbor_stream *cs, struct tm *tm)
{
	cbor_encode_int32(cs, tm->tm_hour);
	cbor_encode_int32(cs, tm->tm_min);
	cbor_encode_int32(cs, tm->tm_sec);
}


void serialize_bool(struct cbor_stream *cs, bool b)
{
	cbor_encode_sval(cs, b ? CBOR_SVAL_TRUE : CBOR_SVAL_FALSE);
}


void serialize_attr_bgp_as_path(struct cbor_stream *cs, struct rte_attr *attr)
{
	size_t i;

	cbor_encode_array_begin(cs, array_size(attr->bgp_as_path));
	for (i = 0; i < array_size(attr->bgp_as_path); i++)
		cbor_encode_int32(cs, attr->bgp_as_path[i]);
	cbor_encode_array_end(cs);
}


void serialize_attr_bgp_community(struct cbor_stream *cs, struct rte_attr *attr)
{
	size_t i;

	cbor_encode_array_begin(cs, 2 * array_size(attr->cflags));
	for (i = 0; i < array_size(attr->cflags); i++) {
		cbor_encode_uint32(cs, attr->cflags[i].flag);
		cbor_encode_uint32(cs, attr->cflags[i].as_no);
	}
	cbor_encode_array_end(cs);
}


/* as no is u64 */
void serialize_rte_attr(struct cbor_stream *cs, struct rte_attr *attr)
{
	char *value;

	cbor_encode_int32(cs, attr->type);
	serialize_bool(cs, attr->tflag);

	switch (attr->type) {
	case RTE_ATTR_TYPE_BGP_AS_PATH:
		serialize_attr_bgp_as_path(cs, attr);
		break;
	case RTE_ATTR_TYPE_BGP_ORIGIN:
		cbor_encode_int32(cs, attr->bgp_origin);
		break;
	case RTE_ATTR_TYPE_BGP_NEXT_HOP:
		serialize_ipv4(cs, &attr->bgp_next_hop);
		break;
	case RTE_ATTR_TYPE_BGP_LOCAL_PREF:
		cbor_encode_int32(cs, attr->bgp_local_pref);
		break;
	case RTE_ATTR_TYPE_BGP_COMMUNITY:
		serialize_attr_bgp_community(cs, attr);
		break;
	case RTE_ATTR_TYPE_BGP_AGGREGATOR:
		serialize_ipv4(cs, &attr->aggr.ip);
		cbor_encode_uint32(cs, attr->aggr.as_no);
		break;
	case RTE_ATTR_TYPE_OTHER:
		cbor_encode_text(cs, (nb_byte_t *)attr->other_attr.key, strlen(attr->other_attr.key));
		value = attr->other_attr.value;
		if (value == NULL)
			value = "";
		cbor_encode_text(cs, (nb_byte_t *)value, strlen(value));
		break;
	}
}


void serialize_rte(struct cbor_stream *cs, struct rte *rte)
{
	size_t i;

	serialize_ipv4_net(cs, &rte->netaddr, rte->netmask);
	serialize_ipv4(cs, &rte->gwaddr);
	cbor_encode_text(cs, (nb_byte_t *)rte->ifname, strlen(rte->ifname));
	serialize_time(cs, &rte->uplink);

	serialize_bool(cs, rte->uplink_from_valid);
	if (rte->uplink_from_valid)
		serialize_ipv4(cs, &rte->uplink_from);

	serialize_bool(cs, rte->as_no_valid);
	if (rte->as_no_valid)
		cbor_encode_uint64(cs, rte->as_no);
	cbor_encode_uint32(cs, rte->src);

	cbor_encode_int32(cs, rte->type);
	cbor_encode_array_begin_indef(cs);
	for (i = 0; i < array_size(rte->attrs); i++)
		serialize_rte_attr(cs, &rte->attrs[i]);
	cbor_encode_array_end(cs);
}


static void cbor_error_handler(struct cbor_stream *cs, nb_err_t err, void *arg)
{
	fprintf(stderr, "CBOR encoding error (%i): %s.\n", err, cbor_stream_strerror(cs));
	exit(EXIT_FAILURE);
}


void serialize_cbor(struct rt *rt, struct nb_buf *buf)
{
	struct cbor_stream *cs;
	size_t i;

	cs = cbor_stream_new(buf);
	cbor_encode_text(cs, (nb_byte_t *)rt->version_str, strlen(rt->version_str));
	
	for (i = 0; i < array_size(rt->entries); i++)
		serialize_rte(cs, &rt->entries[i]);
}
