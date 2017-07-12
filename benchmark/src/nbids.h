#ifndef NBIDS_H
#define NBIDS_H

#include "netbufs.h"
#include "debug.h"

enum bird_org
{
	BIRD_ORG_RT,
	BIRD_ORG_RTA,
	BIRD_ORG_RTE,
	BIRD_ORG_TIME,
	BIRD_ORG_KVP,
	BIRD_ORG_BGP_CFLAG,
};

enum bird_org_rt
{
	BIRD_ORG_RT_ROUTES,
	BIRD_ORG_RT_VERSION,
};

enum bird_org_rte
{
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

enum bird_org_kvp
{
	BIRD_ORG_KVP_KEY,
	BIRD_ORG_KVP_VALUE,
};

enum bird_org_bgp_cflag
{
	BIRD_ORG_BGP_CFLAG_AS_NO,
	BIRD_ORG_BGP_CFLAG_FLAG,
};

enum bird_org_rta
{
	BIRD_ORG_RTA_BGP_AGGR,
	BIRD_ORG_RTA_BGP_AGGR_AS_NO,
	BIRD_ORG_RTA_BGP_AGGR_IP,
	BIRD_ORG_RTA_BGP_AS_PATH,
	BIRD_ORG_RTA_BGP_COMMUNITY,
	BIRD_ORG_RTA_BGP_LOCAL_PREF,
	BIRD_ORG_RTA_BGP_NEXT_HOP,
	BIRD_ORG_RTA_BGP_ORIGIN,
	BIRD_ORG_RTA_OTHER,
	BIRD_ORG_RTA_TFLAG,
};

enum bird_org_time
{
	BIRD_ORG_TIME_HOUR,
	BIRD_ORG_TIME_MIN,
	BIRD_ORG_TIME_SEC,
};


static inline void setup_ids(struct nb *nb)
{
	struct nb_group *rt;
	struct nb_group *rte;
	struct nb_group *rta;
	struct nb_group *kvp;
	struct nb_group *time;
	struct nb_group *bgp_cflag;

	rt = nb_group(nb, BIRD_ORG_RT, "bird.org/rt");
	nb_bind(nb, rt, BIRD_ORG_RT_ROUTES, "./routes", true);
	nb_bind(nb, rt, BIRD_ORG_RT_VERSION, "./version", true);

	rte = nb_group(nb, BIRD_ORG_RTE, "bird.org/rte");
	nb_bind(nb, rte, BIRD_ORG_RTE_AS_NO, "./as_no", false);
	nb_bind(nb, rte, BIRD_ORG_RTE_ATTRS, "./attrs", true);
	nb_bind(nb, rte, BIRD_ORG_RTE_GWADDR, "./gwaddr", true);
	nb_bind(nb, rte, BIRD_ORG_RTE_IFNAME, "./ifname", false);
	nb_bind(nb, rte, BIRD_ORG_RTE_NETADDR, "./netaddr", true);
	nb_bind(nb, rte, BIRD_ORG_RTE_NETMASK, "./netmask", true);
	nb_bind(nb, rte, BIRD_ORG_RTE_SRC, "./src", true);
	nb_bind(nb, rte, BIRD_ORG_RTE_TYPE, "./type", true);
	nb_bind(nb, rte, BIRD_ORG_RTE_UPLINK_FROM, "./uplink_from", false);
	nb_bind(nb, rte, BIRD_ORG_RTE_UPLINK, "./uplink", false);

	rta = nb_group(nb, BIRD_ORG_RTA, "bird.org/rta");
	nb_bind(nb, rta, BIRD_ORG_RTA_TFLAG, "./tflag", false);
	nb_bind(nb, rta, BIRD_ORG_RTA_BGP_AGGR, "./bgp_aggr", false);
	nb_bind(nb, rta, BIRD_ORG_RTA_BGP_AS_PATH, "./bgp_as_path", false);
	nb_bind(nb, rta, BIRD_ORG_RTA_BGP_COMMUNITY, "./bgp_community", false);
	nb_bind(nb, rta, BIRD_ORG_RTA_BGP_LOCAL_PREF, "./bgp_local_pref", false);
	nb_bind(nb, rta, BIRD_ORG_RTA_BGP_NEXT_HOP, "./bgp_next_hop", false);
	nb_bind(nb, rta, BIRD_ORG_RTA_BGP_ORIGIN, "./bgp_origin", false);
	nb_bind(nb, rta, BIRD_ORG_RTA_OTHER, "./other", false);

	kvp = nb_group(nb, BIRD_ORG_KVP, "bird.org/rta/kvp");
	nb_bind(nb, kvp, BIRD_ORG_KVP_KEY, "./key", true);
	nb_bind(nb, kvp, BIRD_ORG_KVP_VALUE, "./value", true);

	bgp_cflag = nb_group(nb, BIRD_ORG_BGP_CFLAG, "bird.org/bgp_cflag");
	nb_bind(nb, bgp_cflag, BIRD_ORG_BGP_CFLAG_FLAG, "./flag", true);
	nb_bind(nb, bgp_cflag, BIRD_ORG_BGP_CFLAG_AS_NO, "./as_no", true);

	time = nb_group(nb, BIRD_ORG_TIME, "bird.org/time");
	nb_bind(nb, time, BIRD_ORG_TIME_HOUR, "./hour", true);
	nb_bind(nb, time, BIRD_ORG_TIME_MIN, "./min", true);
	nb_bind(nb, time, BIRD_ORG_TIME_SEC, "./sec", true);
}

#endif
