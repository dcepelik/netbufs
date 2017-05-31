#ifndef SERIALIZE_H
#define SERIALIZE_H

#include "array.h"
#include "buf.h"
#include "internal.h"
#include "memory.h"
#include <assert.h>
#include <inttypes.h>
#include <time.h>

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

enum as_path_flag
{
	AS_PATH_FLAG_END = -1,
	AS_PATH_FLAG_LBRACE = -2,
	AS_PATH_FLAG_RBRACE = -3
};

enum rte_src
{
	RTE_SRC_INTERNAL,
	RTE_SRC_EXTERNAL,
	RTE_SRC_U,			/* TODO Ask MM about this value */
	RTE_SRC_WHO_KNOWS,
};

enum bgp_origin
{
	BGP_ORIGIN_IGP,
	BGP_ORIGIN_EGP,
	BGP_ORIGIN_INCOMPLETE,
};

/*
 * BGP boolean flags: key-value pairs (flag, as_no)
 */
struct bgp_cflag
{
	uint32_t flag;		/* the flag being set */
	uint32_t as_no;		/* number of AS that set it */
};

/*
 * BGP aggregator
 */
struct bgp_aggr
{
	ipv4_t ip;
	uint32_t as_no;
};

/*
 * Key-value pair for other route attributes (unknown to Bird)
 */
struct kvp
{
	char *key;
	char *value;
};

/*
 * Type of a route attribute.
 */
enum rte_attr_type
{
	RTE_ATTR_TYPE_BGP_ORIGIN = 0,
	RTE_ATTR_TYPE_BGP_NEXT_HOP,
	RTE_ATTR_TYPE_BGP_AS_PATH,
	RTE_ATTR_TYPE_BGP_LOCAL_PREF,
	RTE_ATTR_TYPE_BGP_COMMUNITY,
	RTE_ATTR_TYPE_BGP_AGGREGATOR,
	RTE_ATTR_TYPE_OTHER,
};

/*
 * Routing table attribute
 */
struct rte_attr
{
	enum rte_attr_type type;		/* type of the attribute */
	bool tflag;						/* [t] flag on this attr? */

	union {
		enum bgp_origin bgp_origin;	/* BGP.origin */
		ipv4_t bgp_next_hop;		/* BGP.next_hop */
		int *bgp_as_path;			/* BGP.as_path */
		uint32_t bgp_local_pref;	/* BGP.local_pref */
		struct bgp_cflag *cflags;	/* BGP.community */
		struct bgp_aggr aggr;		/* BGP.aggregator */
		struct kvp other_attr;		/* other attributes */
	};
};

enum rte_type
{
	RTE_TYPE_BGP = 1,
	RTE_TYPE_UNICAST = 2,
	RTE_TYPE_UNIV = 4,
	RTE_TYPE_STATIC = 8,
};

struct rte
{
	ipv4_t netaddr;
	uint32_t netmask;
	ipv4_t gwaddr;
	char *ifname;
	struct tm uplink;
	bool uplink_from_valid;
	ipv4_t uplink_from;
	struct rte_attr *attrs;
	enum rte_type type;
	bool as_no_valid;
	uint32_t as_no;
	enum rte_src src;
};

static inline struct rte_attr *rte_add_attr(struct rte *rte, enum rte_attr_type type)
{
	struct rte_attr *attr;

	rte->attrs = array_push(rte->attrs, 1);
	attr = &rte->attrs[array_size(rte->attrs) - 1]; /* TODO */
	attr->type = type;
	return attr;
}

struct rt
{
	char *version_str;		/* Bird dump version string */
	struct rte *entries;	/* entries of the routing table */
};


void serialize_bird(struct rt *rt, struct nb_buf *buf);
struct rt *deserialize_bird(struct nb_buf *buf);

void serialize_cbor(struct rt *rt, struct nb_buf *buf);
struct rt *deserialize_cbor(struct nb_buf *buf);

void serialize_netbufs(struct rt *rt, struct nb_buf *buf);
struct rt *deserialize_netbufs(struct nb_buf *buf);

#endif
