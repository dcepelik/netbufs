/*
 * Test CBOR encoder and decoder by decoding a valid CBOR buf and encoding
 * all data items it contains into another file.
 *
 * The files are then diffed by run-tests.sh.
 */

#include "cbor.h"
#include "debug.h"

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>


int main(int argc, char *argv[])
{
	char *infn;
	char *outfn;
	struct buf *in;
	struct buf *out;
	struct cbor_stream *cs_in;
	struct cbor_stream *cs_out;
	struct cbor_item item;
	int mode;
	cbor_err_t err;

	assert(argc == 3);

	infn = argv[1];
	outfn = argv[2];

	in = buf_new();
	assert(in != NULL);

	assert(buf_open_file(in, infn, O_RDONLY, 0) == CBOR_ERR_OK);

	out = buf_new();
	assert(out != NULL);

	mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;
	assert(buf_open_file(out, outfn, O_RDWR | O_CREAT | O_TRUNC, mode) == CBOR_ERR_OK);

	cs_in = cbor_stream_new(in);
	assert(cs_in != NULL);

	cs_out = cbor_stream_new(out);
	assert(cs_out != NULL);

	while ((err = cbor_decode_item(cs_in, &item)) == CBOR_ERR_OK) {
		if ((err = cbor_encode_item(cs_out, &item)) != CBOR_ERR_OK)
			break;
	}

	DEBUG_EXPR("%s", cbor_err_to_string(err));
	assert(err == CBOR_ERR_EOF);

	buf_close(in);
	buf_close(out);
	cbor_stream_delete(cs_in);
	cbor_stream_delete(cs_out);

	return EXIT_SUCCESS;
}
