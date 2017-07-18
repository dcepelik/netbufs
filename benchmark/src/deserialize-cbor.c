#include "array.h"
#include "benchmark.h"
#include "cbor.h"
#include "debug.h"


void deserialize_ipv4(struct cbor_stream *cs, ipv4_t *ip)
{
	cbor_decode_uint32(cs, ip);
}


void deserialize_ipv4_net(struct cbor_stream *cs, ipv4_t *ip, uint32_t *netmask)
{
	deserialize_ipv4(cs, ip);
	cbor_decode_uint32(cs, netmask);
}


void deserialize_time(struct cbor_stream *cs, struct tm *tm)
{
	cbor_decode_int32(cs, &tm->tm_hour);
	cbor_decode_int32(cs, &tm->tm_min);
	cbor_decode_int32(cs, &tm->tm_sec);
}


void deserialize_bool(struct cbor_stream *cs, bool *b)
{
	enum cbor_sval sval;

	cbor_decode_sval(cs, &sval);
	assert(sval == CBOR_SVAL_TRUE || sval == CBOR_SVAL_FALSE);

	*b = (sval == CBOR_SVAL_TRUE);
}


void deserialize_attr_bgp_as_path(struct cbor_stream *cs, struct rte_attr *attr)
{
	size_t nitems;
	size_t i;

	cbor_decode_array_begin(cs, &nitems);
	attr->bgp_as_path = array_new(nitems, sizeof(*attr->bgp_as_path));

	for (i = 0; i < nitems; i++) {
		attr->bgp_as_path = array_push(attr->bgp_as_path, 1);
		cbor_decode_int32(cs, &attr->bgp_as_path[i]);
	}
	cbor_decode_array_end(cs);
}


void deserialize_attr_bgp_community(struct cbor_stream *cs, struct rte_attr *attr)
{
	size_t nitems;
	size_t i;

	cbor_decode_array_begin(cs, &nitems);
	attr->cflags = array_new(nitems / 2, sizeof(*attr->cflags));
	assert(nitems % 2 == 0); /* TODO clean this up */
	for (i = 0; i < nitems / 2; i++) {
		attr->cflags = array_push(attr->cflags, 1);
		cbor_decode_uint32(cs, &attr->cflags[i].flag);
		cbor_decode_uint32(cs, &attr->cflags[i].as_no);
	}
	cbor_decode_array_end(cs);
}


static void deserialize_rte_attr(struct cbor_stream *cs, struct rte_attr *attr)
{
	cbor_decode_int32(cs, (int32_t *)&attr->type);
	deserialize_bool(cs, &attr->tflag);

	switch (attr->type) {
	case RTE_ATTR_TYPE_BGP_AS_PATH:
		deserialize_attr_bgp_as_path(cs, attr);
		break;
	case RTE_ATTR_TYPE_BGP_ORIGIN:
		cbor_decode_int32(cs, (int32_t *)&attr->bgp_origin);
		break;
	case RTE_ATTR_TYPE_BGP_NEXT_HOP:
		deserialize_ipv4(cs, &attr->bgp_next_hop);
		break;
	case RTE_ATTR_TYPE_BGP_LOCAL_PREF:
		cbor_decode_uint32(cs, &attr->bgp_local_pref);
		break;
	case RTE_ATTR_TYPE_BGP_COMMUNITY:
		deserialize_attr_bgp_community(cs, attr);
		break;
	case RTE_ATTR_TYPE_BGP_AGGREGATOR:
		deserialize_ipv4(cs, &attr->aggr.ip);
		cbor_decode_uint32(cs, &attr->aggr.as_no);
		break;
	case RTE_ATTR_TYPE_OTHER:
		cbor_decode_text(cs, &attr->other_attr.key);
		cbor_decode_text(cs, &attr->other_attr.value);
		break;
	}
}


void deserialize_rte(struct cbor_stream *cs, struct rte *rte)
{
	size_t i;
	struct cbor_item item;
	nb_err_t err;

	deserialize_ipv4_net(cs, &rte->netaddr, &rte->netmask);
	deserialize_ipv4(cs, &rte->gwaddr);
	cbor_decode_text(cs, &rte->ifname);
	deserialize_time(cs, &rte->uplink);

	deserialize_bool(cs, &rte->uplink_from_valid);
	if (rte->uplink_from_valid)
		deserialize_ipv4(cs, &rte->uplink_from);

	deserialize_bool(cs, &rte->as_no_valid);
	if (rte->as_no_valid)
		cbor_decode_uint32(cs, &rte->as_no);
	cbor_decode_uint32(cs, &rte->src);

	cbor_decode_int32(cs, (int32_t *)&rte->type);
	cbor_decode_array_begin_indef(cs);
	rte->attrs = array_new(2, sizeof(*rte->attrs));
	for (i = 0; !cbor_is_break(cs); i++) {
		rte->attrs = array_push(rte->attrs, 1);
		deserialize_rte_attr(cs, &rte->attrs[i]);
	}
	cbor_decode_array_end(cs);
}


struct rt *deserialize_cbor(struct nb_buffer *buf)
{
	struct cbor_stream cs;
	struct diag diag;
	struct rt *rt;
	size_t i;

	cbor_stream_init(&cs, buf);
	diag_init(&diag, stderr);
	diag.enabled = true;
	diag_enable_col(&diag, DIAG_COL_RAW);
	diag_enable_col(&diag, DIAG_COL_ITEMS);
	diag_enable_col(&diag, DIAG_COL_CBOR);
	cbor_stream_set_diag(&cs, &diag);
	rt = nb_malloc(sizeof(*rt));

	cbor_decode_text(&cs, &rt->version_str);

	rt->entries = array_new(256, sizeof(*rt->entries));
	for (i = 0; !nb_buffer_is_eof(buf); i++) {
		rt->entries = array_push(rt->entries, 1);
		deserialize_rte(&cs, &rt->entries[i]);
	}

	cbor_stream_free(&cs);

	return rt;
}
