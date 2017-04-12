#ifndef CBOR_H
#define CBOR_H

#include <stdlib.h>
#include <inttypes.h>


struct cbor_encoder;

struct cbor_encoder *cbor_encoder_new();
void cbor_encoder_delete(struct cbor_encoder *enc);

struct cbor_decoder;

struct cbor_decoder *cbor_decoder_new();
void cbor_decoder_delete(struct cbor_decoder *dec);

/*
 * Item-oriented encoder API.
 */

cbor_err_t cbor_int8_encode(struct cbor_encoder *enc, int8_t val);
cbor_err_t cbor_int16_encode(struct cbor_encoder *enc, int16_t val);
cbor_err_t cbor_int32_encode(struct cbor_encoder *enc, int32_t val);
cbor_err_t cbor_int64_encode(struct cbor_encoder *enc, int64_t val);

cbor_err_t cbor_uint8_encode(struct cbor_encoder *enc, uint8_t val);
cbor_err_t cbor_uint16_encode(struct cbor_encoder *enc, uint16_t val);
cbor_err_t cbor_uint32_encode(struct cbor_encoder *enc, uint32_t val);
cbor_err_t cbor_uint64_encode(struct cbor_encoder *enc, uint64_t val);

cbor_err_t cbor_float16_encode(struct cbor_encoder *enc, float val);
cbor_err_t cbor_float32_encode(struct cbor_encoder *enc, float val);
cbor_err_t cbor_float64_encode(struct cbor_encoder *enc, double val);

cbor_err_t cbor_simple_value_encode(struct cbor_encoder *enc, enum cbor_simple_value val);

cbor_err_t cbor_array_encode_begin(struct cbor_encoder *enc, size_t len);
cbor_err_t cbor_array_encode_end(struct cbor_encoder *enc);

cbor_err_t cbor_map_encode_begin(struct cbor_encoder *enc, size_t len);
cbor_err_t cbor_map_encode_end(struct cbor_encoder *enc);

cbor_err_t cbor_text_encode(struct cbor_encoder *enc, char *str, size_t len);
cbor_err_t cbor_text_encode_begin(struct cbor_encoder *enc);
cbor_err_t cbor_text_encode_end(struct cbor_encoder *enc);

cbor_err_t cbor_bytes_encode(struct cbor_encoder *enc, char *bytes, size_t len);
cbor_err_t cbor_bytes_encode_begin(struct cbor_encoder *enc);
cbor_err_t cbor_bytes_encode_end(struct cbor_encoder *enc);

/*
 * Item-oriented decoder API.
 */

cbor_err_t cbor_item_decode(struct cbor_decoder *dec, struct cbor_item *item);

cbor_err_t cbor_int8_decode(struct cbor_encoder *enc, int8_t *val);
cbor_err_t cbor_int16_decode(struct cbor_encoder *enc, int8_t *val);
cbor_err_t cbor_int32_decode(struct cbor_encoder *enc, int32_t *val);
cbor_err_t cbor_int64_decode(struct cbor_encoder *enc, int64_t *val);

cbor_err_t cbor_uint8_decode(struct cbor_encoder *enc, uint8_t *val);
cbor_err_t cbor_uint16_decode(struct cbor_encoder *enc, uint8_t *val);
cbor_err_t cbor_uint32_decode(struct cbor_encoder *enc, uint32_t *val);
cbor_err_t cbor_uint64_decode(struct cbor_encoder *enc, uint64_t *val);

cbor_err_t cbor_float16_decode(struct cbor_decoder *dec, float *val);
cbor_err_t cbor_float32_decode(struct cbor_decoder *dec, float *val);
cbor_err_t cbor_float64_decode(struct cbor_decoder *dec, double *val);

cbor_err_t cbor_simple_value_decode(struct cbor_decoder *dec, enum cbor_simple_value *val);

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
