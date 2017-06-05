#include "benchmark.h"
#include "netbufs.h"
#include "serialize-netbufs.h"


static bool recv_ipv4(struct netbuf *nb, int key, ipv4_t *ip)
{
	return nb_recv_ipv4(nb, key, ip) == NB_ERR_OK;
}


static bool recv_rte(struct netbuf *nb, struct rte *rte)
{
	nb_recv_group_begin(nb, BIRD_RTE);
	recv_ipv4(nb, BIRD_RTE_NETADDR, &rte->netaddr);
	nb_recv_u32(nb, BIRD_RTE_NETMASK, &rte->netmask);
	return nb_recv_group_end(nb);
}


static bool recv_rt(struct netbuf *nb, struct rt *rt)
{
	uint64_t num_rtes;

	nb_recv_group_begin(nb, BIRD_RT);

	nb_recv_string(nb, BIRD_RT_VERSION_STR, &rt->version_str);
	nb_recv_array_begin(nb, BIRD_RTES, &num_rtes);
	rt->entries = array_new(num_rtes, sizeof(*rt->entries));
	nb_recv_array_end(nb);

	return nb_recv_group_end(nb);
}


struct rt *deserialize_netbufs(struct nb_buf *buf)
{
	struct netbuf nb;
	struct rt *rt;

	nb_init(&nb, buf);
	rt = nb_malloc(sizeof(*rt));
	recv_rt(&nb, rt);
	//nb_free(&nb);
	
	return rt;
}
