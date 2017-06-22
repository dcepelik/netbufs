#include "netbufs.h"
#include "benchmark.h"

enum bird_org_rt
{
	BIRD_ORG_RT,
	BIRD_ORG_RT_ROUTES,
	BIRD_ORG_RT_VERSION,
};

enum bird_org_rte
{
	BIRD_ORG_RTE,
	BIRD_ORG_RTE_AS_NO,
	BIRD_ORG_RTE_ATTRS,
	BIRD_ORG_RTE_GWADDR,
	BIRD_ORG_RTE_IFNAME,
	BIRD_ORG_RTE_NETADDR,
	BIRD_ORG_RTE_NETMASK,
	BIRD_ORG_RTE_SRC,
	BIRD_ORG_RTE_TYPE,
	BIRD_ORG_RTE_UPLINK_FROM,
	BIRD_ORG_RTE_UPLINK,
};

enum bird_org_rta
{
	BIRD_ORG_RTA,
	BIRD_ORG_RTA_TFLAG,
	BIRD_ORG_RTA_BGP_AGGR,
	BIRD_ORG_RTA_BGP_AGGR_IP,
	BIRD_ORG_RTA_BGP_AGGR_AS_NO,
	BIRD_ORG_RTA_BGP_AS_PATH,
	BIRD_ORG_RTA_BGP_COMMUNITY,
	BIRD_ORG_RTA_BGP_CFLAG,
	BIRD_ORG_RTA_BGP_CFLAG_FLAG,
	BIRD_ORG_RTA_BGP_CFLAG_AS_NO,
	BIRD_ORG_RTA_BGP_LOCAL_PREF,
	BIRD_ORG_RTA_BGP_NEXT_HOP,
	BIRD_ORG_RTA_BGP_ORIGIN,
	BIRD_ORG_RTA_OTHER,
	BIRD_ORG_RTA_OTHER_KEY,
	BIRD_ORG_RTA_OTHER_VALUE,
};

enum bird_org_time
{
	BIRD_ORG_TIME,
	BIRD_ORG_TIME_HOUR,
	BIRD_ORG_TIME_MIN,
	BIRD_ORG_TIME_SEC,
};


static void setup_ids(struct nb *nb)
{
	struct nb_group *rt;
	struct nb_group *rte;
	struct nb_group *rta;
	struct nb_group *rta_other;
	struct nb_group *time;

	rt = nb_group(nb, BIRD_ORG_RT, "bird.org/rt");
	nb_bind(rt, BIRD_ORG_RT_ROUTES, "./routes", true);
	nb_bind(rt, BIRD_ORG_RT_VERSION, "./version", true);

	rte = nb_group(nb, BIRD_ORG_RTE, "bird.org/rte");
	nb_bind(rte, BIRD_ORG_RTE_AS_NO, "./as_no", false);
	nb_bind(rte, BIRD_ORG_RTE_ATTRS, "./attrs", true);
	nb_bind(rte, BIRD_ORG_RTE_GWADDR, "./gwaddr", true);
	nb_bind(rte, BIRD_ORG_RTE_IFNAME, "./ifname", false);
	nb_bind(rte, BIRD_ORG_RTE_NETADDR, "./netaddr", true);
	nb_bind(rte, BIRD_ORG_RTE_NETMASK, "./netmask", true);
	nb_bind(rte, BIRD_ORG_RTE_SRC, "./src", true);
	nb_bind(rte, BIRD_ORG_RTE_TYPE, "./type", true);
	nb_bind(rte, BIRD_ORG_RTE_UPLINK_FROM, "./uplink_from", false);
	nb_bind(rte, BIRD_ORG_RTE_UPLINK, "./uplink", false);

	rta = nb_group(nb, BIRD_ORG_RTA, "bird.org/rta");
	nb_bind(rta, BIRD_ORG_RTA_TFLAG, "./tflag", false);
	nb_bind(rta, BIRD_ORG_RTA_BGP_AGGR, "./bgp_aggr", false);
	nb_bind(rta, BIRD_ORG_RTA_BGP_AS_PATH, "./bgp_as_path", false);
	nb_bind(rta, BIRD_ORG_RTA_BGP_COMMUNITY, "./bgp_community", false);
	nb_bind(rta, BIRD_ORG_RTA_BGP_LOCAL_PREF, "./bgp_local_pref", false);
	nb_bind(rta, BIRD_ORG_RTA_BGP_NEXT_HOP, "./bgp_next_hop", false);
	nb_bind(rta, BIRD_ORG_RTA_BGP_ORIGIN, "./bgp_origin", false);
	nb_bind(rta, BIRD_ORG_RTA_OTHER, "./other", false);

	rta_other = nb_group(nb, BIRD_ORG_RTA_OTHER, "bird.org/rta_other");
	nb_bind(rta_other, BIRD_ORG_RTA_OTHER_KEY, "./key", true);
	nb_bind(rta_other, BIRD_ORG_RTA_OTHER_VALUE, "./value", true);

	time = nb_group(nb, BIRD_ORG_TIME, "bird.org/time");
	nb_bind(time, BIRD_ORG_TIME_HOUR, "./hour", true);
	nb_bind(time, BIRD_ORG_TIME_MIN, "./min", true);
	nb_bind(time, BIRD_ORG_TIME_SEC, "./sec", true);
}


static nb_err_t recv_ipv4(struct nb *nb, ipv4_t *ip)
{
	return nb_recv_u32(nb, ip);
}


static nb_err_t recv_time(struct nb *nb, struct tm *tm)
{
	nb_lid_t id;

	nb_recv_group(nb, BIRD_ORG_TIME);
	while (nb_recv_id(nb, &id)) {
		switch (id) {
			case BIRD_ORG_TIME_HOUR:
				nb_recv_i32(nb, &tm->tm_hour);
				break;
			case BIRD_ORG_TIME_MIN:
				nb_recv_i32(nb, &tm->tm_min);
				break;
			case BIRD_ORG_TIME_SEC:
				nb_recv_i32(nb, &tm->tm_sec);
				break;
		}
	}
	return nb_recv_group_end(nb);	
}


static nb_err_t recv_bgp_cflag(struct nb *nb, struct bgp_cflag *cflag)
{
	nb_lid_t id;

	nb_recv_group(nb, BIRD_ORG_RTA_BGP_CFLAG);
	while (nb_recv_id(nb, &id)) {
		switch (id) {
		case BIRD_ORG_RTA_BGP_CFLAG_FLAG:
			nb_recv_u32(nb, &cflag->flag);
			break;
		case BIRD_ORG_RTA_BGP_CFLAG_AS_NO:
			nb_recv_u32(nb, &cflag->as_no);
			break;
		}
	}
	return nb_recv_group_end(nb);
}


static nb_err_t recv_bgp_aggr(struct nb *nb, struct rte_attr *attr)
{
	nb_lid_t id;

	nb_recv_group(nb, BIRD_ORG_RTA_BGP_AGGR);
	while (nb_recv_id(nb, &id)) {
		switch (id) {
		case BIRD_ORG_RTA_BGP_AGGR_IP:
			recv_ipv4(nb, &attr->aggr.ip);
			break;
		case BIRD_ORG_RTA_BGP_AGGR_AS_NO:
			nb_recv_u32(nb, &attr->aggr.as_no);
			break;
		}
	}
	return nb_recv_group_end(nb);
}


static nb_err_t recv_rta_other(struct nb *nb, struct rte_attr *attr)
{
	nb_lid_t id;

	nb_recv_group(nb, BIRD_ORG_RTA_OTHER);
	while (nb_recv_id(nb, &id)) {
		switch (id) {
		case BIRD_ORG_RTA_OTHER_KEY:
			nb_recv_string(nb, &attr->other_attr.key);
			break;
		case BIRD_ORG_RTA_OTHER_VALUE:
			nb_recv_string(nb, &attr->other_attr.value);
			break;
		}
	}
	return nb_recv_group_end(nb);
}


static nb_err_t recv_rta(struct nb *nb, struct rte_attr *attr)
{
	nb_lid_t id;
	size_t i;

	nb_recv_group(nb, BIRD_ORG_RTA);
	while (nb_recv_id(nb, &id)) {
		switch (id) {
		case BIRD_ORG_RTA_TFLAG:
			nb_recv_bool(nb, &attr->tflag);
			break;
		case BIRD_ORG_RTA_BGP_AGGR:
			attr->type = RTE_ATTR_TYPE_BGP_AGGREGATOR;
			recv_bgp_aggr(nb, attr);
			break;
		case BIRD_ORG_RTA_BGP_AS_PATH:
			attr->type = RTE_ATTR_TYPE_BGP_AS_PATH;
			nb_recv_array(nb, &attr->bgp_as_path);
			for (i = 0; i < array_size(attr->bgp_as_path); i++)
				nb_recv_i32(nb, &attr->bgp_as_path[i]);
			nb_recv_array_end(nb);
			break;
		case BIRD_ORG_RTA_BGP_COMMUNITY:
			attr->type = RTE_ATTR_TYPE_BGP_COMMUNITY;
			nb_recv_array(nb, &attr->cflags);
			for (i = 0; i < array_size(attr->cflags); i++)
				recv_bgp_cflag(nb, &attr->cflags[i]);
			nb_recv_array_end(nb);
			break;
		case BIRD_ORG_RTA_BGP_LOCAL_PREF:
			attr->type = RTE_ATTR_TYPE_BGP_LOCAL_PREF;
			nb_recv_u32(nb, &attr->bgp_local_pref);
			break;
		case BIRD_ORG_RTA_BGP_NEXT_HOP:
			attr->type = RTE_ATTR_TYPE_BGP_NEXT_HOP;
			recv_ipv4(nb, &attr->bgp_next_hop);
			break;
		case BIRD_ORG_RTA_BGP_ORIGIN:
			attr->type = RTE_ATTR_TYPE_BGP_ORIGIN;
			nb_recv_i32(nb, (int32_t *)&attr->bgp_origin);
			break;
		case BIRD_ORG_RTA_OTHER:
			attr->type = RTE_ATTR_TYPE_OTHER;
			recv_rta_other(nb, attr);
			break;
		}
	}
	return nb_recv_group_end(nb);
}


static nb_err_t recv_rte(struct nb *nb, struct rte *rte)
{
	nb_lid_t id;
	size_t i;

	nb_recv_group(nb, BIRD_ORG_RTE);
	while (nb_recv_id(nb, &id)) {
		switch (id) {
		case BIRD_ORG_RTE_NETADDR:
			recv_ipv4(nb, &rte->netaddr);
			break;
		case BIRD_ORG_RTE_NETMASK:
			nb_recv_u32(nb, &rte->netmask);
			break;
		case BIRD_ORG_RTE_GWADDR:
			recv_ipv4(nb, &rte->gwaddr);
			break;
		case BIRD_ORG_RTE_IFNAME:
			nb_recv_string(nb, &rte->ifname);
			break;
		case BIRD_ORG_RTE_UPLINK:
			recv_time(nb, &rte->uplink);
			break;
		case BIRD_ORG_RTE_UPLINK_FROM:
			recv_ipv4(nb, &rte->uplink_from);
			break;
		case BIRD_ORG_RTE_TYPE:
			nb_recv_i32(nb, (int32_t *)&rte->type);
			break;
		case BIRD_ORG_RTE_SRC:
			nb_recv_i32(nb, (int32_t *)&rte->src);
			break;
		case BIRD_ORG_RTE_AS_NO:
			nb_recv_u32(nb, &rte->as_no);
			break;
		case BIRD_ORG_RTE_ATTRS:
			nb_recv_array(nb, &rte->attrs);
			for (i = 0; i < array_size(rte->attrs); i++)
				recv_rta(nb, &rte->attrs[i]);
			nb_recv_array_end(nb);
			break;
		}
	}
	return nb_recv_group_end(nb);
}


static nb_err_t recv_rt(struct nb *nb, struct rt *rt)
{
	nb_lid_t id;
	size_t i;

	nb_recv_group(nb, BIRD_ORG_RT);
	while (nb_recv_id(nb, &id)) {
		switch (id) {
		case BIRD_ORG_RT_VERSION:
			nb_recv_string(nb, &rt->version_str);
			break;

		case BIRD_ORG_RT_ROUTES:
			nb_recv_array(nb, &rt->entries);
			for (i = 0; i < array_size(rt->entries); i++)
				recv_rte(nb, &rt->entries[i]);
			nb_recv_array_end(nb);
			break;
		}
	}
	return nb_recv_group_end(nb);
}


struct rt *deserialize_netbufs(struct nb_buf *buf)
{
	struct nb nb;
	struct rt *rt;

	nb_init(&nb, buf);
	setup_ids(&nb);

	rt = nb_malloc(sizeof(*rt));
	recv_rt(&nb, rt);

	nb_free(&nb);
	return rt;
}
