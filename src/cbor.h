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

struct cbor_type
{
	enum cbor_major major;	/* major type */
	bool indef;		/* indefinite-length type? */
	uint64_t val;		/* value of integers or length of others */
};

const char *cbor_type_to_string(struct cbor_type type);

/*
 * CBOR Simple Values
 * @see RFC 7049, section 2.3.
 */
enum cbor_sval
{
	CBOR_SIMPLE_VALUE_FALSE = 20,
	CBOR_SIMPLE_VALUE_TRUE,
	CBOR_SIMPLE_VALUE_NULL,
	CBOR_SIMPLE_VALUE_UNDEF,
};

/*
 * CBOR data item encapsulation
 */
struct cbor_item
{
	struct cbor_type type;
};

enum cbor_event
{
	CBOR_EVENT_ARRAY_BEGIN,
	CBOR_EVENT_ARRAY_END,
};

struct cbor_encoder *cbor_encoder_new();
void cbor_encoder_delete(struct cbor_encoder *enc);

struct cbor_decoder *cbor_decoder_new();
void cbor_decoder_delete(struct cbor_decoder *dec);

/*
 * Item-oriented encoder API.
 */

cbor_err_t cbor_uint8_encode(struct cbor_encoder *enc, uint8_t val);
cbor_err_t cbor_uint16_encode(struct cbor_encoder *enc, uint16_t val);
cbor_err_t cbor_uint32_encode(struct cbor_encoder *enc, uint32_t val);
cbor_err_t cbor_uint64_encode(struct cbor_encoder *enc, uint64_t val);

cbor_err_t cbor_int8_encode(struct cbor_encoder *enc, int8_t val);
cbor_err_t cbor_int16_encode(struct cbor_encoder *enc, int16_t val);
cbor_err_t cbor_int32_encode(struct cbor_encoder *enc, int32_t val);
cbor_err_t cbor_int64_encode(struct cbor_encoder *enc, int64_t val);

cbor_err_t cbor_float16_encode(struct cbor_encoder *enc, float val);
cbor_err_t cbor_float32_encode(struct cbor_encoder *enc, float val);
cbor_err_t cbor_float64_encode(struct cbor_encoder *enc, double val);

cbor_err_t cbor_sval_encode(struct cbor_encoder *enc, enum cbor_sval val);

cbor_err_t cbor_array_encode_begin(struct cbor_encoder *enc, uint64_t len);
cbor_err_t cbor_array_encode_begin_indef(struct cbor_encoder *enc);
cbor_err_t cbor_array_encode_end(struct cbor_encoder *enc);

cbor_err_t cbor_map_encode_begin(struct cbor_encoder *enc, size_t len);
cbor_err_t cbor_map_encode_begin_indef(struct cbor_encoder *enc);
cbor_err_t cbor_map_encode_end(struct cbor_encoder *enc);

cbor_err_t cbor_text_encode(struct cbor_encoder *enc, char *str, size_t len);
/* TODO cbor_text_encode_begin: use as indefinite string, but normalize afterwards */
cbor_err_t cbor_text_encode_begin_indef(struct cbor_encoder *enc);
cbor_err_t cbor_text_encode_end(struct cbor_encoder *enc);

cbor_err_t cbor_bytes_encode(struct cbor_encoder *enc, unsigned char *bytes, size_t len);
cbor_err_t cbor_bytes_encode_begin_indef(struct cbor_encoder *enc);
cbor_err_t cbor_bytes_encode_end(struct cbor_encoder *enc);

/*
 * Item-oriented decoder API.
 */

cbor_err_t cbor_item_decode(struct cbor_decoder *dec, struct cbor_item *item);

cbor_err_t cbor_uint8_decode(struct cbor_decoder *dec, uint8_t *val);
cbor_err_t cbor_uint16_decode(struct cbor_decoder *dec, uint8_t *val);
cbor_err_t cbor_uint32_decode(struct cbor_decoder *dec, uint32_t *val);
cbor_err_t cbor_uint64_decode(struct cbor_decoder *dec, uint64_t *val);

cbor_err_t cbor_int8_decode(struct cbor_decoder *dec, int8_t *val);
cbor_err_t cbor_int16_decode(struct cbor_decoder *dec, int8_t *val);
cbor_err_t cbor_int32_decode(struct cbor_decoder *dec, int32_t *val);
cbor_err_t cbor_int64_decode(struct cbor_decoder *dec, int64_t *val);

cbor_err_t cbor_float16_decode(struct cbor_decoder *dec, float *val);
cbor_err_t cbor_float32_decode(struct cbor_decoder *dec, float *val);
cbor_err_t cbor_float64_decode(struct cbor_decoder *dec, double *val);

cbor_err_t cbor_sval_decode(struct cbor_decoder *dec, enum cbor_sval *val);

cbor_err_t cbor_array_decode_begin(struct cbor_decoder *dec, size_t *len);
cbor_err_t cbor_array_decode_end(struct cbor_decoder *dec);

cbor_err_t cbor_map_decode_begin(struct cbor_decoder *dec, size_t *len);
cbor_err_t cbor_map_decode_end(struct cbor_decoder *dec);

cbor_err_t cbor_text_decode(struct cbor_decoder *dec, char **str);
cbor_err_t cbor_bytes_decode(struct cbor_decoder *dec, char **bytes);

/*
 * DOM-oriented decoder API.
 */

struct cbor_document *cbor_document_decode(struct cbor_decoder *dec);
void cbor_document_delete(struct cbor_document *doc);

#endif
