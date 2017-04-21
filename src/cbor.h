/*
 * Public CBOR API
 */

#ifndef CBOR_H
#define CBOR_H

#include "error.h"
#include <stdbool.h>
#include <stdlib.h>
#include <inttypes.h>

#define CBOR_EXTRA_1B		24
#define CBOR_EXTRA_2B		25
#define CBOR_EXTRA_4B		26
#define CBOR_EXTRA_8B		27
#define CBOR_EXTRA_VAR_LEN	31

struct cbor_encoder;
struct cbor_decoder;
struct cbor_document;
struct cbor_stream;

struct cbor_stream *cbor_stream_new(void);
void cbor_stream_delete(struct cbor_stream *stream);

bool cbor_stream_is_eof(struct cbor_stream *stream);

cbor_err_t cbor_stream_open_file(struct cbor_stream *stream, char *filename, int flags, int mode);
cbor_err_t cbor_stream_open_memory(struct cbor_stream *stream);
void cbor_stream_close(struct cbor_stream *stream);

/*
 * CBOR Major Types
 * @see RFC 7049, section 2.1.
 */
enum cbor_major
{
	CBOR_MAJOR_UINT,
	CBOR_MAJOR_NEGATIVE_INT,
	CBOR_MAJOR_BYTES,
	CBOR_MAJOR_TEXT,
	CBOR_MAJOR_ARRAY,
	CBOR_MAJOR_MAP,
	CBOR_MAJOR_TAG,
	CBOR_MAJOR_OTHER,
};

const char *cbor_major_to_string(enum cbor_major major);

/*
 * Additional Information for Major Type 7
 * @see RFC 7049, section 2.3., Table 1.
 */
enum cbor_minor
{
	/* 0, ..., 23: simple value (direct) */
	CBOR_MINOR_SVAL = 24,
	CBOR_MINOR_FLOAT16,
	CBOR_MINOR_FLOAT32,
	CBOR_MINOR_FLOAT64,
	/* 28, 29, 30: unassigned */
	CBOR_MINOR_BREAK = 31,
};

/*
 * Type of a CBOR Data Item
 */
struct cbor_hdr
{
	enum cbor_major major;
	enum cbor_minor minor;
	bool indef;		/* indefinite-length item? */
	uint64_t u64;		/* value of integers or length of others */
};

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

struct cbor_pair;

/*
 * CBOR Data Item
 */
struct cbor_item
{
	struct cbor_hdr hdr;
	size_t len;		/* XXX this is different from hdr.u64! */

	union {
		int64_t i64;
		unsigned char *bytes;
		char *str;
		struct cbor_item *items;
		struct cbor_pair *pairs;
	};

};

void cbor_item_dump(struct cbor_item *item);

/*
 * CBOR Key-Value Pair.
 *
 * Beware, This struct's topology affects decode_map_items.
 */
struct cbor_pair
{
	struct cbor_item key;
	struct cbor_item value;
};

enum cbor_event
{
	CBOR_EVENT_ARRAY_BEGIN,
	CBOR_EVENT_ARRAY_END,
};

struct cbor_encoder *cbor_encoder_new(struct cbor_stream *stream);
void cbor_encoder_delete(struct cbor_encoder *enc);

struct cbor_decoder *cbor_decoder_new(struct cbor_stream *stream);
void cbor_decoder_delete(struct cbor_decoder *dec);

/*
 * Item-oriented encoder API.
 */

cbor_err_t cbor_encode_item(struct cbor_encoder *enc, struct cbor_item *item);

cbor_err_t cbor_encode_uint8(struct cbor_encoder *enc, uint8_t val);
cbor_err_t cbor_encode_uint16(struct cbor_encoder *enc, uint16_t val);
cbor_err_t cbor_encode_uint32(struct cbor_encoder *enc, uint32_t val);
cbor_err_t cbor_encode_uint64(struct cbor_encoder *enc, uint64_t val);

cbor_err_t cbor_encode_int8(struct cbor_encoder *enc, int8_t val);
cbor_err_t cbor_encode_int16(struct cbor_encoder *enc, int16_t val);
cbor_err_t cbor_encode_int32(struct cbor_encoder *enc, int32_t val);
cbor_err_t cbor_encode_int64(struct cbor_encoder *enc, int64_t val);

cbor_err_t cbor_encode_float16(struct cbor_encoder *enc, float val);
cbor_err_t cbor_encode_float32(struct cbor_encoder *enc, float val);
cbor_err_t cbor_encode_float64(struct cbor_encoder *enc, double val);

cbor_err_t cbor_encode_sval(struct cbor_encoder *enc, enum cbor_sval val);

cbor_err_t cbor_encode_array_begin(struct cbor_encoder *enc, uint64_t len);
cbor_err_t cbor_encode_array_begin_indef(struct cbor_encoder *enc);
cbor_err_t cbor_encode_array_end(struct cbor_encoder *enc);

cbor_err_t cbor_encode_map_begin(struct cbor_encoder *enc, size_t len);
cbor_err_t cbor_encode_map_begin_indef(struct cbor_encoder *enc);
cbor_err_t cbor_encode_map_end(struct cbor_encoder *enc);

cbor_err_t cbor_encode_bytes(struct cbor_encoder *enc, unsigned char *bytes, size_t len);
cbor_err_t cbor_encode_bytes_begin_indef(struct cbor_encoder *enc);
cbor_err_t cbor_encode_bytes_end(struct cbor_encoder *enc);

cbor_err_t cbor_encode_text(struct cbor_encoder *enc, unsigned char *str, size_t len);
/* TODO cbor_encode_text_begin: use as indefinite string, but normalize afterwards */
cbor_err_t cbor_encode_text_begin_indef(struct cbor_encoder *enc);
cbor_err_t cbor_encode_text_end(struct cbor_encoder *enc);

cbor_err_t cbor_encode_tag(struct cbor_encoder *enc, uint64_t tagno);

/*
 * Item-oriented decoder API.
 */

cbor_err_t cbor_decode_uint8(struct cbor_decoder *dec, uint8_t *val);
cbor_err_t cbor_decode_uint16(struct cbor_decoder *dec, uint16_t *val);
cbor_err_t cbor_decode_uint32(struct cbor_decoder *dec, uint32_t *val);
cbor_err_t cbor_decode_uint64(struct cbor_decoder *dec, uint64_t *val);

cbor_err_t cbor_decode_int8(struct cbor_decoder *dec, int8_t *val);
cbor_err_t cbor_decode_int16(struct cbor_decoder *dec, int16_t *val);
cbor_err_t cbor_decode_int32(struct cbor_decoder *dec, int32_t *val);
cbor_err_t cbor_decode_int64(struct cbor_decoder *dec, int64_t *val);

cbor_err_t cbor_decode_float16(struct cbor_decoder *dec, float *val);
cbor_err_t cbor_decode_float32(struct cbor_decoder *dec, float *val);
cbor_err_t cbor_decode_float64(struct cbor_decoder *dec, double *val);

cbor_err_t cbor_decode_sval(struct cbor_decoder *dec, enum cbor_sval *val);

cbor_err_t cbor_decode_array_begin(struct cbor_decoder *dec, uint64_t *len);
cbor_err_t cbor_decode_array_end(struct cbor_decoder *dec);

cbor_err_t cbor_decode_map_begin(struct cbor_decoder *dec, uint64_t *len);
cbor_err_t cbor_decode_map_end(struct cbor_decoder *dec);

cbor_err_t cbor_decode_bytes(struct cbor_decoder *dec, unsigned char **bytes, size_t *len);
cbor_err_t cbor_decode_text(struct cbor_decoder *dec, unsigned char **str, size_t *len);

cbor_err_t cbor_decode_tag(struct cbor_decoder *dec, uint64_t *tagno);

cbor_err_t cbor_decode_item(struct cbor_decoder *dec, struct cbor_item *item);

/*
 * DOM-oriented decoder API.
 */

struct cbor_document *cbor_document_decode(struct cbor_decoder *dec);
void cbor_document_delete(struct cbor_document *doc);

#endif
