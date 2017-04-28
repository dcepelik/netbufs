#include "buf.h"
#include "parser.h"

#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char *argv[])
{
	char *filename;
	struct parser p;
	struct rt rt;
	char *method;
	const char *argv0;
	struct buf *buf;

	argv0 = (const char *)basename(argv[0]);

	if (argc != 3) {
		fprintf(stderr, "Usage: %s METHOD FILENAME\n", argv0);
		return EXIT_FAILURE;
	}

	method = argv[1];
	filename = argv[2];

	buf = buf_new();
	if (!buf) {
		fprintf(stderr, "%s: buf_new() failed: out of memory\n", argv0);
		return EXIT_FAILURE;
	}

	if (buf_open_file(buf, filename, O_RDONLY, 0) != CBOR_ERR_OK) {
		fprintf(stderr, "%s: cannot open file '%s'\n", argv0, filename);
		return EXIT_FAILURE;
	}

	printf("Loading routing table from '%s'\n", filename);

	parser_init(&p, buf);
	parser_parse_rt(&p, &rt);

	printf("Routing table loaded with %lu entries\n", array_size(rt.entries));
	printf("Transferring data using method '%s'\n", method);

	if (strcmp(method, "bird") == 0) {
		serialize_bird(&rt);
	}
	else {
		fprintf(stderr, "%s: unknown method: '%s'\n", argv0, method);
		exit(EXIT_FAILURE);
	}

	printf("Done.\n");

	parser_free(&p);
	buf_delete(buf);
	return EXIT_SUCCESS;
}
