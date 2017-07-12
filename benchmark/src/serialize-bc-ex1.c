/*
 * Bachelor's Thesis, Example 1
 *
 * This is not part of the actual benchmarks, it is a convenience utility that
 * allows me to generate example outputs for the Thesis quickly and repeatedly.
 */

#include "benchmark.h"
#include "cbor.h"
#include "array.h"
#include "string.h"


static void serialize_ipv4(struct cbor_stream *cs, ipv4_t *ip)
{
	cbor_encode_array_begin(cs, 4);
	size_t i;
	for (i = 0; i < 4; i++)
		cbor_encode_uint32(cs, ipv4_get_byte(ip, i));
	cbor_encode_array_end(cs);
}


static void serialize_ipv4_net(struct cbor_stream *cs, ipv4_t *ip, uint32_t netmask)
{
	serialize_ipv4(cs, ip);
	cbor_encode_uint32(cs, netmask);
}


static void serialize_time(struct cbor_stream *cs, struct tm *tm)
{
	cbor_encode_array_begin(cs, 3);
	cbor_encode_int32(cs, tm->tm_hour);
	cbor_encode_int32(cs, tm->tm_min);
	cbor_encode_int32(cs, tm->tm_sec);
	cbor_encode_array_end(cs);
}


static void serialize_bool(struct cbor_stream *cs, bool b)
{
	cbor_encode_sval(cs, b ? CBOR_SVAL_TRUE : CBOR_SVAL_FALSE);
}


static void serialize_string(struct cbor_stream *cs, char *str)
{
	cbor_encode_text(cs, str);
}


void serialize_bc_ex1(struct rt *rt, struct nb_buf *buf)
{
	assert(array_size(rt->entries) >= 1);

	struct rte *rte;
	struct cbor_stream *cs;
	struct diag diag;
	size_t i;

	rte = &rt->entries[0];
	cs = cbor_stream_new(buf);
	diag_init(&diag, stderr);
	cbor_stream_set_diag(cs, &diag);

	cbor_encode_map_begin_indef(cs);

	serialize_string(cs, "netaddr");
	serialize_ipv4(cs, &rte->netaddr);

	serialize_string(cs, "netmask");
	cbor_encode_int32(cs, rte->netmask);

	serialize_string(cs, "gwaddr");
	serialize_ipv4(cs, &rte->gwaddr);

	serialize_string(cs, "ifname");
	cbor_encode_text(cs, rte->ifname);

	serialize_string(cs, "uplink");
	serialize_time(cs, &rte->uplink);

	serialize_string(cs, "uplink_from_valid");
	serialize_bool(cs, rte->uplink_from_valid);

	if (rte->uplink_from_valid) {
		serialize_string(cs, "uplink_from");
		serialize_ipv4(cs, &rte->uplink_from);
	}

	serialize_string(cs, "as_no_valid");
	serialize_bool(cs, rte->as_no_valid);

	if (rte->as_no_valid) {
		serialize_string(cs, "as_no");
		cbor_encode_uint64(cs, rte->as_no);
	}

	serialize_string(cs, "src");
	cbor_encode_uint32(cs, rte->src);

	serialize_string(cs, "type");
	cbor_encode_int32(cs, rte->type);

	cbor_encode_map_end(cs);
}
