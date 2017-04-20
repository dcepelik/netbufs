#include "parse.h"

#include <stdio.h>
#include <stdlib.h>


int main(int argc, char *argv[])
{
	char *filename;
	struct parser p;
	struct rt rt;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s FILENAME\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	filename = argv[1];

	parser_init(&p, filename);
	parser_parse_rt(&p, &rt);

	printf("Table loaded, count=%lu\n", rt.count);

	parser_free(&p);
	return EXIT_SUCCESS;
}
