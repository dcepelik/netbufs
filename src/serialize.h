#ifndef SERIALIZE_H
#define SERIALIZE_H

#include "buf.h"
#include <inttypes.h>
#include <time.h>

typedef uint32_t ipv4_t;

enum bgp_origin
{
	BGP_ORIGIN_IGP,
	BGP_ORIGIN_EGP,
	BGP_ORIGIN_INCOMPLETE,
};

struct rte
{
	ipv4_t netaddr;
	int netmask;
	ipv4_t gwaddr;
	char *ifname;
	struct tm uplink;
	ipv4_t uplink_from;

	struct {
		struct {
			int *as_path;
			enum bgp_origin origin;
		} bgp;
	} attrs;
};

struct rt
{
	struct rte *entries;	/* entries of the routing table */
	size_t size;		/* size of *entries */
	size_t count;		/* number of entries in *entries */
};

void serialize_bird(struct buf *buf, struct rt *rt);

#endif
