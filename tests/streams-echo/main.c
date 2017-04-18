#include "internal.h"
#include "stream.h"
#include "cbor.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

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

	if ((err = cbor_stream_open_file(in, infn, O_RDONLY)) != CBOR_ERR_OK) {
		return EXIT_FAILURE;
	}

	if ((err = cbor_stream_open_file(out, outfn, O_WRONLY)) != CBOR_ERR_OK) {
		return EXIT_FAILURE;
	}

	buf = malloc(BUFSIZE);
	if (!buf) {
		return EXIT_FAILURE;
	}

	while ((nbytes = cbor_stream_read(in, buf, 0, BUFSIZE)) != 0) {
		cbor_stream_write(out, buf, nbytes); /* TODO */
	}

	return EXIT_SUCCESS;
}
