/*
 * nbdiag:
 * Diagnostics utility for CBOR-encoded netbuf messages
 *
 * This utility shall be linked with the die()-ing memory wrappers.
 *
 * TODO String and bytestream trimming, dynamic col widths.
 */

#include "buffer.h"
#include "cbor.h"
#include "common.h"
#include "debug.h"
#include "diag.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define EXIT_DATA_ERROR	2


static const char *argv0;
static const char *optstring = "01234b:i:I:emo:t:Jh";

static char *fname_in = "-";
static char *fname_out = "-";
static struct nb_buffer *buf_in;
static struct nb_buffer *buf_out;
static struct cbor_stream cbor_in;
static struct cbor_stream cbor_out;

bool cols_given;
static struct diag diag;
bool mirror;


static struct option longopts[] = {
	{ "escape",		no_argument,		0,	'e' },
	{ "indent-char",	required_argument,	0,	'i' },
	{ "indent-size",	required_argument,	0,	'I' },
	{ "mirror",		no_argument,		0,	'm' },
	{ "output",		required_argument,	0,	'o' },
	{ "trim-bytes",		required_argument,	0,	'b' },
	{ "trim-text",		required_argument,	0,	't' },
	{ "help",		no_argument,		0,	'h' },
	{ "json",		no_argument,		0,	'J' },
	{ 0,			0,			0,	0 },
};


static void usage(int status)
{
	fprintf(stderr, "Usage: %s [OPTION]... [FILE]\n\n", argv0);

	fprintf(stderr, "With no FILE or when FILE is -, read stdin. Default columns are -04.\n\n");

	fprintf(stderr, "Mode switch:\n");
	fprintf(stderr, "  -m, --mirror    Mirror the CBOR stream (pass-through)\n");
	fprintf(stderr, "  -J, --json      Print JSON instead of CBOR Diagnostic Notation\n\n");

	fprintf(stderr, "Column selection options:\n");
	fprintf(stderr, "  -0    Print stream offset column\n");
	fprintf(stderr, "  -1    Print raw data column\n");
	fprintf(stderr, "  -2    Print CBOR items column\n");
	fprintf(stderr, "  -3    Print CBOR Diagnostic Notation column\n");
	fprintf(stderr, "  -4    Print NetBufs diagnostics data column\n\n");

	fprintf(stderr, "Formatting options:\n");
	fprintf(stderr, "  -e, --escape              Escape special characters in text\n");
	fprintf(stderr, "  -t, --trim-text=NUM       Limit text output to NUM characters\n");
	fprintf(stderr, "  -b, --trim-bytes=NUM      Limit bytes output to NUM bytes\n");
	fprintf(stderr, "  -i, --indent-char=CHAR    Use CHAR for indentation (dot by default)\n");
	fprintf(stderr, "  -I, --indent-size=NUM     Indent size is NUM chars\n\n");

	fprintf(stderr, "Other options:\n");
	fprintf(stderr, "  -o, --output=FILE     Write output to FILE instead of stdout\n");
	fprintf(stderr, "  -h, --help            Print this message and exit\n\n");
	exit(status);
}


static void cbor_err_handler(struct cbor_stream *cs, nb_err_t err, void *arg)
{
	struct diag *diag = (struct diag *)arg;
	//fputs(strbuf_get_string(&diag->strbuf), stdout); /* TODO */
	fprintf(stderr, "\n%s: error while decoding CBOR stream: %s\n", argv0,
		cbor_stream_strerror(cs));
	exit(EXIT_DATA_ERROR);
}


static long long argtoll(const char *arg, const char *argname)
{
	char *end;
	long long val;

	val = strtoll(arg, &end, 10);
	if ((val == LLONG_MIN || val == LLONG_MAX) && errno == ERANGE) {
		fprintf(stderr, "%s: value of option %s doesn't fit long long\n", argv0, argname);
		exit(EXIT_FAILURE);
	}
	if (*end != '\0') {
		fprintf(stderr, "%s: value of option %s is not numeric\n", argv0, argname);
		exit(EXIT_FAILURE);
	}

	return val;
}


static void parse_args(int argc, char *argv[])
{
	int digit_optind;
	int option_index;
	int c;
	size_t num;

	while ((c = getopt_long(argc, argv, optstring, longopts, &option_index)) != -1) {
		switch (c) {
		case '0':
			cols_given = true;
			diag_enable_col(&diag, DIAG_COL_OFFSET);
			break;
		case '1':
			cols_given = true;
			diag_enable_col(&diag, DIAG_COL_RAW);
			break;
		case '2':
			cols_given = true;
			diag_enable_col(&diag, DIAG_COL_ITEMS);
			break;
		case '3':
			cols_given = true;
			diag_enable_col(&diag, DIAG_COL_CBOR);
			break;
		case '4':
			cols_given = true;
			diag_enable_col(&diag, DIAG_COL_PROTO);
			break;
		case 'b':
			diag.bytes_dump_maxlen = argtoll(optarg, "-b|--trim-bytes");
			break;
		case 't':
			diag.str_dump_maxlen = argtoll(optarg, "-t|--trim-text");
			break;
		case 'i':
			if (strlen(optarg) != 1) {
				fprintf(stderr, "%s: value of -i|--indent-char must be a "
					"single char, string \"%s\" was given\n",
					argv0, optarg);
				exit(EXIT_FAILURE);
			}
			diag.indent_char = optarg[0];
			break;
		case 'I':
			diag.indent_size = argtoll(optarg, "-I|--indent-size");
			break;
		case 'm':
			mirror = true;
			break;
		case 'o':
			fname_out = optarg;
			break;
		case 'J':
			diag.print_json = true;
			break;
		case 'h':
			usage(EXIT_SUCCESS);
		default:
			usage(EXIT_FAILURE);
		}
	}

	if (optind < argc)
		fname_in = argv[optind];

	if (!cols_given) {
		diag_enable_col(&diag, DIAG_COL_RAW);
		diag_enable_col(&diag, DIAG_COL_CBOR);
	}
}


static void mirror_stream(void)
{
	struct cbor_item item;
	nb_err_t err;

	while (!nb_buffer_is_eof(buf_in)) {
		cbor_decode_item(&cbor_in, &item);
		cbor_encode_item(&cbor_out, &item);
	}
}


int main(int argc, char *argv[])
{
	nb_err_t err;

	diag_init(&diag, stdout);
	
	argv0 = basename(argv[0]);
	parse_args(argc, argv);

	buf_in = nb_buffer_new();
	cbor_stream_init(&cbor_in, buf_in);

	buf_out = nb_buffer_new();
	cbor_stream_init(&cbor_out, buf_out);

	if (fname_in && strcmp(fname_in, "-") != 0)
		err = nb_buffer_open_file(buf_in, fname_in, O_RDONLY, 0);
	else
		err = nb_buffer_open_stdin(buf_in);

	if (err != NB_ERR_OK) {
		fprintf(stderr, "%s: Cannot open input file '%s': %s\n", argv0, fname_in,
			strerror(errno));
		return EXIT_FAILURE;
	}

	if (fname_out && strcmp(fname_out, "-") != 0)
		err = nb_buffer_open_file(buf_out, fname_out, O_RDWR | O_CREAT | O_TRUNC, 0666);
	else
		err = nb_buffer_open_stdout(buf_out);
	
	if (err != NB_ERR_OK) {
		fprintf(stderr, "%s: Cannot open output file '%s': %s\n", argv0, fname_out,
			strerror(errno));
		return EXIT_FAILURE;
	}

	if (!mirror) {
		cbor_stream_set_diag(&cbor_in, &diag);
		cbor_stream_set_error_handler(&cbor_in, cbor_err_handler, &diag);
		diag_dump_cbor_stream(&diag, &cbor_in);
		diag_free(&diag);

		if (!cbor_block_stack_empty(&cbor_in)) {
			fprintf(stderr, "%s: warning: some blocks are still open after EOF\n", argv0);
			return 2;
		}
	}
	else {
		mirror_stream();
	}

	nb_buffer_close(buf_in);
	cbor_stream_free(&cbor_in);

	nb_buffer_close(buf_out);
	cbor_stream_free(&cbor_out);

	return EXIT_SUCCESS;
}
