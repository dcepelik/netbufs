/*
 * Test CBOR decoder by feeding it various corrupt data. The decoder should
 * die gracefully and the test should return EXIT_SUCCESS from its main.
 *
 * The files are then diffed by run-tests.sh.
 */

#include "cbor.h"
#include "debug.h"
#include "buf.h"

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

	err = CBOR_ERR_ITEM;

	/* bon appétit! */
	while (!buf_is_eof(in)) {
		if ((err = cbor_decode_item(cs_in, &item)) != CBOR_ERR_OK)
			break;
	}

	printf("The error detected was: %s\n", cbor_err_to_string(err));
	assert(err != CBOR_ERR_OK);

	buf_close(in);
	cbor_stream_delete(cs_in);

	return EXIT_SUCCESS;
}
