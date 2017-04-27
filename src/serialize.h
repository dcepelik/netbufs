#ifndef SERIALIZE_H
#define SERIALIZE_H

#include "array.h"
#include "memory.h"
#include "buf.h"
#include <assert.h>
#include <inttypes.h>
#include <time.h>

typedef uint32_t ipv4_t;

static inline void ipv4_init(ipv4_t *ip)
{
	*ip = 0;
}

static inline void ipv4_set_byte(ipv4_t *ip, size_t i, byte_t b)
{
	assert(i >= 0 && i <= 3);
	byte_t *ip_ptr = (byte_t *)ip;
	ip_ptr[i] = b;
}

static inline byte_t ipv4_get_byte(ipv4_t *ip, size_t i)
{
	assert(i >= 0 && i <= 3);
	byte_t *ip_ptr = (byte_t *)ip;
	return ip_ptr[i];
}

enum bgp_origin
{
	BGP_ORIGIN_IGP,
	BGP_ORIGIN_EGP,
	BGP_ORIGIN_INCOMPLETE,
};

enum rte_attr_type
{
	RTE_ATTR_TYPE_BGP_AS_PATH = 1,
	RTE_ATTR_TYPE_BGP_ORIGIN,
	RTE_ATTR_TYPE_BGP_NEXT_HOP,
	RTE_ATTR_TYPE_BGP_LOCAL_PREF,
	RTE_ATTR_TYPE_BGP_COMMUNITY,
	RTE_ATTR_TYPE_BGP_AGGREGATOR,
};

/*
 * BGP boolean flags: key-values pairs (flag, as_no).
 */
struct bgp_cflag
{
	int flag;		/* the flag being set */
	int as_no;		/* number of AS that set it */
};

struct bgp_aggr
{
	ipv4_t ip;
	int as_no;
};

struct rte_attr
{
	enum rte_attr_type type;		/* type of the attribute */

	union {
		int *bgp_as_path;		/* BGP.as_path */
		enum bgp_origin bgp_origin;	/* BGP.orogin */
		ipv4_t bgp_next_hop;		/* BGP.next_hop */
		int bgp_local_pref;		/* BGP.local_pref */
		struct bgp_cflag *cflags;	/* BGP.community */
		struct bgp_aggr aggr;
	};
};

enum rte_type
{
	RTE_TYPE_BGP = 1,
	RTE_TYPE_UNICAST = 2,
	RTE_TYPE_UNIV = 4,
};

struct rte
{
	ipv4_t netaddr;
	int netmask;
	ipv4_t gwaddr;
	char *ifname;
	struct tm uplink;
	ipv4_t uplink_from;
	struct rte_attr *attrs;
	size_t num_attrs;
	size_t attrs_size;
	enum rte_type type;
};

static inline struct rte_attr *rte_add_attr(struct rte *rte, enum rte_attr_type type)
{
	struct rte_attr *attr;

	if (rte->num_attrs == rte->attrs_size) {
		rte->attrs_size *= 2;
		if (rte->attrs_size == 0)
			rte->attrs_size = 8; /* TODO */
		rte->attrs = realloc_safe(rte->attrs, rte->attrs_size * sizeof(*rte->attrs));
	}
	attr = &rte->attrs[rte->num_attrs++];
	attr->type = type;

	return attr;
}

struct rt
{
	struct rte *entries;	/* entries of the routing table */
	size_t size;		/* size of *entries */
	size_t count;		/* number of entries in *entries */
};


void serialize_bird(struct rt *rt);

#endif
