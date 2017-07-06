#include "array.h"
#include "benchmark.h"
#include "debug.h"

#include <endian.h>
#include <stdbool.h>
#include <string.h>


static void serialize_uint(struct nb_buf *buf, size_t nbytes, uint64_t u64)
{
	assert(nbytes >= 1 && nbytes <= 8);

	uint64_t u64be;
	nb_byte_t *u64be_ptr = (nb_byte_t *)&u64be;
	size_t i;

	u64be = htobe64(u64);
	nb_buf_write(buf, u64be_ptr + (8 - nbytes), nbytes);
}


static void serialize_u8(struct nb_buf *buf, uint8_t u8)
{
	serialize_uint(buf, 1, u8);
}


static void serialize_u32(struct nb_buf *buf, uint32_t u32)
{
	serialize_uint(buf, 4, u32);
}


static void serialize_u64(struct nb_buf *buf, uint64_t u64)
{
	serialize_uint(buf, 8, u64);
}


static void serialize_bool(struct nb_buf *buf, bool b)
{
	serialize_u8(buf, b ? 1 : 0);
}


static void serialize_i32(struct nb_buf *buf, int32_t i32)
{
	bool negative = (i32 < 0);
	serialize_bool(buf, negative);

	if (!negative)
		serialize_u32(buf, (uint32_t)i32);
	else
		serialize_u32(buf, -1 - i32);
}


static void serialize_ipv4(struct nb_buf *buf, ipv4_t ip)
{
	serialize_u32(buf, ip);
}


static void serialize_string(struct nb_buf *buf, char *str)
{
	size_t len;
	len = strlen(str);
	serialize_u64(buf, len);
	nb_buf_write(buf, (nb_byte_t *)str, len);
}


static void serialize_time(struct nb_buf *buf, struct tm *tm)
{
	serialize_i32(buf, tm->tm_hour);
	serialize_i32(buf, tm->tm_min);
	serialize_i32(buf, tm->tm_sec);
}


static void serialize_attr_bgp_as_path(struct nb_buf *buf, struct rte_attr *attr)
{
	size_t i;
	serialize_u64(buf, array_size(attr->bgp_as_path));
	for (i = 0; i < array_size(attr->bgp_as_path); i++)
		serialize_i32(buf, attr->bgp_as_path[i]);
}


static void serialize_attr_bgp_community(struct nb_buf *buf, struct rte_attr *attr)
{
	size_t i;
	serialize_u64(buf, array_size(attr->cflags));
	for (i = 0; i < array_size(attr->cflags); i++) {
		serialize_u32(buf, attr->cflags[i].flag);
		serialize_u32(buf, attr->cflags[i].as_no);
	}
}


static void serialize_rte_attr(struct nb_buf *buf, struct rte_attr *attr)
{
	char *value;

	serialize_i32(buf, attr->type);
	serialize_bool(buf, attr->tflag);

	switch (attr->type) {
	case RTE_ATTR_TYPE_BGP_AS_PATH:
		serialize_attr_bgp_as_path(buf, attr);
		break;
	case RTE_ATTR_TYPE_BGP_ORIGIN:
		serialize_i32(buf, attr->bgp_origin);
		break;
	case RTE_ATTR_TYPE_BGP_NEXT_HOP:
		serialize_ipv4(buf, attr->bgp_next_hop);
		break;
	case RTE_ATTR_TYPE_BGP_LOCAL_PREF:
		serialize_u32(buf, attr->bgp_local_pref);
		break;
	case RTE_ATTR_TYPE_BGP_COMMUNITY:
		serialize_attr_bgp_community(buf, attr);
		break;
	case RTE_ATTR_TYPE_BGP_AGGREGATOR:
		serialize_ipv4(buf, attr->aggr.ip);
		serialize_u32(buf, attr->aggr.as_no);
		break;
	case RTE_ATTR_TYPE_OTHER:
		serialize_string(buf, attr->other_attr.key);
		value = attr->other_attr.value;
		if (value == NULL)
			value = "";
		serialize_string(buf, value);
		break;
	}
}


static void serialize_rte(struct nb_buf *buf, struct rte *rte)
{
	size_t i;
	serialize_u32(buf, rte->netmask);
	serialize_ipv4(buf, rte->netaddr);
	serialize_ipv4(buf, rte->gwaddr);
	serialize_string(buf, rte->ifname);
	serialize_time(buf, &rte->uplink);

	serialize_bool(buf, rte->uplink_from_valid);
	if (rte->uplink_from_valid)
		serialize_ipv4(buf, rte->uplink_from);

	serialize_bool(buf, rte->as_no_valid);
	if (rte->as_no_valid)
		serialize_u32(buf, rte->as_no);

	serialize_i32(buf, rte->src);
	serialize_i32(buf, rte->type);

	serialize_u64(buf, array_size(rte->attrs));
	for (i = 0; i < array_size(rte->attrs); i++)
		serialize_rte_attr(buf, &rte->attrs[i]);
}


void serialize_binary(struct rt *rt, struct nb_buf *buf)
{
	size_t i;

	serialize_string(buf, rt->version_str);
	for (i = 0; i < array_size(rt->entries); i++) {
		serialize_rte(buf, &rt->entries[i]);
	}
}
