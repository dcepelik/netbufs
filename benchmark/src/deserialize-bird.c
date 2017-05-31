#include "benchmark.h"
#include "parser.h"


struct rt *deserialize_bird(struct nb_buf *buf)
{
	struct rt *rt;
	struct parser p;

	parser_init(&p, buf);
	rt = nb_malloc(sizeof(*rt));
	parser_parse_rt(&p, rt);
	parser_free(&p);
	return rt;
}
