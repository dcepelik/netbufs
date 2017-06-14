#ifndef SERIALIZE_NETBUFS_H
#define SERIALIZE_NETBUFS_H

enum bird_nb_key
{
	BIRD_AS_NO,
	BIRD_IPV4,
	BIRD_RT,
	BIRD_RTE,
	BIRD_RTES,
	BIRD_RTE_AS_NO,
	BIRD_RTE_ATTR,
	BIRD_RTE_ATTRS,
	BIRD_RTE_ATTR_BGP_AGGREGATOR,
	BIRD_RTE_ATTR_BGP_AS_PATH,
	BIRD_RTE_ATTR_BGP_COMMUNITY,
	BIRD_RTE_ATTR_BGP_LOCAL_PREF,
	BIRD_RTE_ATTR_BGP_NEXT_HOP,
	BIRD_RTE_ATTR_BGP_ORIGIN,
	BIRD_RTE_ATTR_TFLAG,
	BIRD_RTE_ATTR_TYPE,
	BIRD_RTE_ATTR_OTHER_KEY,
	BIRD_RTE_ATTR_OTHER_VALUE,
	BIRD_RTE_GWADDR,
	BIRD_RTE_IFNAME,
	BIRD_RTE_NETADDR,
	BIRD_RTE_NETMASK,
	BIRD_RTE_SRC,
	BIRD_RTE_TYPE,
	BIRD_RTE_UPLINK_FROM,
	BIRD_RTE_UPTIME,
	BIRD_RT_VERSION_STR,
	BIRD_TIME,
	BIRD_TIME_HOUR,
	BIRD_TIME_MIN,
	BIRD_TIME_SEC,
};

#endif
