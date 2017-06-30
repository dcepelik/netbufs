#ifndef DIAG_H
#define DIAG_H

#include "strbuf.h"
#include "error.h"
#include "sval.h"
#include <inttypes.h>
#include <stdio.h>

struct diag
{
	struct cbor_stream *cs;
	FILE *fout;

	unsigned level;			/* output.cbor indentation level */
	bool indent_next;		/* shall indent next write to output.cbor? */
	bool have_output;		/* do we have output to print? */

	struct {
		char offset[16 + 1];
		struct strbuf raw;
		struct strbuf item;
		struct strbuf cbor;
		struct strbuf proto;
	} output;

	/* TODO more options to come as needed */
	size_t str_dump_maxlen;
	size_t bytes_dump_maxlen;
};

void diag_init(struct diag *diag, struct cbor_stream *cs, FILE *fout);
nb_err_t diag_dump(struct diag *diag, FILE *out);
void diag_free(struct diag *diag);

void diag_log_offset(struct diag *diag, size_t offset);
void diag_log_raw(struct diag *diag, unsigned char *bytes, size_t count);
void diag_log_item(struct diag *diag, char *msg, ...);
void diag_log_cbor(struct diag *diag, char *msg, ...);
void diag_log_proto(struct diag *diag, char *msg, ...);
void diag_increase(struct diag *diag);
void diag_decrease(struct diag *diag);
void diag_dump_line(struct diag *diag);

void diag_log_sval(struct diag *diag, uint64_t u64);

#endif
