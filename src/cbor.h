/*
 * CBOR Encoder/Decoder Interface
 */

#ifndef CBOR_H
#define CBOR_H

#include "buf.h"
#include "common.h"
#include "diag.h"
#include "error.h"
#include "stack.h"
#include "sval.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

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
};

const char *cbor_type_string(enum cbor_type type);

struct cbor_pair;

/*
 * CBOR Data Item
 * TODO Fit tags as u32 into padding.
 * 1 B type
 * 1 B flags
 * 2 B padding/explicit rfu
 * 4 B tag
 * 8 B union
 * TODO Use a hashmap for map.
 * //TODO Embedded strings.
 */
struct cbor_item
{
	enum cbor_type type;
	bool indefinite;

	/* refactoring aid */
	uint64_t len;				/* go away! */
	uint64_t u64;				/* CBOR_TYPE_UINT */

	union {
		int64_t i64;			/* CBOR_TYPE_INT */
		nb_byte_t *bytes;		/* CBOR_TYPE_BYTES */
		char *str;			/* CBOR_TYPE_TEXT */
		struct cbor_item *items;	/* CBOR_TYPE_ARRAY */
		struct cbor_pair *pairs;	/* CBOR_TYPE_MAP */
		uint64_t tag;			/* CBOR_TYPE_TAG */
		enum cbor_sval sval;		/* CBOR_TYPE_SVAL */
	};
	struct cbor_item *tagged_item;
};

/*
 * CBOR block context for arrays, maps and infeninite-length byte and text streams.
 * TODO Documentation: netbfus leverages this stack, too.
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

typedef void (cbor_error_handler_t)(struct cbor_stream *cs, nb_err_t err, void *arg);

/*
 * CBOR decoder/encoder context encapsulation.
 */
struct cbor_stream
{
	struct nb_buf *buf;	/* the buffer being read or written */
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

struct cbor_stream *cbor_stream_new(struct nb_buf *buf);
void cbor_stream_delete(struct cbor_stream *cs);

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

/*
 * Stream encoding and decoding of items.
 */

nb_err_t cbor_encode_uint8(struct cbor_stream *cs, uint8_t val);
nb_err_t cbor_decode_uint8(struct cbor_stream *cs, uint8_t *val);

nb_err_t cbor_encode_uint16(struct cbor_stream *cs, uint16_t val);
nb_err_t cbor_decode_uint16(struct cbor_stream *cs, uint16_t *val);

nb_err_t cbor_encode_uint32(struct cbor_stream *cs, uint32_t val);
nb_err_t cbor_decode_uint32(struct cbor_stream *cs, uint32_t *val);

nb_err_t cbor_encode_uint64(struct cbor_stream *cs, uint64_t val);
nb_err_t cbor_decode_uint64(struct cbor_stream *cs, uint64_t *val);

nb_err_t cbor_encode_int8(struct cbor_stream *cs, int8_t val);
nb_err_t cbor_decode_int8(struct cbor_stream *cs, int8_t *val);

nb_err_t cbor_encode_int16(struct cbor_stream *cs, int16_t val);
nb_err_t cbor_decode_int16(struct cbor_stream *cs, int16_t *val);

nb_err_t cbor_encode_int32(struct cbor_stream *cs, int32_t val);
nb_err_t cbor_decode_int32(struct cbor_stream *cs, int32_t *val);

nb_err_t cbor_encode_int64(struct cbor_stream *cs, int64_t val);
nb_err_t cbor_decode_int64(struct cbor_stream *cs, int64_t *val);

nb_err_t cbor_encode_float16(struct cbor_stream *cs, float val);
nb_err_t cbor_decode_float16(struct cbor_stream *cs, float *val);

nb_err_t cbor_encode_float32(struct cbor_stream *cs, float val);
nb_err_t cbor_decode_float32(struct cbor_stream *cs, float *val);

nb_err_t cbor_encode_float64(struct cbor_stream *cs, double val);
nb_err_t cbor_decode_float64(struct cbor_stream *cs, double *val);

nb_err_t cbor_encode_tag(struct cbor_stream *cs, uint64_t tagno);
nb_err_t cbor_decode_tag(struct cbor_stream *cs, uint64_t *tagno);

nb_err_t cbor_encode_sval(struct cbor_stream *cs, enum cbor_sval val);
nb_err_t cbor_decode_sval(struct cbor_stream *cs, enum cbor_sval *val);

nb_err_t cbor_encode_bool(struct cbor_stream *cs, bool b);
nb_err_t cbor_decode_bool(struct cbor_stream *cs, bool *b);

nb_err_t cbor_encode_array_begin(struct cbor_stream *cs, uint64_t len);
nb_err_t cbor_encode_array_begin_indef(struct cbor_stream *cs);
nb_err_t cbor_encode_array_end(struct cbor_stream *cs);

nb_err_t cbor_decode_array_begin(struct cbor_stream *cs, uint64_t *len);
nb_err_t cbor_decode_array_begin_indef(struct cbor_stream *cs);
nb_err_t cbor_decode_array_end(struct cbor_stream *cs);

nb_err_t cbor_encode_map_begin(struct cbor_stream *cs, size_t len);
nb_err_t cbor_encode_map_begin_indef(struct cbor_stream *cs);
nb_err_t cbor_encode_map_end(struct cbor_stream *cs);

nb_err_t cbor_decode_map_begin(struct cbor_stream *cs, uint64_t *len);
nb_err_t cbor_decode_map_begin_indef(struct cbor_stream *cs);
nb_err_t cbor_decode_map_end(struct cbor_stream *cs);

nb_err_t cbor_decode_stream(struct cbor_stream *cs, struct cbor_item *stream,
	nb_byte_t **bytes, size_t *len);
nb_err_t cbor_decode_stream0(struct cbor_stream *cs, struct cbor_item *stream,
	nb_byte_t **bytes, size_t *len);

nb_err_t cbor_encode_bytes(struct cbor_stream *cs, nb_byte_t *bytes, size_t len);
nb_err_t cbor_encode_bytes_begin_indef(struct cbor_stream *cs);
nb_err_t cbor_encode_bytes_end(struct cbor_stream *cs);

nb_err_t cbor_decode_bytes(struct cbor_stream *cs, nb_byte_t **bytes, size_t *len);

nb_err_t cbor_encode_text(struct cbor_stream *cs, char *str);
nb_err_t cbor_encode_text_begin_indef(struct cbor_stream *cs);
nb_err_t cbor_encode_text_end(struct cbor_stream *cs);

nb_err_t cbor_decode_text(struct cbor_stream *cs, char **str);

/* TODO valid fields in item when using peek_item -> manual */
nb_err_t cbor_peek(struct cbor_stream *cs, struct cbor_item *item);

/*
 * DOM-oriented encoding and decoding of (generic) items.
 */

nb_err_t cbor_encode_item(struct cbor_stream *cs, struct cbor_item *item);
nb_err_t cbor_decode_item(struct cbor_stream *cs, struct cbor_item *item);

#endif
