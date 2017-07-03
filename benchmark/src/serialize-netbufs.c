#include "benchmark.h"
#include "nbids.h"
#include "netbufs.h"


static nb_err_t send_time(struct nb *nb, int id, struct tm *tm)
{
	nb_send_id(nb, id);
	nb_send_group(nb, BIRD_ORG_TIME);
	nb_send_i32(nb, BIRD_ORG_TIME_HOUR, tm->tm_hour);
	nb_send_i32(nb, BIRD_ORG_TIME_MIN, tm->tm_min);
	nb_send_i32(nb, BIRD_ORG_TIME_SEC, tm->tm_sec);
	return nb_send_group_end(nb);
}


static void send_ipv4(struct nb *nb, nb_lid_t id, ipv4_t ip)
{
	nb_send_u32(nb, id, ip);
}


//static nb_err_t send_bgp_aggr(struct nb *nb, struct bgp_aggr *aggr)
//{
//	nb_send_group(nb, BIRD_ORG_RTA_BGP_AGGR);
//	send_ipv4(nb, BIRD_ORG_RTA_BGP_AGGR_IP, aggr->ip);
//	nb_send_u32(nb, BIRD_ORG_RTA_BGP_AGGR_AS_NO, aggr->as_no);
//	return nb_send_group_end(nb);
//}


static nb_err_t send_bgp_cflag(struct nb *nb, struct bgp_cflag *cflag)
{
	nb_send_group(nb, BIRD_ORG_RTA_BGP_CFLAG);
	nb_send_u32(nb, BIRD_ORG_RTA_BGP_CFLAG_FLAG, cflag->flag);
	nb_send_u32(nb, BIRD_ORG_RTA_BGP_CFLAG_AS_NO, cflag->as_no);
	return nb_send_group_end(nb);
}


static nb_err_t send_rta_other(struct nb *nb, struct rte_attr *attr)
{
	nb_send_group(nb, BIRD_ORG_KVP);
	nb_send_string(nb, BIRD_ORG_KVP_KEY, attr->other_attr.key);
	nb_send_string(nb, BIRD_ORG_KVP_VALUE, attr->other_attr.value);
	return nb_send_group_end(nb);
}


static nb_err_t send_rte_attr(struct nb *nb, struct rte_attr *attr)
{
	size_t i;

	nb_send_group(nb, BIRD_ORG_RTA);
	nb_send_bool(nb, BIRD_ORG_RTA_TFLAG, attr->tflag);

	switch (attr->type) {
	case RTE_ATTR_TYPE_BGP_ORIGIN:
		nb_send_u32(nb, BIRD_ORG_RTA_BGP_ORIGIN, attr->bgp_origin);
		break;
	case RTE_ATTR_TYPE_BGP_NEXT_HOP:
		send_ipv4(nb, BIRD_ORG_RTA_BGP_NEXT_HOP, attr->bgp_next_hop);
		break;
	case RTE_ATTR_TYPE_BGP_LOCAL_PREF:
		nb_send_u32(nb, BIRD_ORG_RTA_BGP_LOCAL_PREF, attr->bgp_local_pref);
		break;
	case RTE_ATTR_TYPE_BGP_AS_PATH:
		/* TODO 2 * is a hack */
		nb_send_array(nb, BIRD_ORG_RTA_BGP_AS_PATH, 2 * array_size(attr->bgp_as_path));
		for (i = 0; i < array_size(attr->bgp_as_path); i++)
			nb_send_u32(nb, -1, attr->bgp_as_path[i]); /* TODO give a name to -1 */
		nb_send_array_end(nb);
		break;
	case RTE_ATTR_TYPE_BGP_AGGREGATOR:
		/* TODO */
		break;
	case RTE_ATTR_TYPE_BGP_COMMUNITY:
		nb_send_array(nb, BIRD_ORG_RTA_BGP_COMMUNITY, array_size(attr->cflags));
		for (i = 0; i < array_size(attr->cflags); i++)
			send_bgp_cflag(nb, &attr->cflags[i]);
		nb_send_array_end(nb);
		break;
	case RTE_ATTR_TYPE_OTHER:
		nb_send_id(nb, BIRD_ORG_RTA_OTHER);
		send_rta_other(nb, attr);
		break;
	}
	return nb_send_group_end(nb);
}


static void send_rte(struct nb *nb, struct rte *rte)
{
	size_t i;

	nb_send_group(nb, BIRD_ORG_RTE);
	send_ipv4(nb, BIRD_ORG_RTE_NETADDR, rte->netaddr);
	nb_send_u32(nb, BIRD_ORG_RTE_NETMASK, rte->netmask);
	send_ipv4(nb, BIRD_ORG_RTE_GWADDR, rte->gwaddr);
	nb_send_string(nb, BIRD_ORG_RTE_IFNAME, rte->ifname);
	send_time(nb, BIRD_ORG_RTE_UPLINK, &rte->uplink);
	nb_send_i32(nb, BIRD_ORG_RTE_TYPE, rte->type);
	nb_send_i32(nb, BIRD_ORG_RTE_SRC, rte->src);

	nb_send_array(nb, BIRD_ORG_RTE_ATTRS, array_size(rte->attrs));
	for (i = 0; i < array_size(rte->attrs); i++)
		send_rte_attr(nb, &rte->attrs[i]);
	nb_send_array_end(nb);

	if (rte->uplink_from_valid)
		send_ipv4(nb, BIRD_ORG_RTE_UPLINK_FROM, rte->uplink_from);
	if (rte->as_no_valid)
		nb_send_u32(nb, BIRD_ORG_RTE_AS_NO, rte->as_no);

	nb_send_group_end(nb);
}


static void send_rt(struct nb *nb, struct rt *rt)
{
	size_t i;

	nb_send_group(nb, BIRD_ORG_RT);
	nb_send_string(nb, BIRD_ORG_RT_VERSION, rt->version_str);

	nb_send_array(nb, BIRD_ORG_RT_ROUTES, array_size(rt->entries));
	for (i = 0; i < array_size(rt->entries); i++)
		send_rte(nb, &rt->entries[i]);
	nb_send_array_end(nb);

	nb_send_group_end(nb);
}


void serialize_netbufs(struct rt *rt, struct nb_buf *buf)
{
	struct nb nb;
	nb_init(&nb, buf);
	setup_ids(&nb);
	send_rt(&nb, rt);
	nb_free(&nb);
}
