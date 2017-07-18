extern "C" {
#include "benchmark.h"
}

#include "gen/rt.pb.h"

#include <iostream>
#include <iomanip>
#include <cassert>

using namespace std;
namespace pb = protobuf_bird_bench;


struct rt *deserialize_pb(struct nb_buffer *buf)
{
	size_t buf_written = nb_buffer_get_written_total(buf);
	nb_byte_t *bytes = (nb_byte_t *)nb_malloc(buf_written);
	ssize_t read = nb_buffer_read(buf, bytes, buf_written);
	assert(read >= 0 && (size_t)read == buf_written);
	string input((char *)bytes, buf_written);

	pb::rt pb_rt;

	clock_t start_i = clock();
	assert(pb_rt.ParseFromString(input));
	clock_t end_i = clock();
	cout << "\tinner deserialize " << setprecision(3) << time_diff(start_i, end_i) << endl;

	struct rt *rt = (struct rt *)nb_malloc(sizeof(rt));
	rt->version_str = (char *)pb_rt.version().c_str();
	rt->entries = (struct rte *)array_new_size(pb_rt.entries_size(), sizeof(*rt->entries));

	for (size_t i = 0; i < array_size(rt->entries); i++) {
		struct rte *rte = &rt->entries[i];
		const struct pb::rte &pb_rte = pb_rt.entries(i);
		rte->netaddr = pb_rte.netaddr();
		rte->netmask = pb_rte.prefix();
		rte->gwaddr = pb_rte.gwaddr();
		rte->as_no = pb_rte.as_no();
		rte->ifname = strdup(pb_rte.ifname().c_str());

		rte->uplink.tm_hour = pb_rte.uplink().hour();
		rte->uplink.tm_min = pb_rte.uplink().min();
		rte->uplink.tm_sec = pb_rte.uplink().sec();

		rte->uplink_from_valid = pb_rte.has_uplink_from();
		if (rte->uplink_from_valid)
			rte->uplink_from = pb_rte.uplink_from();

		rte->type = (enum rte_type)pb_rte.type();

		switch (pb_rte.src()) {
		case pb::rte_src::INTERNAL:
			rte->src = RTE_SRC_INTERNAL;
			break;
		case pb::rte_src::EXTERNAL:
			rte->src = RTE_SRC_EXTERNAL;
			break;
		case pb::rte_src::U:
			rte->src = RTE_SRC_U;
			break;
		case pb::rte_src::WHO_KNOWS:
			rte->src = RTE_SRC_WHO_KNOWS;
			break;
		}

		rte->attrs = (struct rte_attr *)array_new_size(pb_rte.attrs_size(), sizeof(*rte->attrs));
		for (size_t j = 0; j < array_size(rte->attrs); j++) {
			struct rte_attr *rta = &rte->attrs[j];
			const struct pb::rta pb_rta = pb_rte.attrs(j);
			rta->tflag = pb_rta.tflag();

			switch (pb_rta.type()) {
			case pb::rta_type::BGP_ORIGIN:
				rta->type = RTE_ATTR_TYPE_BGP_ORIGIN;
				switch (pb_rta.origin()) {
				case pb::bgp_origin::IGP:
					rta->bgp_origin = BGP_ORIGIN_IGP;
					break;
				case pb::bgp_origin::EGP:
					rta->bgp_origin = BGP_ORIGIN_EGP;
					break;
				case pb::bgp_origin::INCOMPLETE:
					rta->bgp_origin = BGP_ORIGIN_INCOMPLETE;
					break;
				}
				break;
			case pb::rta_type::BGP_AS_PATH:
				rta->type = RTE_ATTR_TYPE_BGP_AS_PATH;
				rta->bgp_as_path = (int *)array_new_size(pb_rta.as_path().as_no_size(),
					sizeof(*rta->bgp_as_path));
				for (size_t k = 0; k < array_size(rta->bgp_as_path); k++)
					rta->bgp_as_path[k] = pb_rta.as_path().as_no(k);
				break;
			case pb::rta_type::BGP_NEXT_HOP:
				rta->type = RTE_ATTR_TYPE_BGP_NEXT_HOP;
				rta->bgp_next_hop = pb_rta.next_hop();
				break;
			case pb::rta_type::BGP_LOCAL_PREF:
				rta->type = RTE_ATTR_TYPE_BGP_LOCAL_PREF;
				rta->bgp_local_pref = pb_rta.local_pref();
				break;
			case pb::rta_type::BGP_COMMUNITY:
				rta->type = RTE_ATTR_TYPE_BGP_COMMUNITY;
				rta->cflags = (struct bgp_cflag *)array_new_size(
					pb_rta.community().cflags_size(),
					sizeof(*rta->cflags));
				for (size_t k = 0; k < array_size(rta->cflags); k++) {
					rta->cflags[k].as_no = pb_rta.community().cflags(k).as_no();
					rta->cflags[k].flag = pb_rta.community().cflags(k).flag();
				}
				break;
			case pb::rta_type::BGP_AGGREGATOR:
				rta->type = RTE_ATTR_TYPE_BGP_AGGREGATOR;
				rta->aggr.ip = pb_rta.aggregator().ip();
				rta->aggr.as_no = pb_rta.aggregator().as_no();
				break;
			case pb::rta_type::OTHER:
				rta->type = RTE_ATTR_TYPE_OTHER;
				rta->other_attr.key = strdup(pb_rta.other().key().c_str());
				if (pb_rta.other().has_value())
					rta->other_attr.value = strdup(pb_rta.other().value().c_str());
				else
					rta->other_attr.value = NULL;
				break;
			}
		}
	}

	return rt;
}
