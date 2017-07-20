#ifndef DIAG_H
#define DIAG_H

#include "error.h"
#include "strbuf.h"
#include "sval.h"

#include <inttypes.h>
#include <stdio.h>

#define DIAG_ENABLE			1

#define DIAG_NUM_COLS			5
#define DIAG_DEFAULT_INDENT_CHAR	'.'
#define DIAG_DEFAULT_INDENT_SIZE	4

/*
 * Diagnostics column.
 */
enum diag_col
{
	DIAG_COL_OFFSET,	/* stream offset column */
	DIAG_COL_RAW,		/* raw data column */
	DIAG_COL_ITEMS,		/* CBOR items column */
	DIAG_COL_CBOR,		/* CBOR Diagnostic Notation column */
	DIAG_COL_PROTO,		/* NetBufs protocol diagnostics column */
};


#define diag_if_on(diag, command) \
	do { \
		if (DIAG_ENABLE && (diag)->enabled) \
			command; \
	} while (0)

/*
 * Conditionally compiled wrappers for diag utility functions
 */

#define diag_comma(diag)		diag_if_on(diag, diag_comma_do(diag))
#define diag_dedent_cbor(diag)		diag_if_on(diag, diag_dedent_cbor_do(diag))
#define diag_dedent_proto(diag)		diag_if_on(diag, diag_dedent_proto_do(diag))
#define diag_eol(diag, cbor_comma)	diag_if_on(diag, diag_eol_do(diag, cbor_comma))
#define diag_flush(diag)		diag_if_on(diag, diag_flush_do(diag))
#define diag_force_newline(diag)	diag_if_on(diag, diag_force_newline_do(diag))
#define diag_indent_cbor(diag)		diag_if_on(diag, diag_indent_cbor_do(diag))
#define diag_indent_proto(diag)		diag_if_on(diag, diag_indent_proto_do(diag))
#define diag_log_cbor(diag, ...) 	diag_if_on(diag, diag_log_cbor_do(diag, __VA_ARGS__))
#define diag_log_item(diag, ...) 	diag_if_on(diag, diag_log_item_do(diag, __VA_ARGS__))
#define diag_log_offset(diag, offset) 	diag_if_on(diag, diag_log_offset_do(diag, offset))
#define diag_log_proto(diag, ...)	diag_if_on(diag, diag_log_proto_do(diag, __VA_ARGS__))
#define diag_log_raw(diag, bytes, n)	diag_if_on(diag, diag_log_raw_do(diag, bytes, n))

/*
 * A string buffer with output indentation.
 */
struct isbuf
{
	struct strbuf sb;
	size_t indent_level;
	bool indent_next;
};

/*
 * Single line of diagnostics output.
 */
struct diag_line
{
	bool empty;		/* is the line empty? */
	bool cbor_comma;	/* should a comma follow in CBOR Diagnostic Notation? */
	size_t offset;		/* stream offset address in hex */
	struct strbuf raw;	/* raw bytes in hex */
	struct strbuf item;	/* CBOR item, our notation */
	struct isbuf cbor;	/* single line of CBOR Diagnostic Notation */
	struct isbuf proto;	/* protocol information related to this item */
};


/*
 * Diagnostics buffer.
 */
struct diag
{
	FILE *fout;

	bool enabled;				/* is diagnostics enabled? */
	bool eol_on_next_raw;			/* emit EOL on next diag_log_raw call? */

	struct diag_line lines[2];		/* two lines are buffered */
	unsigned char line_idx;			/* index of active line within lines */
	struct diag_line *line;			/* line = &lines[line_idx] */

	size_t str_dump_maxlen;			/* max length of a printed string */
	size_t bytes_dump_maxlen;		/* max length of a printed bytestream */
	char indent_char;			/* character used for indentation */
	size_t indent_size;			/* indentation size */
	bool cols_enabled[DIAG_NUM_COLS];	/* which columns are enabled? */
	bool print_json;			/* print JSON instead of CBOR Diagnostic Notation */
};

struct cbor_stream;

void diag_init(struct diag *diag, FILE *fout);
void diag_free(struct diag *diag);

const char *diag_get_sval_name(struct diag *diag, enum cbor_sval sval);
void diag_enable_col(struct diag *diag, enum diag_col col);

nb_err_t diag_dump_cbor_stream(struct diag *diag, struct cbor_stream *cs);

void diag_eol_do(struct diag *diag, bool cbor_comma);
void diag_force_newline_do(struct diag *diag);
void diag_flush_do(struct diag *diag);
void diag_comma_do(struct diag *diag);
void diag_print_block_stack(struct diag *diag);

void diag_log_offset_do(struct diag *diag, size_t offset);
void diag_log_raw_do(struct diag *diag, unsigned char *bytes, size_t count);
void diag_log_item_do(struct diag *diag, char *msg, ...);
void diag_log_cbor_do(struct diag *diag, char *msg, ...);
void diag_log_proto_do(struct diag *diag, char *msg, ...);
void diag_log_sval(struct diag *diag, uint64_t u64);

void diag_indent_cbor_do(struct diag *diag);
void diag_dedent_cbor_do(struct diag *diag);
void diag_indent_proto_do(struct diag *diag);
void diag_dedent_proto_do(struct diag *diag);

#endif
