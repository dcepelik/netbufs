#ifndef DIAG_H
#define DIAG_H

#include "error.h"
#include "strbuf.h"
#include "sval.h"
#include <inttypes.h>
#include <stdio.h>

struct isbuf
{
	struct strbuf sb;
	size_t indent_level;
	bool indent_next;
};

/*
 * Diagnostics buffer
 */
struct diag
{
	struct cbor_stream *cs;
	FILE *fout;

	bool have_output;		/* do we have output to print? */
	bool eol_on_next_raw;	/* emit EOL on next diag_log_raw call? */

	struct {
		char offset[16 + 1];
		struct strbuf raw;
		struct strbuf item;
		struct isbuf cbor_is;
		struct isbuf proto_is;
	} output;

	/* TODO more options to come as needed */
	size_t str_dump_maxlen;
	size_t bytes_dump_maxlen;
};

void diag_init(struct diag *diag, struct cbor_stream *cs, FILE *fout);
nb_err_t diag_dump(struct diag *diag, FILE *out);
void diag_free(struct diag *diag);

void diag_dump_line(struct diag *diag);

void diag_log_offset(struct diag *diag, size_t offset);
void diag_log_raw(struct diag *diag, unsigned char *bytes, size_t count);
void diag_log_item(struct diag *diag, char *msg, ...);

/* TODO rename */
void diag_increase(struct diag *diag);
void diag_decrease(struct diag *diag);
void diag_log_cbor(struct diag *diag, char *msg, ...);

void diag_indent_proto(struct diag *diag);
void diag_dedent_proto(struct diag *diag);
void diag_log_proto(struct diag *diag, char *msg, ...);

void diag_log_sval(struct diag *diag, uint64_t u64);
void diag_print_block_stack(struct diag *diag);

#endif
