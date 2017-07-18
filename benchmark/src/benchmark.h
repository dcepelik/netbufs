/*
 * benchmark:
 * Data structures and routines for benchmarking purposes
 */

#ifndef BENCHMARK_H
#define BENCHMARK_H

#include "array.h"
#include "buffer.h"
#include "common.h"
#include "memory.h"

#include <assert.h>
#include <inttypes.h>
#include <time.h>

typedef uint32_t ipv4_t;

static inline double time_diff(clock_t start, clock_t end)
{
	return ((double)(end - start)) / CLOCKS_PER_SEC;
}

static inline void ipv4_init(ipv4_t *ip)
{
	*ip = 0;
}

static inline void ipv4_set_byte(ipv4_t *ip, size_t i, nb_byte_t b)
{
	assert(i >= 0 && i <= 3);
	nb_byte_t *ip_ptr = (nb_byte_t *)ip;
	ip_ptr[i] = b;
}

static nb_byte_t ipv4_get_byte(ipv4_t *ip, size_t i)
{
	assert(i >= 0 && i <= 3);
	nb_byte_t *ip_ptr = (nb_byte_t *)ip;
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

static inline const char *rte_src_to_string(enum rte_src src)
{
	switch (src) {
	case RTE_SRC_INTERNAL:
		return "internal";
	case RTE_SRC_EXTERNAL:
		return "external";
	case RTE_SRC_U:
		return "u";
	case RTE_SRC_WHO_KNOWS:
		return "unknown";
	default:
		return NULL;
	}
}

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
 * key-value pair for other route attributes
 */
struct kvp
{
	char *key;
	char *value;
};

/*
 * type of a route attribute
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
 * routing table attribute
 */
struct rte_attr
{
	enum rte_attr_type type;		/* type of the attribute */
	bool tflag;						/* [t] flag on this attr? */

	union {
		enum bgp_origin bgp_origin;	/* BGP.origin */
		ipv4_t bgp_next_hop;		/* BGP.next_hop */
		int *bgp_as_path;		/* BGP.as_path */
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

/*
 * Routing Table Entry
 */
struct rte
{
        ipv4_t netaddr;         /* target network address (IPv4) */
        uint32_t netmask;       /* target network prefix */
        ipv4_t gwaddr;          /* target gateway address (IPv4) */
        char *ifname;           /* interface where the gateway is reachable */
        struct tm uplink;       /* route uplink: how long the route is known  */
        bool uplink_from_valid; /* is uplink_from field valid? */
        ipv4_t uplink_from;     /* IPv4 address of the provider */
        struct rte_attr *attrs; /* route attributes */
        enum rte_type type;     /* route type */
        bool as_no_valid;       /* is as_no field valid? */
        uint32_t as_no;         /* number of the AS which provided the route */
        enum rte_src src;       /* route source */
};

struct rt
{
	char *version_str;		/* Bird dump version string */
	struct rte *entries;	/* entries of the routing table */
};


static inline const char *get_attr_name(struct rte_attr *attr)
{
	switch (attr->type) {
	case RTE_ATTR_TYPE_BGP_AS_PATH:
		return "bgp_as_path";
	case RTE_ATTR_TYPE_BGP_ORIGIN:
		return "bgp_origin";
	case RTE_ATTR_TYPE_BGP_NEXT_HOP:
		return "bgp_next_hop";
	case RTE_ATTR_TYPE_BGP_LOCAL_PREF:
		return "bgp_local_pref";
	case RTE_ATTR_TYPE_BGP_COMMUNITY:
		return "bgp_community";
	case RTE_ATTR_TYPE_BGP_AGGREGATOR:
		return "bgp_aggregator";
	case RTE_ATTR_TYPE_OTHER:
		return "bgp_other";
	default:
		return NULL;
	}
}


void serialize_bird(struct rt *rt, struct nb_buffer *buf);
struct rt *deserialize_bird(struct nb_buffer *buf);

void serialize_cbor(struct rt *rt, struct nb_buffer *buf);
struct rt *deserialize_cbor(struct nb_buffer *buf);

void serialize_netbufs(struct rt *rt, struct nb_buffer *buf);
struct rt *deserialize_netbufs(struct nb_buffer *buf);

void serialize_binary(struct rt *rt, struct nb_buffer *buf);
struct rt *deserialize_binary(struct nb_buffer *buf);

void serialize_bc_ex1(struct rt *rt, struct nb_buffer *buf);

void serialize_xml(struct rt *rt, struct nb_buffer *buf);
struct rt *deserialize_xml(struct nb_buffer *buf);

void serialize_pb(struct rt *rt, struct nb_buffer *buf);
struct rt *deserialize_pb(struct nb_buffer *buf);

#endif
