#ifndef DIAG_H
#define DIAG_H

#include "error.h"
#include "strbuf.h"
#include "sval.h"

#include <inttypes.h>
#include <stdio.h>

#define DIAG_ENABLE			0

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


#define diag_log_offset(diag, offset) \
	do { \
		if (DIAG_ENABLE && (diag)->enabled) \
			diag_log_offset_internal(diag, offset); \
	} while (0)


#define	diag_log_raw(diag, bytes, count) \
	do { \
		if (DIAG_ENABLE && (diag)->enabled) \
			diag_log_raw_internal(diag, bytes, count); \
	} while (0)


#define	diag_log_item(diag, ...) \
	do { \
		if (DIAG_ENABLE && (diag)->enabled) \
			diag_log_item_internal(diag, __VA_ARGS__); \
	} while (0)


#define diag_log_cbor(diag, ...) \
	do { \
		if (DIAG_ENABLE && (diag)->enabled) \
			diag_log_cbor_internal(diag, __VA_ARGS__); \
	} while (0)


#define diag_log_proto(diag, ...) \
	do { \
		if (DIAG_ENABLE && (diag)->enabled) \
			diag_log_proto_internal(diag, __VA_ARGS__); \
	} while (0)


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

nb_err_t diag_dump_cbor_stream(struct diag *diag, struct cbor_stream *cs);

void diag_eol(struct diag *diag, bool cbor_comma);
void diag_comma(struct diag *diag);
void diag_force_newline(struct diag *diag);
void diag_flush(struct diag *diag);

void diag_log_offset_internal(struct diag *diag, size_t offset);
void diag_log_raw_internal(struct diag *diag, unsigned char *bytes, size_t count);
void diag_log_item_internal(struct diag *diag, char *msg, ...);

/* TODO rename */
void diag_increase(struct diag *diag);
void diag_decrease(struct diag *diag);
void diag_log_cbor_internal(struct diag *diag, char *msg, ...);

void diag_indent_proto(struct diag *diag);
void diag_dedent_proto(struct diag *diag);
void diag_log_proto_internal(struct diag *diag, char *msg, ...);

void diag_log_sval(struct diag *diag, uint64_t u64);
void diag_print_block_stack(struct diag *diag);

const char *diag_get_sval_name(struct diag *diag, enum cbor_sval sval);

void diag_enable_col(struct diag *diag, enum diag_col col);

#endif
