/*
 * This utility dumps a CBOR document into CBOR diagnostic notation.
 */

#include "cbor.h"
#include "internal.h"

#include <assert.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>


int main(int argc, char *argv[])
{
	char *fn;
	struct buf *in;
	struct cbor_stream *cs;
	struct cbor_item item;
	cbor_err_t err;
	char *argv0;
	
	argv0 = basename(argv[0]);

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <filename>\n", argv0);
		return EXIT_FAILURE;
	}

	fn = argv[1];

	in = buf_new();
	if (!in) {
		fprintf(stderr, "%s: out of memory\n", argv0);
		return EXIT_FAILURE;
	}

	if (strcmp(fn, "-") == 0) {
		err = buf_open_stdin(in);
		buf_set_filter(in, buf_hex_filter);

	}
	else {
		err = buf_open_file(in, fn, O_RDONLY, 0);
	}

	if (err != CBOR_ERR_OK) {
		fprintf(stderr, "%s: cannot open input file\n", argv0);
		return EXIT_FAILURE;
	}

	cs = cbor_stream_new(in);
	if (!cs) {
		fprintf(stderr, "%s: out of memory\n", argv0);
		return EXIT_FAILURE;
	}

	cbor_stream_dump(cs, stdout);
	putchar('\n');

	buf_close(in);
	cbor_stream_delete(cs);
	return EXIT_SUCCESS;
}
