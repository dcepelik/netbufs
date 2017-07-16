#include "gen/rt.pb.h"
#include <iostream>
#include <fstream>
#include <ctime>

extern "C" {
#include "benchmark.h"
}

namespace pb = protobuf_bird_bench;
using namespace std;

void serialize_pb(struct rt *rt, struct nb_buffer *buf)
{
	GOOGLE_PROTOBUF_VERIFY_VERSION;

	pb::rt pb_rt;
	pb_rt.set_version(rt->version_str);

	for (size_t i = 0; i < array_size(rt->entries); i++) {
		pb::rte *pb_rte = pb_rt.add_entries();
		struct rte *rte = &rt->entries[i];

		pb_rte->set_netaddr(rte->netaddr);
		pb_rte->set_prefix(rte->netmask);
		pb_rte->set_gwaddr(rte->gwaddr);
		pb_rte->set_as_no(rte->as_no);
		pb_rte->set_ifname(rte->ifname);
		pb_rte->mutable_uplink()->set_hour(rte->uplink.tm_hour);
		pb_rte->mutable_uplink()->set_min(rte->uplink.tm_min);
		pb_rte->mutable_uplink()->set_sec(rte->uplink.tm_sec);
		if (rte->uplink_from_valid)
			pb_rte->set_uplink_from(rte->uplink_from);
		pb_rte->set_type(rte->type);

		switch (rte->src) {
		case RTE_SRC_INTERNAL:
			pb_rte->set_src(pb::rte_src::INTERNAL);
			break;
		case RTE_SRC_EXTERNAL:
			pb_rte->set_src(pb::rte_src::EXTERNAL);
			break;
		case RTE_SRC_U:
			pb_rte->set_src(pb::rte_src::U);
			break;
		case RTE_SRC_WHO_KNOWS:
			pb_rte->set_src(pb::rte_src::WHO_KNOWS);
			break;
		}

		for (size_t j = 0; j < array_size(rte->attrs); j++) {
			pb::rta *pb_rta = pb_rte->add_attrs();
			struct rte_attr *rta = &rte->attrs[j];
			pb_rta->set_tflag(rta->tflag);
			
			switch (rta->type) {
			case RTE_ATTR_TYPE_BGP_ORIGIN:
				pb_rta->set_type(pb::rta_type::BGP_ORIGIN);
				switch (rta->bgp_origin) {
				case BGP_ORIGIN_IGP:
					pb_rta->set_origin(pb::bgp_origin::IGP);
					break;
				case BGP_ORIGIN_EGP:
					pb_rta->set_origin(pb::bgp_origin::EGP);
					break;
				case BGP_ORIGIN_INCOMPLETE:
					pb_rta->set_origin(pb::bgp_origin::INCOMPLETE);
					break;
				}
				break;
			case RTE_ATTR_TYPE_BGP_AS_PATH:
				pb_rta->set_type(pb::rta_type::BGP_AS_PATH);
				for (size_t k = 0; k < array_size(rta->bgp_as_path); k++)
					pb_rta->mutable_as_path()->add_as_no(rta->bgp_as_path[k]);
				break;
			case RTE_ATTR_TYPE_BGP_NEXT_HOP:
				pb_rta->set_type(pb::rta_type::BGP_NEXT_HOP);
				pb_rta->set_next_hop(rta->bgp_next_hop);
				break;
			case RTE_ATTR_TYPE_BGP_LOCAL_PREF:
				pb_rta->set_type(pb::rta_type::BGP_LOCAL_PREF);
				pb_rta->set_local_pref(rta->bgp_local_pref);
				break;
			case RTE_ATTR_TYPE_BGP_COMMUNITY:
				pb_rta->set_type(pb::rta_type::BGP_COMMUNITY);
				for (size_t k = 0; k < array_size(rta->cflags); k++) {
					pb::cflag *pb_cflag = pb_rta->mutable_community()->add_cflags();
					pb_cflag->set_flag(rta->cflags[k].flag);
					pb_cflag->set_as_no(rta->cflags[k].as_no);
				}
				break;
			case RTE_ATTR_TYPE_BGP_AGGREGATOR:
				pb_rta->set_type(pb::rta_type::BGP_AGGREGATOR);
				pb_rta->mutable_aggregator()->set_as_no(rta->aggr.as_no);
				pb_rta->mutable_aggregator()->set_ip(rta->aggr.ip);
				break;
			case RTE_ATTR_TYPE_OTHER:
				pb_rta->set_type(pb::rta_type::OTHER);
				pb_rta->mutable_other()->set_key(rta->other_attr.key);
				if (rta->other_attr.value)
					pb_rta->mutable_other()->set_value(rta->other_attr.value);
				break;
			}
		}
	}

	clock_t start = clock();
	fstream output("/tmp/out.pb", ios::out | ios::trunc | ios::binary);
	if (!pb_rt.SerializeToOstream(&output)) {
		cerr << "Failed to serialize RT using protobufs" << endl;
	}
	output.flush();
	clock_t end = clock();
	cout << "time c++ " << ((double) (end - start)) / CLOCKS_PER_SEC;
}
