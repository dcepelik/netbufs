#include "benchmark.h"
#include "netbufs.h"


enum netbufs_key
{
	BIRD_RTE_ATTR_TYPE,
	BIRD_RTE_ATTR_TFLAG,
	BIRD_RTE_ATTR_BGP_NEXT_HOP,
	BIRD_RTE_ATTR_BGP_ORIGIN,
	BIRD_RTE_ATTR_BGP_LOCAL_PREF,
	BIRD_RTE_NETADDR,
	BIRD_RTE_NETMASK,
	BIRD_RTE_GWADDR,
	BIRD_RTE_IFNAME,
	BIRD_RTE_UPTIME,
	BIRD_RTE_UPLINK_FROM,
	BIRD_RTE_TYPE,
	BIRD_RTE_AS_NO,
	BIRD_RTE_SRC,
	BIRD_RT_VERSION_STR,
	BIRD_ORG_TIME_HOUR,
	BIRD_ORG_TIME_MIN,
	BIRD_ORG_TIME_SEC,
};


void send_time(struct netbuf *nb, int key, struct tm *tm)
{
	nb_map_begin(nb, key);
	nb_send_int(nb, BIRD_ORG_TIME_HOUR, tm->tm_hour);
	nb_send_int(nb, BIRD_ORG_TIME_MIN, tm->tm_min);
	nb_send_int(nb, BIRD_ORG_TIME_SEC, tm->tm_sec);
	nb_map_end(nb);
}


void send_rte_attr(struct netbuf *nb, struct rte_attr *attr)
{
	nb_send_uint(nb, BIRD_RTE_ATTR_TYPE, attr->type);
	nb_send_bool(nb, BIRD_RTE_ATTR_TFLAG, attr->tflag);

	switch (attr->type) {
	case RTE_ATTR_TYPE_BGP_ORIGIN:
		nb_send_uint(nb, BIRD_RTE_ATTR_BGP_ORIGIN, attr->bgp_origin);
		break;
	case RTE_ATTR_TYPE_BGP_NEXT_HOP:
		nb_send_ipv4(nb, BIRD_RTE_ATTR_BGP_NEXT_HOP, attr->bgp_next_hop);
		break;
	case RTE_ATTR_TYPE_BGP_LOCAL_PREF:
		nb_send_uint(nb, BIRD_RTE_ATTR_BGP_LOCAL_PREF, attr->bgp_local_pref);
		break;
	default:
		/* ignore the attr */
		break;
	}
}


void send_rte(struct netbuf *nb, struct rte *rte)
{
	size_t i;

	nb_send_ipv4(nb, BIRD_RTE_NETADDR, rte->netaddr);
	nb_send_uint(nb, BIRD_RTE_NETMASK, rte->netmask);
	nb_send_ipv4(nb, BIRD_RTE_GWADDR, rte->gwaddr);
	nb_send_string(nb, BIRD_RTE_IFNAME, rte->ifname);
	send_time(nb, BIRD_RTE_UPTIME, &rte->uplink);

	if (rte->uplink_from_valid)
		nb_send_ipv4(nb, BIRD_RTE_UPLINK_FROM, rte->uplink_from);
	
	nb_send_uint(nb, BIRD_RTE_TYPE, rte->type);

	if (rte->as_no_valid)
		nb_send_uint(nb, BIRD_RTE_AS_NO, rte->as_no);
	
	nb_send_uint(nb, BIRD_RTE_SRC, rte->src);

	for (i = 0; i < array_size(rte->attrs); i++)
		send_rte_attr(nb, &rte->attrs[i]);
}


void send_rt(struct netbuf *nb, struct rt *rt)
{
	size_t i;

	nb_send_string(nb, BIRD_RT_VERSION_STR, rt->version_str);
	for (i = 0; i < array_size(rt->entries); i++)
		send_rte(nb, &rt->entries[i]);
}


void serialize_netbufs(struct rt *rt, struct nb_buf *buf)
{
	struct netbuf nb;
	nb_init(&nb, buf);
	send_rt(&nb, rt);
	//netbuf_free(&nb);
}
