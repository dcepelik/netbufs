/*
 * cbor:
 * CBOR Encoder/Decoder Interface
 */

#ifndef CBOR_H
#define CBOR_H

#include "buffer.h"
#include "common.h"
#include "diag.h"
#include "error.h"
#include "memory.h"
#include "stack.h"
#include "sval.h"
#include "diag.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#define CBOR_BREAK	0xFF

#define diag_finish_item(cs)	diag_if_on((cs)->diag, diag_finish_item_do(cs))


/*
 * Type of a CBOR Data Item as percieved by the user.
 * The first 6 items coincide with enum major (see cbor-internal.h).
 */
enum cbor_type
{
	CBOR_TYPE_UINT,
	CBOR_TYPE_INT,
	CBOR_TYPE_BYTES,
	CBOR_TYPE_TEXT,
	CBOR_TYPE_ARRAY,
	CBOR_TYPE_MAP,
	CBOR_TYPE_TAG,
	CBOR_TYPE_SVAL,
	CBOR_TYPE_FLOAT16,
	CBOR_TYPE_FLOAT32,
	CBOR_TYPE_FLOAT64,
	CBOR_TYPE_BREAK,
};

const char *cbor_type_to_string(enum cbor_type type);

struct cbor_pair;

/*
 * Flags for CBOR items.
 */
enum cbor_flag
{
	CBOR_FLAG_INDEFINITE = 1 << 0,
};

/*
 * CBOR Data Item
 *
 * TODO Use a hashmap for CBOR_TYPE_MAP.
 * TODO Embed short (<= 14B) strings directly into the structure.
 */
struct cbor_item
{
	nb_byte_t type;		/* enum cbor_type */
	nb_byte_t flags;	/* bitmask of enum cbor_flag */
	nb_byte_t rfu[2];	/* reserved for future use */
	uint32_t tag;		/* CBOR_TYPE_TAG */

	/* TODO refactoring aids, remove when done - only use the union */
	uint64_t len;				/* go away! */
	uint64_t u64;				/* CBOR_TYPE_UINT */

	union {
		int64_t i64;			/* CBOR_TYPE_INT */
		nb_byte_t *bytes;		/* CBOR_TYPE_BYTES */
		char *str;			/* CBOR_TYPE_TEXT */
		struct cbor_item *items;	/* CBOR_TYPE_ARRAY */
		struct cbor_pair *pairs;	/* CBOR_TYPE_MAP */
		enum cbor_sval sval;		/* CBOR_TYPE_SVAL */
		struct cbor_item *tagged_item;	/* CBOR_TYPE_TAG */
	};
};

/*
 * CBOR block context for arrays, maps and infeninite-length byte and text streams.
 *
 * TODO Use for tagged items, too.
 */
struct block
{
	enum cbor_type type;	/* type for which this block has been open */
	bool indefinite;	/* is indefinite-lenght encoding used? */
	uint64_t len;		/* intended length of the block (if not indefinite) */
	size_t num_items;	/* actual number of items encoded */
	struct nb_group *group;	/* active netbufs group */
	struct nb_attr *attr;	/* current attribute */
};

struct cbor_stream;

typedef void (cbor_error_handler_t)(struct cbor_stream *cs, nb_err_t err, void *arg);

/*
 * CBOR decoder/encoder context encapsulation.
 */
struct cbor_stream
{
	mempool_t mempool;	/* memory pool for this CBOR stream */
	struct nb_buffer *buf;	/* the buffer being read or written */
	struct stack blocks;	/* block stack (open arrays and maps) */
	struct diag *diag;	/* diagnostics buffer */

	bool peeking;		/* are we peeking? */
	struct cbor_item peek;	/* item to be returned by next predecode() call */

	/* TODO various encoding/decoding options to come as needed */
	nb_err_t err;		/* last error */
	struct strbuf err_buf;	/* error string buffer */
	cbor_error_handler_t *error_handler;
	void *error_handler_arg;
};

void cbor_stream_init(struct cbor_stream *cs, struct nb_buffer *buf);
void cbor_stream_free(struct cbor_stream *cs);

void cbor_stream_set_diag(struct cbor_stream *cs, struct diag *diag);
void cbor_stream_set_error_handler(struct cbor_stream *cs, cbor_error_handler_t *handler,
	void *arg);

void cbor_default_error_handler(struct cbor_stream *cs, nb_err_t err, void *arg);

char *cbor_stream_strerror(struct cbor_stream *cs);
bool cbor_block_stack_empty(struct cbor_stream *cs);

/*
 * CBOR Key-Value Pair.
 * NOTE: this struct's topology affects decode_map_items.
 *
 * TODO Remove this and use an associative array for maps instead.
 */
struct cbor_pair
{
	struct cbor_item key;
	struct cbor_item value;
};

void predecode(struct cbor_stream *cs, struct cbor_item *item);

static inline struct block *top_block(struct cbor_stream *cs)
{
	assert(!stack_is_empty(&cs->blocks));
	return stack_top(&cs->blocks);
}


/* TODO valid fields in item when using peek_item -> manual */
static inline void cbor_peek(struct cbor_stream *cs, struct cbor_item *item)
{
	nb_byte_t major;
	nb_byte_t lbits;
	nb_byte_t hdr = nb_buffer_peek(cs->buf);
	if (hdr == CBOR_BREAK) {
		item->type = CBOR_TYPE_BREAK;
		item->flags = 0;
		item->u64 = 0;
		return;
	}

	major = (hdr & 0xE0) >> 5;
	lbits = hdr & 0x1F;
	item->type = (enum cbor_type)major;
	item->flags = 0;
	if (lbits == 31)
		item->flags |= CBOR_FLAG_INDEFINITE;
}


static inline bool cbor_is_break(struct cbor_stream *cs)
{
	return nb_buffer_peek(cs->buf) == CBOR_BREAK;
}

/*
 * Stream encoding and decoding of items.
 */

nb_err_t cbor_encode_uint8(struct cbor_stream *cs, uint8_t val);
nb_err_t cbor_encode_uint16(struct cbor_stream *cs, uint16_t val);
nb_err_t cbor_encode_uint32_slow(struct cbor_stream *cs, uint32_t val);


static inline nb_err_t cbor_encode_uint32(struct cbor_stream *cs, uint32_t val)
{
	nb_byte_t hdr;
	struct block *top_block;
	
	if (val <= 23) {
		top_block = stack_top(&cs->blocks);
		top_block->num_items++;
		hdr = (CBOR_TYPE_UINT << 5) + val;
		return nb_buffer_write(cs->buf, &hdr, 1) == 1 ? NB_ERR_OK : NB_ERR_WRITE;
	}
	else {
		return cbor_encode_uint32_slow(cs, val);
	}
}


uint64_t decode_uint(struct cbor_stream *cs, uint64_t max);
void cbor_decode_uint8(struct cbor_stream *cs, uint8_t *val);
void cbor_decode_uint16(struct cbor_stream *cs, uint16_t *val);
void cbor_decode_uint32(struct cbor_stream *cs, uint32_t *val);
nb_err_t cbor_encode_uint64_slow(struct cbor_stream *cs, uint64_t val);


static inline nb_err_t cbor_encode_uint64(struct cbor_stream *cs, uint64_t val)
{
	nb_byte_t hdr;
	struct block *top_block;
	
	if (val <= 23) {
		top_block = stack_top(&cs->blocks);
		top_block->num_items++;
		hdr = (CBOR_TYPE_UINT << 5) + val;
		return nb_buffer_write(cs->buf, &hdr, 1) == 1 ? NB_ERR_OK : NB_ERR_WRITE;
	}
	else {
		return cbor_encode_uint32_slow(cs, val);
	}
}


static inline void diag_finish_item_do(struct cbor_stream *cs)
{
	switch (top_block(cs)->type) {
	case CBOR_TYPE_ARRAY:
		diag_eol(cs->diag, true);
		break;

	case CBOR_TYPE_MAP: /* TODO */
		if (top_block(cs)->num_items % 2 == 0) {
			diag_dedent_cbor(cs->diag);
			diag_eol(cs->diag, true);
		}
		else {
			diag_log_cbor(cs->diag, ":");
			diag_eol(cs->diag, false);
			diag_indent_cbor(cs->diag);
		}
		break;

	default:
		if (cbor_block_stack_empty(cs))
			diag_eol(cs->diag, true);
	}
}


static inline void cbor_decode_uint64(struct cbor_stream *cs, uint64_t *val)
{
	nb_byte_t hdr = (nb_byte_t)nb_buffer_peek(cs->buf);
	if (hdr <= 23) {
		nb_buffer_getc(cs->buf);
		top_block(cs)->num_items++;
		*val = hdr;
		diag_log_raw(cs->diag, (nb_byte_t *)val, 1);
		diag_comma(cs->diag); /* append a comma to previous line if needed */
		diag_log_item(cs->diag, "%s(%lu)", cbor_type_to_string(CBOR_TYPE_UINT), *val);
		diag_log_cbor(cs->diag, "%lu", *val);
		diag_finish_item(cs);
	}
	else {
		*val = decode_uint(cs, UINT64_MAX);
	}
}


nb_err_t cbor_encode_int8(struct cbor_stream *cs, int8_t val);
void cbor_decode_int8(struct cbor_stream *cs, int8_t *val);

nb_err_t cbor_encode_int16(struct cbor_stream *cs, int16_t val);
void cbor_decode_int16(struct cbor_stream *cs, int16_t *val);

nb_err_t cbor_encode_int32(struct cbor_stream *cs, int32_t val);
void cbor_decode_int32(struct cbor_stream *cs, int32_t *val);

nb_err_t cbor_encode_int64(struct cbor_stream *cs, int64_t val);
void cbor_decode_int64(struct cbor_stream *cs, int64_t *val);

nb_err_t cbor_encode_tag(struct cbor_stream *cs, uint32_t tagno);
void cbor_decode_tag(struct cbor_stream *cs, uint32_t *tagno);

nb_err_t cbor_encode_sval(struct cbor_stream *cs, enum cbor_sval val);
void cbor_decode_sval(struct cbor_stream *cs, enum cbor_sval *val);

nb_err_t cbor_encode_bool(struct cbor_stream *cs, bool b);
void cbor_decode_bool(struct cbor_stream *cs, bool *b);

nb_err_t cbor_encode_array_begin(struct cbor_stream *cs, uint64_t len);
nb_err_t cbor_encode_array_begin_indef(struct cbor_stream *cs);
nb_err_t cbor_encode_array_end(struct cbor_stream *cs);

void cbor_decode_array_begin(struct cbor_stream *cs, uint64_t *len);
void cbor_decode_array_begin_indef(struct cbor_stream *cs);
void cbor_decode_array_end(struct cbor_stream *cs);

nb_err_t cbor_encode_map_begin(struct cbor_stream *cs, size_t len);
nb_err_t cbor_encode_map_begin_indef(struct cbor_stream *cs);
nb_err_t cbor_encode_map_end(struct cbor_stream *cs);

void cbor_decode_map_begin(struct cbor_stream *cs, uint64_t *len);
void cbor_decode_map_begin_indef(struct cbor_stream *cs);
void cbor_decode_map_end(struct cbor_stream *cs);

nb_err_t cbor_encode_bytes(struct cbor_stream *cs, nb_byte_t *bytes, size_t len);
nb_err_t cbor_encode_bytes_begin_indef(struct cbor_stream *cs);
nb_err_t cbor_encode_bytes_end(struct cbor_stream *cs);

nb_err_t cbor_encode_text(struct cbor_stream *cs, char *str);
nb_err_t cbor_encode_text_begin_indef(struct cbor_stream *cs);
nb_err_t cbor_encode_text_end(struct cbor_stream *cs);
void cbor_decode_text(struct cbor_stream *cs, char **str);

/*
 * DOM-oriented encoding and decoding of (generic) items.
 */

nb_err_t cbor_encode_item(struct cbor_stream *cs, struct cbor_item *item);
void cbor_decode_item(struct cbor_stream *cs, struct cbor_item *item);

#endif
