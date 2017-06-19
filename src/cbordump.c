/*
 * cbordump:
 * CBOR stream introspection tool
 *
 * This utility shall be linked with the die()-ing memory wrappers.
 */

#include "cbor.h"
#include "debug.h"
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
	{ "output",		required_argument,	0,	'o' },
	{ "roundtrip",	no_argument,		0, 	'r' },
	{ 0,			0,					0,	0 },
};


static void usage(char *argv0, int status)
{
	fprintf(stderr, "Usage: %s [OPTION]... [FILE]\n\n", argv0);
	fprintf(stderr, "With no FILE or when FILE is -, read stdin.\n\n");
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "\t%-20sWrite output to FILE instead of stdout\n", "-o, --output=FILE");
	fprintf(stderr, "\t%-20sRead the data and write it back\n", "-r, --roundtrip");
	exit(status);
}


int main(int argc, char *argv[])
{
	char *fname_in = "-";
	char *fname_out = "-";
	struct nb_buf *nb_buf_in;
	struct nb_buf *nb_buf_out;
	struct cbor_stream *cbor_in;
	struct cbor_stream *cbor_out;
	struct cbor_item item;
	char *argv0;
	int c;
	int digit_optind;
	int option_index;
	nb_err_t err;

	bool roundtrip = false;
	bool hex_input = false;
	bool hex_output = false;
	
	argv0 = basename(argv[0]);

	while ((c = getopt_long(argc, argv, "hHi:o:p", longopts, &option_index)) != -1) {
		switch (c) {
		case 'h':
			hex_input = true;
			break;
		case 'H':
			hex_output = true;
			break;
		case 'o':
			fname_out = optarg;
			break;
		case 'L':
			roundtrip = true;
			break;
		default:
			usage(argv0, EXIT_FAILURE);
		}

	}

	if (optind < argc)
		fname_in = argv[optind];

	nb_buf_in = nb_buf_new();
	cbor_in = cbor_stream_new(nb_buf_in);

	nb_buf_out = nb_buf_new();
	cbor_out = cbor_stream_new(nb_buf_out);

	if (fname_in && strcmp(fname_in, "-") != 0)
		err = nb_buf_open_file(nb_buf_in, fname_in, O_RDONLY, 0);
	else
		err = nb_buf_open_stdin(nb_buf_in);

	if (err != NB_ERR_OK) {
		fprintf(stderr, "%s: cannot open input file '%s'\n", argv0, fname_in);
		return EXIT_FAILURE;
	}

	if (fname_out && strcmp(fname_out, "-") != 0)
		err = nb_buf_open_file(nb_buf_out, fname_out, O_RDWR | O_CREAT | O_TRUNC, 0666);
	else
		err = nb_buf_open_stdout(nb_buf_out);
	
	if (err != NB_ERR_OK) {
		fprintf(stderr, "%s: cannot open output file '%s'\n", argv0, fname_out);
		return EXIT_FAILURE;
	}

	if (!roundtrip) {
		if ((err = cbor_stream_dump(cbor_in, stdout)) != NB_ERR_OK) {
			fprintf(stderr, "%s: error: %s\n", argv0, cbor_stream_strerror(cbor_in));
			DEBUG_EXPR("%i", err);
			return 2;
		}
		putchar('\n');
	}
	else {
		while (!nb_buf_is_eof(nb_buf_in)) {
			if ((err = cbor_decode_item(cbor_in, &item)) != NB_ERR_OK) {
				fprintf(stderr, "Error decoding item: %s\n",
					cbor_stream_strerror(cbor_in));
				return 2;
			}

			if ((err = cbor_encode_item(cbor_out, &item)) != NB_ERR_OK) {
				fprintf(stderr, "Error encoding item: %s\n",
					cbor_stream_strerror(cbor_out));
				return 2;
			}
		}
	}

	nb_buf_close(nb_buf_in);
	cbor_stream_delete(cbor_in);

	nb_buf_close(nb_buf_out);
	cbor_stream_delete(cbor_out);

	return EXIT_SUCCESS;
}
