#include "benchmark.h"
#include "debug.h"
#include "netbufs.h"


union remove_this
{
	enum bgp_origin bgp_origin;	/* BGP.origin */
	ipv4_t bgp_next_hop;		/* BGP.next_hop */
	int *bgp_as_path;			/* BGP.as_path */
	uint32_t bgp_local_pref;	/* BGP.local_pref */
	struct bgp_cflag *cflags;	/* BGP.community */
	struct bgp_aggr aggr;		/* BGP.aggregator */
	struct kvp other_attr;		/* other attributes */
};


void serialize_netbufs(struct rt *rt, struct nb_buf *buf)
{
	struct netbuf nb;
	struct rt *rt0 = NULL;
	struct rte *rte0 = NULL;
	struct rte_attr *attr0 = NULL;
	union remove_this *rem0 = NULL;
	struct tm *tm0 = NULL;

	DEBUG_EXPR("%p", (void *)&rt->entries[3].attrs[0].bgp_origin);

	nb_init(&nb, buf);

	nb_bind_struct(&nb, 0, sizeof(struct rt), "bird.org/rt");
	nb_bind_string(&nb, (size_t)&rt0->version_str, "bird.org/rt/version_str");
	nb_bind_array(&nb, (size_t)&rt0->entries, "bird.org/rt/entries", "bird.org/rte");

	nb_bind_struct(&nb, 0, sizeof(struct rte), "bird.org/rte");
	nb_bind_uint32(&nb, (size_t)&rte0->netaddr, "bird.org/rte/netaddr");
	nb_bind_uint32(&nb, (size_t)&rte0->netmask, "bird.org/rte/netmask");
	nb_bind_uint32(&nb, (size_t)&rte0->gwaddr, "bird.org/rte/gwaddr");
	nb_bind_string(&nb, (size_t)&rte0->ifname, "bird.org/rte/ifname");
	nb_bind_bool(&nb, (size_t)&rte0->uplink_from_valid, "bird.org/rte/uplink_from_valid");
	nb_bind_array(&nb, (size_t)&rte0->attrs, "bird.org/rte/attrs", "bird.org/rte_attr");
	nb_bind_int32(&nb, (size_t)&rte0->type, "bird.org/rte/type");
	nb_bind_bool(&nb, (size_t)&rte0->as_no_valid, "bird.org/rte/as_no_valid");
	nb_bind_uint32(&nb, (size_t)&rte0->as_no, "bird.org/rte/as_no");
	nb_bind_int32(&nb, (size_t)&rte0->src, "bird.org/rte/src");

	nb_bind_struct(&nb, (size_t)&rte0->uplink, sizeof(struct tm), "bird.org/rte/uplink");
	nb_bind_int32(&nb, (size_t)&tm0->tm_hour, "bird.org/rte/uplink/hour");
	nb_bind_int32(&nb, (size_t)&tm0->tm_min, "bird.org/rte/uplink/min");
	nb_bind_int32(&nb, (size_t)&tm0->tm_sec, "bird.org/rte/uplink/sec");

	nb_bind_struct(&nb, 0, sizeof(struct rte_attr), "bird.org/rte_attr");
	nb_bind_int32(&nb, (size_t)&attr0->type, "bird.org/rte_attr/type");
	nb_bind_bool(&nb, (size_t)&attr0->tflag, "bird.org/rte_attr/tflag");
	nb_bind_union(&nb, (size_t)&attr0->bgp_origin, -1, "bird.org/rte_attr/anon_union", "bird.org/rte_attr/type");

	nb_bind_int32(&nb, (size_t)&rem0->bgp_origin, "bird.org/rte_attr/anon_union/bgp_origin");
	nb_bind_uint32(&nb, (size_t)&rem0->bgp_next_hop, "bird.org/rte_attr/anon_union/bgp_next_hop");
	nb_bind_int32(&nb, 0, "bird.org/attrs/bgp/as_path/crumb");
	nb_bind_array(&nb, (size_t)&rem0->bgp_as_path, "bird.org/rte_attr/anon_union/bgp_as_path", "bird.org/attrs/bgp/as_path/crumb");
	nb_bind_uint32(&nb, (size_t)&rem0->bgp_local_pref, "bird.org/rte_attr/anon_union/bgp_local_pref");

	//nb_bind_struct(&nb, (size_t)&rem0->cflags, sizeof(struct bgp_cflag), "bird.org/rte_attr/anon_union/bgp_cflags");

	nb_send(&nb, rt, "bird.org/rt");
}
