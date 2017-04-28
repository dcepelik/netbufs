#include "buf.h"
#include "parser.h"

#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>


int main(int argc, char *argv[])
{
	char *filename;
	struct rt *rt;
	struct rt *new_rt;
	char *method;
	const char *argv0;
	struct nb_buf *inbuf;
	struct nb_buf *mry_buf;
	struct nb_buf *stdout_buf;

	argv0 = (const char *)basename(argv[0]);

	if (argc != 3) {
		fprintf(stderr, "Usage: %s METHOD FILENAME\n", argv0);
		return EXIT_FAILURE;
	}

	method = argv[1];
	filename = argv[2];

	inbuf = nb_buf_new();
	if (!inbuf) {
		fprintf(stderr, "%s: nb_buf_new() failed: out of memory\n", argv0);
		return EXIT_FAILURE;
	}

	if (nb_buf_open_file(inbuf, filename, O_RDONLY, 0) != NB_ERR_OK) {
		fprintf(stderr, "%s: cannot open file '%s'\n", argv0, filename);
		return EXIT_FAILURE;
	}

	printf("Loading routing table from '%s'\n", filename);

	mry_buf = nb_buf_new();
	if (nb_buf_open_memory(mry_buf) != NB_ERR_OK) {
		fprintf(stderr, "%s: cannot open file '%s'\n", argv0, filename);
		return EXIT_FAILURE;
	}

	rt = deserialize_bird(inbuf);

	printf("Routing table loaded with %lu entries\n", array_size(rt->entries));
	printf("Transferring data using method '%s'\n", method);

	if (strcmp(method, "bird") == 0) {
		//stdout_buf = nb_buf_new();
		//nb_buf_open_file(stdout_buf, "/tmp/out.test", O_RDWR | O_CREAT | O_TRUNC, 0666);

		serialize_bird(rt, mry_buf);
		//nb_buf_flush(mry_buf);
		new_rt = deserialize_bird(mry_buf);
		serialize_bird(new_rt, mry_buf);
	}
	else {
		fprintf(stderr, "%s: unknown method: '%s'\n", argv0, method);
		exit(EXIT_FAILURE);
	}

	printf("Done.\n");

	nb_buf_delete(inbuf);
	nb_buf_delete(mry_buf);
	return EXIT_SUCCESS;
}
