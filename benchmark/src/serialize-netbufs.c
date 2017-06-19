#include "benchmark.h"
#include "debug.h"
#include "netbufs.h"
#include "serialize-netbufs.h"

/*
 * TODO Table generation
 * TODO "Autofixed" types
 */


static void send_time(struct netbuf *nb, int key, struct tm *tm)
{
	nb_send_group_begin(nb, key);
	nb_send_int(nb, BIRD_TIME_HOUR, tm->tm_hour);
	nb_send_int(nb, BIRD_TIME_MIN, tm->tm_min);
	nb_send_int(nb, BIRD_TIME_SEC, tm->tm_sec);
	nb_send_group_end(nb);
}


static nb_err_t send_ipv4(struct netbuf *nb, int key, ipv4_t ip)
{
	return nb_send_uint(nb, key, ip);
}


static void send_rte_attr(struct netbuf *nb, struct rte_attr *attr)
{
	size_t i;

	nb_send_group_begin(nb, BIRD_RTE_ATTR);

	nb_send_int(nb, BIRD_RTE_ATTR_TYPE, attr->type);
	nb_send_bool(nb, BIRD_RTE_ATTR_TFLAG, attr->tflag);

	switch (attr->type) {
	case RTE_ATTR_TYPE_BGP_ORIGIN:
		nb_send_uint(nb, BIRD_RTE_ATTR_BGP_ORIGIN, attr->bgp_origin);
		break;
	case RTE_ATTR_TYPE_BGP_NEXT_HOP:
		send_ipv4(nb, BIRD_RTE_ATTR_BGP_NEXT_HOP, attr->bgp_next_hop);
		break;
	case RTE_ATTR_TYPE_BGP_LOCAL_PREF:
		nb_send_uint(nb, BIRD_RTE_ATTR_BGP_LOCAL_PREF, attr->bgp_local_pref);
		break;
	case RTE_ATTR_TYPE_BGP_AS_PATH:
		nb_send_array_begin(nb, BIRD_RTE_ATTR_BGP_AS_PATH, array_size(attr->bgp_as_path));
		for (i = 0; i < array_size(attr->bgp_as_path); i++)
			nb_send_uint(nb, BIRD_AS_NO, attr->bgp_as_path[i]);
		nb_send_array_end(nb);
		break;
	case RTE_ATTR_TYPE_BGP_AGGREGATOR:
		nb_send_map_begin(nb, BIRD_RTE_ATTR_BGP_AGGREGATOR, 1);
		nb_send_uint(nb, BIRD_AS_NO, attr->aggr.as_no);
		send_ipv4(nb, BIRD_IPV4, attr->aggr.ip);
		nb_send_map_end(nb);
		break;
	case RTE_ATTR_TYPE_BGP_COMMUNITY:
		nb_send_array_begin(nb, BIRD_RTE_ATTR_BGP_COMMUNITY, 0);
		nb_send_array_end(nb);
		break;
	case RTE_ATTR_TYPE_OTHER:
		nb_send_string(nb, BIRD_RTE_ATTR_OTHER_KEY, attr->other_attr.key);
		nb_send_string(nb, BIRD_RTE_ATTR_OTHER_VALUE, attr->other_attr.value);
		break;
	default:
		/* ignore the attr */
		assert(false);
		break;
	}

	nb_send_group_end(nb);
}


static void send_rte(struct netbuf *nb, struct rte *rte)
{
	size_t i;

	nb_send_group_begin(nb, BIRD_RTE);
	send_ipv4(nb, BIRD_RTE_NETADDR, rte->netaddr);
	nb_send_uint(nb, BIRD_RTE_NETMASK, rte->netmask);
	send_ipv4(nb, BIRD_RTE_GWADDR, rte->gwaddr);
	nb_send_string(nb, BIRD_RTE_IFNAME, rte->ifname);
	send_time(nb, BIRD_RTE_UPTIME, &rte->uplink);

	if (rte->uplink_from_valid)
		send_ipv4(nb, BIRD_RTE_UPLINK_FROM, rte->uplink_from);
	
	nb_send_int(nb, BIRD_RTE_TYPE, rte->type);

	if (rte->as_no_valid)
		nb_send_uint(nb, BIRD_RTE_AS_NO, rte->as_no);
	
	nb_send_int(nb, BIRD_RTE_SRC, rte->src);

	nb_send_array_begin(nb, BIRD_RTE_ATTRS, array_size(rte->attrs));
	for (i = 0; i < array_size(rte->attrs); i++)
		send_rte_attr(nb, &rte->attrs[i]);
	nb_send_array_end(nb);

	nb_send_group_end(nb);
}


static void send_rt(struct netbuf *nb, struct rt *rt)
{
	size_t i;

	nb_send_group_begin(nb, BIRD_RT);
	nb_send_string(nb, BIRD_RT_VERSION_STR, rt->version_str);

	nb_send_array_begin(nb, BIRD_RTES, array_size(rt->entries));
	for (i = 0; i < array_size(rt->entries); i++)
		send_rte(nb, &rt->entries[i]);
	nb_send_array_end(nb);

	nb_send_group_end(nb);
}


void serialize_netbufs(struct rt *rt, struct nb_buf *buf)
{
	struct netbuf nb;
	nb_init(&nb, buf);

	nb_bind(&nb, "BIRD_AS_NO", BIRD_AS_NO);
	nb_bind(&nb, "BIRD_IPV4", BIRD_IPV4);
	nb_bind(&nb, "BIRD_RT", BIRD_RT);
	nb_bind(&nb, "BIRD_RTE", BIRD_RTE);
	nb_bind(&nb, "BIRD_RTES", BIRD_RTES);
	nb_bind(&nb, "BIRD_RTE_AS_NO", BIRD_RTE_AS_NO);
	nb_bind(&nb, "BIRD_RTE_ATTR", BIRD_RTE_ATTR);
	nb_bind(&nb, "BIRD_RTE_ATTRS", BIRD_RTE_ATTRS);
	nb_bind(&nb, "BIRD_RTE_ATTR_BGP_AGGREGATOR", BIRD_RTE_ATTR_BGP_AGGREGATOR);
	nb_bind(&nb, "BIRD_RTE_ATTR_BGP_AS_PATH", BIRD_RTE_ATTR_BGP_AS_PATH);
	nb_bind(&nb, "BIRD_RTE_ATTR_BGP_COMMUNITY", BIRD_RTE_ATTR_BGP_COMMUNITY);
	nb_bind(&nb, "BIRD_RTE_ATTR_BGP_LOCAL_PREF", BIRD_RTE_ATTR_BGP_LOCAL_PREF);
	nb_bind(&nb, "BIRD_RTE_ATTR_BGP_NEXT_HOP", BIRD_RTE_ATTR_BGP_NEXT_HOP);
	nb_bind(&nb, "BIRD_RTE_ATTR_BGP_ORIGIN", BIRD_RTE_ATTR_BGP_ORIGIN);
	nb_bind(&nb, "BIRD_RTE_ATTR_TFLAG", BIRD_RTE_ATTR_TFLAG);
	nb_bind(&nb, "BIRD_RTE_ATTR_TYPE", BIRD_RTE_ATTR_TYPE);
	nb_bind(&nb, "BIRD_RTE_ATTR_OTHER_KEY", BIRD_RTE_ATTR_OTHER_KEY);
	nb_bind(&nb, "BIRD_RTE_ATTR_OTHER_VALUE", BIRD_RTE_ATTR_OTHER_VALUE);
	nb_bind(&nb, "BIRD_RTE_GWADDR", BIRD_RTE_GWADDR);
	nb_bind(&nb, "BIRD_RTE_IFNAME", BIRD_RTE_IFNAME);
	nb_bind(&nb, "BIRD_RTE_NETADDR", BIRD_RTE_NETADDR);
	nb_bind(&nb, "BIRD_RTE_NETMASK", BIRD_RTE_NETMASK);
	nb_bind(&nb, "BIRD_RTE_SRC", BIRD_RTE_SRC);
	nb_bind(&nb, "BIRD_RTE_TYPE", BIRD_RTE_TYPE);
	nb_bind(&nb, "BIRD_RTE_UPLINK_FROM", BIRD_RTE_UPLINK_FROM);
	nb_bind(&nb, "BIRD_RTE_UPTIME", BIRD_RTE_UPTIME);
	nb_bind(&nb, "BIRD_RT_VERSION_STR", BIRD_RT_VERSION_STR);
	nb_bind(&nb, "BIRD_TIME", BIRD_TIME);
	nb_bind(&nb, "BIRD_TIME_HOUR", BIRD_TIME_HOUR);
	nb_bind(&nb, "BIRD_TIME_MIN", BIRD_TIME_MIN);
	nb_bind(&nb, "BIRD_TIME_SEC", BIRD_TIME_SEC);
	nb_bind(&nb, "NB_KEY", NB_KEY);
	nb_bind(&nb, "NB_KEY_NAME", NB_KEY_NAME);
	nb_bind(&nb, "NB_KEY_ID", NB_KEY_ID);

	send_rt(&nb, rt);
	nb_free(&nb);
}
