#include "array.h"
#include "benchmark.h"
#include "debug.h"


static void deserialize_uint(struct nb_buffer *buf, size_t nbytes, uint64_t *u64)
{
	ssize_t read;
	assert(nbytes >= 1 && nbytes <= 8);

	uint64_t u64be = 0;
	nb_byte_t *u64be_ptr = (nb_byte_t *)&u64be;
	read = nb_buffer_read(buf, u64be_ptr + (8 - nbytes), nbytes);
	assert(read >= 0 && (size_t)read == nbytes);
	*u64 = be64toh(u64be);
}


static void deserialize_u8(struct nb_buffer *buf, uint8_t *u8)
{
	uint64_t u64;
	deserialize_uint(buf, 1, &u64);
	assert(u64 <= UINT8_MAX);
	*u8 = (uint8_t)u64;
}


static void deserialize_u32(struct nb_buffer *buf, uint32_t *u32)
{
	uint64_t u64;
	deserialize_uint(buf, 4, &u64);
	assert(u64 <= UINT32_MAX);
	*u32 = (uint32_t)u64;
}


static void deserialize_u64(struct nb_buffer *buf, uint64_t *u64)
{
	deserialize_uint(buf, 8, u64);
}


static void deserialize_bool(struct nb_buffer *buf, bool *b)
{
	uint8_t u8;
	deserialize_u8(buf, &u8);
	assert(u8 == 0 || u8 == 1);
	*b = (u8 == 1);
}


static void deserialize_i32(struct nb_buffer *buf, int32_t *i32)
{
	bool negative;
	uint32_t u32;

	deserialize_bool(buf, &negative);
	deserialize_u32(buf, &u32);

	if (!negative)
		*i32 = (int32_t)u32;
	else
		*i32 = (int32_t)(-1 - u32);
}


static void deserialize_ipv4(struct nb_buffer *buf, ipv4_t *ip)
{
	deserialize_u32(buf, ip);
}


static void deserialize_time(struct nb_buffer *buf, struct tm *tm)
{
	deserialize_i32(buf, &tm->tm_hour);
	deserialize_i32(buf, &tm->tm_min);
	deserialize_i32(buf, &tm->tm_sec);
}


static void deserialize_string(struct nb_buffer *buf, char **str)
{
	size_t len;
	ssize_t read;
	deserialize_u64(buf, &len);
	*str = malloc(len + 1); /* TODO: * sizeof(char)? */
	read = nb_buffer_read(buf, (nb_byte_t *)*str, len);
	assert(read >= 0 && (size_t)read == len);
	(*str)[len] = '\0';
}


static void deserialize_attr_bgp_as_path(struct nb_buffer *buf, struct rte_attr *attr)
{
	size_t nitems;
	size_t i;

	deserialize_u64(buf, &nitems);
	attr->bgp_as_path = array_new(nitems, sizeof(*attr->bgp_as_path));
	for (i = 0; i < nitems; i++) {
		attr->bgp_as_path = array_push(attr->bgp_as_path, 1);
		deserialize_i32(buf, &attr->bgp_as_path[i]);
	}
}


static void deserialize_attr_bgp_community(struct nb_buffer *buf, struct rte_attr *attr)
{
	size_t nitems;
	size_t i;

	deserialize_u64(buf, &nitems);
	attr->cflags = array_new(nitems, sizeof(*attr->cflags));
	for (i = 0; i < nitems; i++) {
		attr->cflags = array_push(attr->cflags, 1);
		deserialize_u32(buf, &attr->cflags[i].flag);
		deserialize_u32(buf, &attr->cflags[i].as_no);
	}
}


static void deserialize_rte_attr(struct nb_buffer *buf, struct rte_attr *attr)
{
	size_t foo;

	deserialize_i32(buf, (int32_t *)&attr->type);
	deserialize_bool(buf, &attr->tflag);

	switch (attr->type) {
	case RTE_ATTR_TYPE_BGP_AS_PATH:
		deserialize_attr_bgp_as_path(buf, attr);
		break;
	case RTE_ATTR_TYPE_BGP_ORIGIN:
		deserialize_i32(buf, (int32_t *)&attr->bgp_origin);
		break;
	case RTE_ATTR_TYPE_BGP_NEXT_HOP:
		deserialize_ipv4(buf, &attr->bgp_next_hop);
		break;
	case RTE_ATTR_TYPE_BGP_LOCAL_PREF:
		deserialize_u32(buf, &attr->bgp_local_pref);
		break;
	case RTE_ATTR_TYPE_BGP_COMMUNITY:
		deserialize_attr_bgp_community(buf, attr);
		break;
	case RTE_ATTR_TYPE_BGP_AGGREGATOR:
		deserialize_ipv4(buf, &attr->aggr.ip);
		deserialize_u32(buf, &attr->aggr.as_no);
		break;
	case RTE_ATTR_TYPE_OTHER:
		deserialize_string(buf, &attr->other_attr.key);
		deserialize_string(buf, &attr->other_attr.value);
		break;
	}
}


static void deserialize_rte(struct nb_buffer *buf, struct rte *rte)
{
	size_t i;
	size_t nattrs;

	deserialize_u32(buf, &rte->netmask);
	deserialize_ipv4(buf, &rte->netaddr);
	deserialize_ipv4(buf, &rte->gwaddr);
	deserialize_string(buf, &rte->ifname);
	deserialize_time(buf, &rte->uplink);

	deserialize_bool(buf, &rte->uplink_from_valid);
	if (rte->uplink_from_valid)
		deserialize_ipv4(buf, &rte->uplink_from);

	deserialize_bool(buf, &rte->as_no_valid);
	if (rte->as_no_valid)
		deserialize_u32(buf, &rte->as_no);

	deserialize_i32(buf, (int32_t *)&rte->src);
	deserialize_i32(buf, (int32_t *)&rte->type);

	deserialize_u64(buf, &nattrs);
	rte->attrs = array_new(nattrs, sizeof(*rte->attrs));
	for (i = 0; i < nattrs; i++) {
		rte->attrs = array_push(rte->attrs, 1);
		deserialize_rte_attr(buf, &rte->attrs[i]);
	}
}


struct rt *deserialize_binary(struct nb_buffer *buf)
{
	struct rt *rt;
	size_t i;

	rt = malloc(sizeof(*rt));
	deserialize_string(buf, &rt->version_str);

	rt->entries = array_new(256, sizeof(*rt->entries)); /* TODO 256 */
	for (i = 0; !nb_buffer_is_eof(buf); i++) {
		rt->entries = array_push(rt->entries, 1);
		deserialize_rte(buf, &rt->entries[i]);
	}

	return rt;
}
