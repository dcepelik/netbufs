#include "benchmark.h"
#include "debug.h"
#include "netbufs.h"


void serialize_netbufs(struct rt *rt, struct nb_buf *buf)
{
	struct netbuf nb;
	struct rt *rt0 = NULL;
	struct rte *rte0 = NULL;
	struct rte_attr *attr0 = NULL;

	nb_init(&nb, buf);

	nb_bind_struct(&nb, 0, sizeof(struct rt), "bird.org/rt");
	nb_bind_string(&nb, (size_t)&rt0->version_str, "bird.org/rt/version_str");
	nb_bind_array(&nb, (size_t)&rt0->entries, "bird.org/rt/entries", "bird.org/rte");

	nb_bind_struct(&nb, 0, sizeof(struct rte), "bird.org/rte");
	nb_bind_uint32(&nb, (size_t)&rte0->netaddr, "bird.org/rte/netaddr");
	nb_bind_uint32(&nb, (size_t)&rte0->netmask, "bird.org/rte/netmask");
	nb_bind_uint32(&nb, (size_t)&rte0->gwaddr, "bird.org/rte/gwaddr");
	nb_bind_string(&nb, (size_t)&rte0->ifname, "bird.org/rte/ifname");
	nb_bind_bool(&nb, (size_t)&rte0->uplink_from_valid, "bord.org/rte/uplink_from_valid");
	nb_bind_struct(&nb, (size_t)&rte0->uplink, sizeof(struct tm), "bird.org/rte/uplink");
	//nb_bind_array(&nb, (size_t)&rte0->attrs, sizeof(struct rte_attr), "bird.org/rte_attr");
	nb_bind_int32(&nb, (size_t)&rte0->type, "bird.org/rte/type");
	nb_bind_bool(&nb, (size_t)&rte0->as_no_valid, "bird.org/rte/as_no_valid");
	nb_bind_uint32(&nb, (size_t)&rte0->as_no, "bird.org/rte/as_no");
	nb_bind_int32(&nb, (size_t)&rte0->src, "bird.org/rte/src");

	DEBUG_EXPR("%p", (void *)rt);

	DEBUG_EXPR("%lu", (size_t)&rt0->version_str);
	DEBUG_EXPR("%lu", (size_t)&rt0->entries);

	DEBUG_EXPR("%p", (void *)&rt->version_str);
	DEBUG_EXPR("%p", (void *)&rt->entries);

	DEBUG_EXPR("%p", (void *)((byte_t *)rt + (size_t)&rt0->version_str));
	DEBUG_EXPR("%p", (void *)((byte_t *)rt + (size_t)&rt0->entries));

	nb_send(&nb, rt, "bird.org/rt");
}
