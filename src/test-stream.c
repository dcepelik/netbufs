#include "cbor.h"
#include "debug.h"
#include "internal.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>


#define BUFSIZE	117	/* make sure buffer isn't boundary-aligned with stream's internal buffer */


int main(int argc, char *argv[])
{
	char *infn;
	char *outfn;
	struct cbor_stream *in;
	struct cbor_stream *out;
	unsigned char *buf;
	size_t len;
	int mode;

	assert(argc == 3);

	infn = argv[1];
	outfn = argv[2];

	buf = malloc(BUFSIZE);
	assert(buf != NULL);

	in = cbor_stream_new();
	assert(in != NULL);
	assert(cbor_stream_open_file(in, infn, O_RDONLY, 0) == CBOR_ERR_OK);

	out = cbor_stream_new();
	assert(out != NULL);

	mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;
	assert(cbor_stream_open_file(out, outfn, O_RDWR | O_CREAT | O_TRUNC, mode) == CBOR_ERR_OK);

	while ((len = cbor_stream_read_len(in, buf, BUFSIZE)) > 0) {
		cbor_stream_write(out, buf, len);
	}

	cbor_stream_close(in);
	cbor_stream_close(out);

	return EXIT_SUCCESS;
}
