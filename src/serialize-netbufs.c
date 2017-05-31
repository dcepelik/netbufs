#include "benchmark.h"
#include "netbufs.h"
#include "serialize-netbufs.h"

/*
 * TODO Table generation
 * TODO "Autofixed" types
 */


static void send_time(struct netbuf *nb, int key, struct tm *tm)
{
	nb_array_begin(nb, key);
	nb_send_int(nb, BIRD_TIME_HOUR, tm->tm_hour);
	nb_send_int(nb, BIRD_TIME_MIN, tm->tm_min);
	nb_send_int(nb, BIRD_TIME_SEC, tm->tm_sec);
	nb_array_end(nb);
}


static void send_rte_attr(struct netbuf *nb, struct rte_attr *attr)
{
	size_t i;

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
	case RTE_ATTR_TYPE_BGP_AS_PATH:
		nb_array_begin(nb, BIRD_RTE_ATTR_BGP_AS_PATH);
		for (i = 0; i < array_size(attr->bgp_as_path); i++)
			nb_send_uint(nb, BIRD_AS_NO, attr->bgp_as_path[i]);
		nb_array_end(nb);
		break;
	case RTE_ATTR_TYPE_BGP_AGGREGATOR:
		nb_map_begin(nb, BIRD_RTE_ATTR_BGP_AGGREGATOR);
		nb_send_uint(nb, BIRD_AS_NO, attr->aggr.as_no);
		nb_send_ipv4(nb, BIRD_IPV4, attr->aggr.ip);
		nb_map_end(nb);
		break;
	case RTE_ATTR_TYPE_BGP_COMMUNITY:
		nb_array_begin(nb, BIRD_RTE_ATTR_BGP_COMMUNITY);
		nb_array_end(nb);
		break;
	default:
		/* ignore the attr */
		break;
	}
}


static void send_rte(struct netbuf *nb, struct rte *rte)
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


static void send_rt(struct netbuf *nb, struct rt *rt)
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
