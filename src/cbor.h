/*
 * Public CBOR API
 */

#ifndef CBOR_H
#define CBOR_H

#include "common.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

struct cbor_stream;
struct nb_buf;


/*
 * CBOR operation result flags
 */
enum cbor_err
{
	CBOR_ERR_OK,		/* no error */
	CBOR_ERR_NOMEM,		/* unable to allocate memory */
	CBOR_ERR_PARSE,		/* unable to parse input */
	CBOR_ERR_ITEM,		/* unexpected item */
	CBOR_ERR_EOF,		/* unexpected EOF */
	CBOR_ERR_RANGE,		/* value is out of range */
	CBOR_ERR_OPER,		/* invalid operation */
	CBOR_ERR_NOFILE,	/* file not found */
	CBOR_ERR_INDEF,		/* (in)definite-length item was unexpected */
	CBOR_ERR_READ,		/* read()-related error */
	CBOR_ERR_UNSUP,		/* operation not supported */
	CBOR_ERR_NOMORE,	/* no more items */
	CBOR_ERR_BREAK,		/* Break was hit */
	CBOR_ERR_NITEMS,	/* Invalid number of items */
};

typedef enum cbor_err cbor_err_t;

struct cbor_stream *cbor_stream_new(struct nb_buf *buf);
void cbor_stream_delete(struct cbor_stream *cs);

char *cbor_stream_strerror(struct cbor_stream *cs);

/*
 * CBOR Simple Values
 * @see RFC 7049, section 2.3.
 */
enum cbor_sval
{
	CBOR_SVAL_FALSE = 20,
	CBOR_SVAL_TRUE,
	CBOR_SVAL_NULL,
	CBOR_SVAL_UNDEF,
};

/*
 * Type of a CBOR Data Item as percieved by the user.
 * The first 6 items coincide with enum major.
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
 */
struct cbor_item
{
	enum cbor_type type;
	bool indefinite;

	/* refactoring aid */
	uint64_t len;				/* go away! */
	uint64_t u64;				/* CBOR_TYPE_UINT */

	union {
		int64_t i64;			/* CBOR_TYPE_NEGINT */
		byte_t *bytes;			/* CBOR_TYPE_BYTES */
		char *str;			/* CBOR_TYPE_TEXT */
		struct cbor_item *items;	/* CBOR_TYPE_ARRAY */
		struct cbor_pair *pairs;	/* CBOR_TYPE_MAP */
		uint64_t tag;			/* CBOR_TYPE_TAG */
		enum cbor_sval sval;
	};
	struct cbor_item *tagged_item;
};

void cbor_item_dump(struct cbor_item *item, FILE *file);
cbor_err_t cbor_stream_dump(struct cbor_stream *cs, FILE *file);

/*
 * CBOR Key-Value Pair.
 * NOTE: this struct's topology affects decode_map_items.
 */
struct cbor_pair
{
	struct cbor_item key;
	struct cbor_item value;
};

/*
 * Encoding/decoding primitives
 */

cbor_err_t cbor_encode_uint8(struct cbor_stream *cs, uint8_t val);
cbor_err_t cbor_decode_uint8(struct cbor_stream *cs, uint8_t *val);

cbor_err_t cbor_encode_uint16(struct cbor_stream *cs, uint16_t val);
cbor_err_t cbor_decode_uint16(struct cbor_stream *cs, uint16_t *val);

cbor_err_t cbor_encode_uint32(struct cbor_stream *cs, uint32_t val);
cbor_err_t cbor_decode_uint32(struct cbor_stream *cs, uint32_t *val);

cbor_err_t cbor_encode_uint64(struct cbor_stream *cs, uint64_t val);
cbor_err_t cbor_decode_uint64(struct cbor_stream *cs, uint64_t *val);

cbor_err_t cbor_encode_int8(struct cbor_stream *cs, int8_t val);
cbor_err_t cbor_decode_int8(struct cbor_stream *cs, int8_t *val);

cbor_err_t cbor_encode_int16(struct cbor_stream *cs, int16_t val);
cbor_err_t cbor_decode_int16(struct cbor_stream *cs, int16_t *val);

cbor_err_t cbor_encode_int32(struct cbor_stream *cs, int32_t val);
cbor_err_t cbor_decode_int32(struct cbor_stream *cs, int32_t *val);

cbor_err_t cbor_encode_int64(struct cbor_stream *cs, int64_t val);
cbor_err_t cbor_decode_int64(struct cbor_stream *cs, int64_t *val);

cbor_err_t cbor_encode_float16(struct cbor_stream *cs, float val);
cbor_err_t cbor_decode_float16(struct cbor_stream *cs, float *val);

cbor_err_t cbor_encode_float32(struct cbor_stream *cs, float val);
cbor_err_t cbor_decode_float32(struct cbor_stream *cs, float *val);

cbor_err_t cbor_encode_float64(struct cbor_stream *cs, double val);
cbor_err_t cbor_decode_float64(struct cbor_stream *cs, double *val);

cbor_err_t cbor_encode_tag(struct cbor_stream *cs, uint64_t tagno);
cbor_err_t cbor_decode_tag(struct cbor_stream *cs, uint64_t *tagno);

cbor_err_t cbor_encode_sval(struct cbor_stream *cs, enum cbor_sval val);
cbor_err_t cbor_decode_sval(struct cbor_stream *cs, enum cbor_sval *val);

cbor_err_t cbor_encode_array_begin(struct cbor_stream *cs, uint64_t len);
cbor_err_t cbor_encode_array_begin_indef(struct cbor_stream *cs);
cbor_err_t cbor_encode_array_end(struct cbor_stream *cs);
cbor_err_t cbor_decode_array_begin(struct cbor_stream *cs, uint64_t *len);
cbor_err_t cbor_decode_array_end(struct cbor_stream *cs);

cbor_err_t cbor_encode_map_begin(struct cbor_stream *cs, size_t len);
cbor_err_t cbor_encode_map_begin_indef(struct cbor_stream *cs);
cbor_err_t cbor_encode_map_end(struct cbor_stream *cs);
cbor_err_t cbor_decode_map_begin(struct cbor_stream *cs, uint64_t *len);
cbor_err_t cbor_decode_map_end(struct cbor_stream *cs);

cbor_err_t cbor_encode_bytes(struct cbor_stream *cs, byte_t *bytes, size_t len);
cbor_err_t cbor_encode_bytes_begin_indef(struct cbor_stream *cs);
cbor_err_t cbor_encode_bytes_end(struct cbor_stream *cs);
cbor_err_t cbor_decode_bytes(struct cbor_stream *cs, byte_t **bytes, size_t *len);

cbor_err_t cbor_encode_text(struct cbor_stream *cs, byte_t *str, size_t len);
cbor_err_t cbor_encode_text_begin_indef(struct cbor_stream *cs);
cbor_err_t cbor_encode_text_end(struct cbor_stream *cs);
cbor_err_t cbor_decode_text(struct cbor_stream *cs, byte_t **str, size_t *len);

/* TODO normalization for byte and text strings */

/*
 * DOM-oriented API: encode and decode generic items.
 */

cbor_err_t cbor_encode_item(struct cbor_stream *cs, struct cbor_item *item);
cbor_err_t cbor_decode_item(struct cbor_stream *cs, struct cbor_item *item);

#endif
