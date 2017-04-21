/*
 * Test CBOR decoder by feeding it various corrupt data. The decoder should
 * die gracefully and the test should return EXIT_SUCCESS from its main.
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
	struct cbor_stream *in;
	struct cbor_decoder *dec;
	struct cbor_item item;
	cbor_err_t err;

	assert(argc == 2);

	infn = argv[1];

	in = cbor_stream_new();
	assert(in != NULL);

	assert(cbor_stream_open_file(in, infn, O_RDONLY, 0) == CBOR_ERR_OK);

	dec = cbor_decoder_new(in);
	assert(dec != NULL);

	err = CBOR_ERR_ITEM;

	/* bon appétit! */
	while (!cbor_stream_is_eof(in)) {
		if ((err = cbor_decode_item(dec, &item)) != CBOR_ERR_OK)
			break;
	}

	printf("The error detected was: %s\n", cbor_err_to_string(err));
	assert(err != CBOR_ERR_OK);

	cbor_stream_close(in);
	cbor_decoder_delete(dec);

	return EXIT_SUCCESS;
}
