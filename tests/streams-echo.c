#include "internal.h"
#include "stream.h"
#include "cbor.h"
#include "debug.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define BUFSIZE	1024


int main(int argc, char *argv[])
{
	char *infn;
	char *outfn;
	struct cbor_stream *in;
	struct cbor_stream *out;
	unsigned char *buf;
	size_t nbytes;
	cbor_err_t err;

	if (argc != 3) {
		fprintf(stderr, "Usage: %s <input-file> <output-file>\n", argv[0]);
		return EXIT_FAILURE;
	}

	infn = argv[1];
	outfn = argv[2];

	in = cbor_stream_new();
	if (!in) {
		fprintf(stderr, "%s: cbor_stream_new() failed\n", argv[0]);
		return EXIT_FAILURE;
	}

	if ((err = cbor_stream_open_file(in, infn, O_RDONLY, 0)) != CBOR_ERR_OK) {
		fprintf(stderr, "%s: cannot open input file '%s'\n", argv[0], infn);
		return EXIT_FAILURE;
	}

	out = cbor_stream_new();
	if (!out) {
		fprintf(stderr, "%s: cbor_stream_new() failed\n", argv[0]);
		return EXIT_FAILURE;
	}

	if ((err = cbor_stream_open_file(out, outfn, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR)) != CBOR_ERR_OK) {
		fprintf(stderr, "%s: cannot open output file '%s'\n", argv[0], outfn);
		return EXIT_FAILURE;
	}

	buf = malloc(BUFSIZE);
	if (!buf) {
		return EXIT_FAILURE;
	}

	while ((nbytes = cbor_stream_read(in, buf, 0, BUFSIZE)) != 0) {
		cbor_stream_write(out, buf, nbytes);
	}

	cbor_stream_close(in);
	cbor_stream_close(out);

	return EXIT_SUCCESS;
}
