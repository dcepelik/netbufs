#ifndef DIAG_H
#define DIAG_H

#include "error.h"
#include "strbuf.h"
#include "sval.h"

#include <inttypes.h>
#include <stdio.h>


#define diag_log_offset(diag, offset) \
	do { \
		if ((diag)->enabled) \
			diag_log_offset_internal(diag, offset); \
	} while (0)


#define	diag_log_raw(diag, bytes, count) \
	do { \
		if ((diag)->enabled) \
			diag_log_raw_internal(diag, bytes, count); \
	} while (0)


#define	diag_log_item(diag, ...) \
	do { \
		if ((diag)->enabled) \
			diag_log_item_internal(diag, __VA_ARGS__); \
	} while (0)


#define diag_log_cbor(diag, ...) \
	do { \
		if ((diag)->enabled) \
			diag_log_cbor_internal(diag, __VA_ARGS__); \
	} while (0)


#define diag_log_proto(diag, ...) \
	do { \
		if ((diag)->enabled) \
			diag_log_proto_internal(diag, __VA_ARGS__); \
	} while (0)


/*
 * A strbuf-based string buffer with automatic output indentation.
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
	bool empty;				/* is the line empty? */
	bool cbor_comma;		/* should a comma follow in CBOR Diagnostic Notation? */
	char offset[16 + 1];	/* stream offset address in hex */
	struct strbuf raw;		/* raw bytes in hex */
	struct strbuf item;		/* CBOR item, our notation */
	struct isbuf cbor;		/* single line of CBOR Diagnostic Notation */
	struct isbuf proto;		/* protocol information related to this item */
};


/*
 * Buffer for diagnostics output buffering and state information.
 *
 * This isn't the pretties, the smartest nor the most effective data structure
 * in the universe, but it's got the immense power of making the important code
 * much simpler, avoiding lots of obscure, formatting-related corner cases.
 *
 * You see, for example, in the CBOR output column, you have to get valid JSON
 * data, and JSON doesn't allow for a trailing comma after last item in a map
 * or an array, so you have to take care of nicknacks like this.
 *
 * Also, diagnostics isn't performance- neither memory-critical.
 */
struct diag
{
	struct cbor_stream *cs;
	FILE *fout;

	bool enabled;				/* is diagnostics enabled? */
	bool eol_on_next_raw;		/* emit EOL on next diag_log_raw call? */

	struct diag_line lines[2];	/* two lines are buffered */
	unsigned char line_idx;		/* index of active line within lines */
	struct diag_line *line;		/* line = &lines[line_idx] */

	size_t str_dump_maxlen;		/* maximum length of a printed string */
	size_t bytes_dump_maxlen;	/* maximum length of a printed bytestream */
};

void diag_init(struct diag *diag, struct cbor_stream *cs, FILE *fout);
nb_err_t diag_dump(struct diag *diag, FILE *out);
void diag_free(struct diag *diag);

void diag_dump_line(struct diag *diag);
void diag_eol(struct diag *diag, bool cbor_comma);
void diag_comma(struct diag *diag);
void diag_force_newline(struct diag *diag);

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

#endif
