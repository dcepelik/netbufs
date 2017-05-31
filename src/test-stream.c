/*
 * Test I/O streams by reading a file and writing the data to another file.
 *
 * The files are then diffed by run-tests.sh.
 */

#include "cbor.h"
#include "internal.h"

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>


#define BUFSIZE	117	/* make sure buffer isn't boundary-aligned with buf's internal buffer */


int main(int argc, char *argv[])
{
	char *infn;
	char *outfn;
	struct nb_buf *in;
	struct nb_buf *out;
	unsigned char *buf;
	size_t len;
	int mode;

	assert(argc == 3);

	infn = argv[1];
	outfn = argv[2];

	buf = malloc(BUFSIZE);
	assert(buf != NULL);

	in = nb_buf_new();
	assert(in != NULL);

	assert(nb_buf_open_file(in, infn, O_RDONLY, 0) == NB_ERR_OK);

	out = nb_buf_new();
	assert(out != NULL);

	mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;
	assert(nb_buf_open_file(out, outfn, O_RDWR | O_CREAT | O_TRUNC, mode) == NB_ERR_OK);

	while ((len = nb_buf_read_len(in, buf, BUFSIZE)) > 0) {
		nb_buf_write(out, buf, len);
	}

	nb_buf_close(in);
	nb_buf_close(out);

	return EXIT_SUCCESS;
}
