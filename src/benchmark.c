#include "parser.h"

#include <stdio.h>
#include <stdlib.h>


int main(int argc, char *argv[])
{
	char *filename;
	struct parser p;
	struct rt rt;
	char *method;

	if (argc != 3) {
		fprintf(stderr, "Usage: %s METHOD FILENAME\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	method = argv[1];
	filename = argv[2];

	printf("Loading routing table from '%s'\n", filename);

	parser_init(&p, filename);
	parser_parse_rt(&p, &rt);

	printf("Routing table loaded, count=%lu\n", rt.count);
	printf("Transferring data using method '%s'\n", method);

	serialize_bird(&rt);

	printf("Done.\n");
	parser_free(&p);

	return EXIT_SUCCESS;
}
