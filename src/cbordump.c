/*
 * This utility dumps a CBOR document into CBOR diagnostic notation.
 */

#include "cbor.h"

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>


int main(int argc, char *argv[])
{
	char *infn;
	struct buf *in;
	struct cbor_stream *cs;
	struct cbor_item item;
	cbor_err_t err;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
		return EXIT_FAILURE;
	}

	infn = argv[1];

	in = buf_new();
	if (!in) {
		fprintf(stderr, "%s: cannot create input buffer: out of memory\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (buf_open_file(in, infn, O_RDONLY, 0) != CBOR_ERR_OK) {
		fprintf(stderr, "%s: cannot open input file\n", argv[0]);
		return EXIT_FAILURE;
	}

	cs = cbor_stream_new(in);
	if (!cs) {
		fprintf(stderr, "%s: cannot create input stream: out of memory\n", argv[0]);
		return EXIT_FAILURE;
	}

	cbor_stream_dump(cs, stdout);
	putchar('\n');

	buf_close(in);
	cbor_stream_delete(cs);
	return EXIT_SUCCESS;
}
