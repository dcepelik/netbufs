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
	struct cbor_stream *cs_in;
	struct cbor_item item;
	cbor_err_t err;

	assert(argc == 2);

	infn = argv[1];

	in = buf_new();
	assert(in != NULL);

	assert(buf_open_file(in, infn, O_RDONLY, 0) == CBOR_ERR_OK);

	cs_in = cbor_stream_new(in);
	assert(cs_in != NULL);

	while ((err = cbor_decode_item(cs_in, &item)) == CBOR_ERR_OK) {
		cbor_item_dump(&item, stdout);
		fprintf(stdout, ", \n");
	}

	buf_close(in);
	cbor_stream_delete(cs_in);
	return EXIT_SUCCESS;
}
