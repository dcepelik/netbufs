/*
 * Test I/O streams by reading a file and writing the data to another file.
 *
 * The files are then diffed by run-tests.sh.
 */

#include "buffer-internal.h"
#include "buffer.h"
#include "cbor.h"
#include "common.h"
#include "memory.h"

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>


#define BUFSIZE	117	/* make sure buffer isn't boundary-aligned with buf's internal buffer */


int main(int argc, char *argv[])
{
	char *fn_in;
	char *fn_out;
	int fd_in;
	int fd_out;
	struct nb_buffer *in;
	struct nb_buffer *out;
	unsigned char *buf;
	size_t len;
	int fd_out_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;

	assert(argc == 3);

	fn_in = argv[1];
	fn_out = argv[2];

	fd_in = open(fn_in, O_RDONLY, 0);
	fd_out = open(fn_out, O_RDWR | O_CREAT | O_TRUNC, fd_out_mode);

	assert(fd_in != -1);
	assert(fd_out != -1);

	buf = nb_malloc(BUFSIZE);
	in = nb_buffer_new_file(fd_in);
	out = nb_buffer_new_file(fd_out);

	while ((len = nb_buffer_read_len(in, buf, BUFSIZE)) > 0)
		nb_buffer_write(out, buf, len);

	nb_buffer_delete(in);
	nb_buffer_delete(out);

	return EXIT_SUCCESS;
}
