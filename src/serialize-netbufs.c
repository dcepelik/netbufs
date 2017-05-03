#include "benchmark.h"
#include "netbufs.h"


void serialize_netbufs(struct rt *rt, struct nb_buf *buf)
{
	struct rt *rt0 = NULL;
	struct rte *rte0 = NULL;

	char **char_arr = array_new(8, sizeof(*char_arr));
	char_arr = array_push(char_arr, 3);

	char_arr[0] = "ahoj";
	char_arr[1] = "nazdar";
	char_arr[2] = "cau";

	struct netbuf nb;
	nb_init(&nb, buf);

	nb_bind_string(&nb, 0, "bird.org/hello");
	nb_bind_array(&nb, 0, "bird.org/hellos", "bird.org/hello");

	nb_bind_string(&nb, (size_t)&rt0->version_str, "bird.org/rt/version_str");
	//nb_bind_array(&nb, &rt0->entries, "bird.org/rt/entries", "bird.org/rt/entry");
	//nb_bind_string(&nb, &rte0->ifname, "bird.org/rt/entry/ifname");

	nb_send(&nb, rt, "bird.org/rt/version_str");
	nb_send(&nb, char_arr, "bird.org/hellos");
}
