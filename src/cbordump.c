/*
 * cbordump:
 * CBOR stream manipulation utility
 */

#include "cbor.h"
#include "internal.h"

#include <assert.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <getopt.h>


static struct option longopts[] = {
	{ "in",			required_argument,	0,	'i' },
	{ "out",		required_argument,	0,	'o' },
	{ "passthru",		no_argument,		0, 	'p' },
	{ "hex-in",		no_argument,		0,	'h' },
	{ "hex-out",		no_argument,		0,	'H' },
};


int main(int argc, char *argv[])
{
	char *fn_in;
	char *fn_out;
	struct buf *buf_in;
	struct buf *buf_out;
	struct cbor_stream *cs_in;
	struct cbor_stream *cs_out;
	struct cbor_item item;
	cbor_err_t err;
	char *argv0;
	int c;
	int digit_optind;
	int option_index;

	bool do_file_input = false;
	bool do_file_output = false;
	bool passthru = false;
	bool hex_input = false;
	bool hex_output = false;
	
	argv0 = basename(argv[0]);

	while ((c = getopt_long(argc, argv, "hHi:o:p", longopts, &option_index)) != -1) {
		switch (c) {
		case 0:
			fprintf(stderr, "longopt\n");
			break;

		case 'h':
			hex_input = true;
			break;

		case 'H':
			hex_output = true;
			break;

		case 'i':
			do_file_input = true;
			fn_in = optarg;
			break;

		case 'o':
			do_file_output = true;
			fn_out = optarg;
			break;

		case 'p':
			passthru = true;
			break;

		default:
			fprintf(stderr, "Usage: %s [--in <input-file>] [--out <output-file>]\n", argv0);
			return EXIT_FAILURE;
		}

	}

	buf_in = buf_new();
	if (!buf_in) {
		fprintf(stderr, "%s: out of memory\n", argv0);
		return EXIT_FAILURE;
	}

	if (do_file_input)
		err = buf_open_file(buf_in, fn_in, O_RDONLY, 0);
	else
		err = buf_open_stdin(buf_in);

	if (hex_input)
		buf_set_read_filter(buf_in, buf_hex_read_filter);

	if (err != CBOR_ERR_OK) {
		fprintf(stderr, "%s: cannot open input file\n", argv0);
		return EXIT_FAILURE;
	}

	cs_in = cbor_stream_new(buf_in);
	if (!cs_in) {
		fprintf(stderr, "%s: out of memory\n", argv0);
		return EXIT_FAILURE;
	}

	buf_out = buf_new();
	if (!buf_out) {
		fprintf(stderr, "%s: out of memory\n", argv0);
		return EXIT_FAILURE;
	}

	if (do_file_output)
		err = buf_open_file(buf_out, fn_out, O_RDWR | O_CREAT | O_TRUNC, 0666);
	else
		err = buf_open_stdout(buf_out);
	
	if (err != CBOR_ERR_OK) {
		fprintf(stderr, "%s: cannot open output file\n", argv0);
		return EXIT_FAILURE;
	}

	cs_out = cbor_stream_new(buf_out);
	if (!cs_out) {
		fprintf(stderr, "%s: out of memory\n", argv0);
		return EXIT_FAILURE;
	}

	if (hex_output)
		buf_set_write_filter(buf_out, buf_hex_write_filter);

	if (!passthru) {
		if ((err = cbor_stream_dump(cs_in, stdout)) != CBOR_ERR_OK) {
			fprintf(stderr, "%s: error: %s\n", argv0, cbor_stream_strerror(cs_in));
			return 2;
		}
		putchar('\n');
	}
	else {
		while (!buf_is_eof(buf_in)) {
			if ((err = cbor_decode_item(cs_in, &item)) != CBOR_ERR_OK) {
				fprintf(stderr, "Error decoding item: %s\n",
					cbor_stream_strerror(cs_in));
				return 2;
			}

			if ((err = cbor_encode_item(cs_out, &item)) != CBOR_ERR_OK) {
				fprintf(stderr, "Error encoding item: %s\n",
					cbor_stream_strerror(cs_out));
				return 2;
			}
		}
	}

	buf_close(buf_in);
	cbor_stream_delete(cs_in);
	buf_close(buf_out);
	cbor_stream_delete(cs_out);
	return EXIT_SUCCESS;
}