#include "benchmark.h"
#include "debug.h"
#include "netbufs.h"
#include "serialize-netbufs.h"


static nb_err_t recv_ipv4(struct netbuf *nb, int key, ipv4_t *ip)
{
	return nb_recv_u32(nb, key, ip);
}


static bool recv_time(struct netbuf *nb, int key, struct tm *tm)
{
	nb_recv_group_begin(nb, key);
	nb_recv_i32(nb, BIRD_TIME_HOUR, &tm->tm_hour);
	nb_recv_i32(nb, BIRD_TIME_MIN, &tm->tm_min);
	nb_recv_i32(nb, BIRD_TIME_SEC, &tm->tm_sec);
	return nb_recv_group_end(nb);
}


static bool recv_rte_attr(struct netbuf *nb, struct rte_attr *attr)
{
	size_t as_path_len;
	size_t bgp_community_len;
	size_t i;
	size_t foo;

	nb_recv_group_begin(nb, BIRD_RTE_ATTR);

	nb_recv_i32(nb, BIRD_RTE_ATTR_TYPE, (int *)&attr->type);
	nb_recv_bool(nb, BIRD_RTE_ATTR_TFLAG, &attr->tflag);

	switch (attr->type) {
	case RTE_ATTR_TYPE_BGP_ORIGIN:
		nb_recv_u32(nb, BIRD_RTE_ATTR_BGP_ORIGIN, &attr->bgp_origin);
		break;
	case RTE_ATTR_TYPE_BGP_NEXT_HOP:
		recv_ipv4(nb, BIRD_RTE_ATTR_BGP_NEXT_HOP, &attr->bgp_next_hop);
		break;
	case RTE_ATTR_TYPE_BGP_LOCAL_PREF:
		nb_recv_i32(nb, BIRD_RTE_ATTR_BGP_LOCAL_PREF, (int *)&attr->bgp_local_pref);
		break;
	case RTE_ATTR_TYPE_BGP_AS_PATH:
		nb_recv_array_begin(nb, BIRD_RTE_ATTR_BGP_AS_PATH, &as_path_len);
		attr->bgp_as_path = array_new(as_path_len, sizeof(*attr->bgp_as_path));
		for (i = 0; i < as_path_len; i++) {
			attr->bgp_as_path = array_push(attr->bgp_as_path, 1);
			nb_recv_i32(nb, BIRD_AS_NO, &attr->bgp_as_path[i]);
		}
		nb_recv_array_end(nb);
		break;
	case RTE_ATTR_TYPE_BGP_AGGREGATOR:
		nb_recv_map_begin(nb, BIRD_RTE_ATTR_BGP_AGGREGATOR, &foo);
		nb_recv_u32(nb, BIRD_AS_NO, &attr->aggr.as_no);
		recv_ipv4(nb, BIRD_IPV4, &attr->aggr.ip);
		nb_recv_map_end(nb); /* TODO send/recv intermix */
		break;
	case RTE_ATTR_TYPE_BGP_COMMUNITY:
		nb_recv_array_begin(nb, BIRD_RTE_ATTR_BGP_COMMUNITY, &bgp_community_len);
		attr->cflags = array_new(1, sizeof(*attr->cflags));
		nb_recv_array_end(nb);
		break;
	case RTE_ATTR_TYPE_OTHER:
		nb_recv_string(nb, BIRD_RTE_ATTR_OTHER_KEY, &attr->other_attr.key);
		nb_recv_string(nb, BIRD_RTE_ATTR_OTHER_VALUE, &attr->other_attr.value);
		break;
	default:
		/* ignore the attr */
		assert(false);
		break;
	}

	return nb_recv_group_end(nb);
}


static bool recv_rte(struct netbuf *nb, struct rte *rte)
{
	size_t i;
	size_t num_attrs;

	nb_recv_group_begin(nb, BIRD_RTE);
	recv_ipv4(nb, BIRD_RTE_NETADDR, &rte->netaddr);
	nb_recv_u32(nb, BIRD_RTE_NETMASK, &rte->netmask);
	recv_ipv4(nb, BIRD_RTE_GWADDR, &rte->gwaddr);
	nb_recv_string(nb, BIRD_RTE_IFNAME, &rte->ifname);
	recv_time(nb, BIRD_RTE_UPTIME, &rte->uplink);
	nb_recv_i32(nb, BIRD_RTE_TYPE, (int32_t *)&rte->type);
	nb_recv_i32(nb, BIRD_RTE_SRC, (int32_t *)&rte->src);

	nb_recv_array_begin(nb, BIRD_RTE_ATTRS, &num_attrs);
	rte->attrs = array_new(num_attrs, sizeof(*rte->attrs));
	for (i = 0; i < num_attrs; i++) {
		rte->attrs = array_push(rte->attrs, 1);
		recv_rte_attr(nb, &rte->attrs[i]);
	}
	nb_recv_array_end(nb);

	rte->as_no_valid = false;
	rte->uplink_from_valid = false;

	return nb_recv_group_end(nb);
}


static bool recv_rt(struct netbuf *nb, struct rt *rt)
{
	uint64_t num_rtes;
	size_t i;
	bool b;

	nb_recv_group_begin(nb, BIRD_RT);
	nb_recv_string(nb, BIRD_RT_VERSION_STR, &rt->version_str);
	nb_recv_array_begin(nb, BIRD_RTES, &num_rtes);
	rt->entries = array_new(num_rtes, sizeof(*rt->entries));
	for (i = 0; i < num_rtes; i++) {
		rt->entries = array_push(rt->entries, 1);
		recv_rte(nb, &rt->entries[i]);
	}
	nb_recv_array_end(nb);

	return nb_recv_group_end(nb);
}


struct rt *deserialize_netbufs_generic(struct nb_buf *buf)
{
	struct netbuf nb;
	struct rt *rt;

	nb_init(&nb, buf);
	nb.cs->fail_on_error = true;

	rt = nb_malloc(sizeof(*rt));
	recv_rt(&nb, rt);
	nb_free(&nb);
	
	return rt;
}
