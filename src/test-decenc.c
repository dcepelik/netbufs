/*
 * Test CBOR encoder and decoder by decoding a valid CBOR stream and encoding
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
	struct cbor_stream *in;
	struct cbor_stream *out;
	struct cbor_decoder *dec;
	struct cbor_encoder *enc;
	struct cbor_item item;
	int mode;
	cbor_err_t err;

	assert(argc == 3);

	infn = argv[1];
	outfn = argv[2];

	in = cbor_stream_new();
	assert(in != NULL);

	assert(cbor_stream_open_file(in, infn, O_RDONLY, 0) == CBOR_ERR_OK);

	out = cbor_stream_new();
	assert(out != NULL);

	mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;
	assert(cbor_stream_open_file(out, outfn, O_RDWR | O_CREAT | O_TRUNC, mode) == CBOR_ERR_OK);

	dec = cbor_decoder_new(in);
	assert(dec != NULL);

	enc = cbor_encoder_new(out);
	assert(enc != NULL);

	while ((err = cbor_decode_item(dec, &item)) == CBOR_ERR_OK) {
		cbor_item_dump(&item);
	}

	DEBUG_EXPR("%i", err);
	assert(err == CBOR_ERR_EOF);

	cbor_decoder_delete(dec);
	cbor_encoder_delete(enc);
	cbor_stream_close(in);
	cbor_stream_close(out);

	return EXIT_SUCCESS;
}
