#include "internal.h"
#include "memory.h"
#include "stream.h"

#include <assert.h>
#include <endian.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>


#define CBOR_5BIT_1B_EXT	24
#define CBOR_5BIT_2B_EXT	25
#define CBOR_5BIT_4B_EXT	26
#define CBOR_5BIT_8B_EXT	27


struct cbor_encoder *cbor_encoder_new()
{
	struct cbor_encoder *enc;

	enc = cbor_malloc(sizeof(*enc));
	return enc;
}


void cbor_encoder_delete(struct cbor_encoder *enc)
{
	cbor_free(enc);
}


static unsigned char cbor_initial(enum cbor_type type)
{
	return ((unsigned char)type) << 5;
}


static cbor_err_t cbor_uint_encode(struct cbor_encoder *enc, uint64_t val,
	bool negative)
{
	uint64_t val_be;
	unsigned char *val_be_bytes = (unsigned char *)&val_be;
	unsigned char bytes[5];
	size_t num_bytes;
	size_t i;

	val_be = htobe64(val);
	bytes[0] = cbor_initial(negative ? CBOR_TYPE_NEGATIVE_INT : CBOR_TYPE_UINT);

	if (val <= 23) {
		num_bytes = 0;
		bytes[0] += val;
	}
	else if (val <= UINT8_MAX) {
		num_bytes = 1;
		bytes[0] += CBOR_5BIT_1B_EXT;
	}
	else if (val <= UINT16_MAX) {
		num_bytes = 2;
		bytes[0] += CBOR_5BIT_2B_EXT;
	}
	else if (val <= UINT32_MAX) {
		num_bytes = 4;
		bytes[0] += CBOR_5BIT_4B_EXT;
	}
	else {
		num_bytes = 8;
		bytes[0] += CBOR_5BIT_8B_EXT;
	}

	for (i = 0; i < num_bytes; i++)
		bytes[1 + i] = val_be_bytes[(8 - num_bytes) + i];

	return cbor_stream_write(&enc->stream, bytes, 1 + num_bytes);
}


cbor_err_t cbor_uint8_encode(struct cbor_encoder *enc, uint8_t val)
{
	return cbor_uint_encode(enc, val, false);
}


cbor_err_t cbor_uint16_encode(struct cbor_encoder *enc, uint16_t val)
{
	return cbor_uint_encode(enc, val, false);
}


cbor_err_t cbor_uint32_encode(struct cbor_encoder *enc, uint32_t val)
{
	return cbor_uint_encode(enc, val, false);
}


cbor_err_t cbor_uint64_encode(struct cbor_encoder *enc, uint64_t val)
{
	return cbor_uint_encode(enc, val, false);
}


static cbor_err_t cbor_int_encode(struct cbor_encoder *enc, int64_t val)
{
	if (val >= 0)
		return cbor_uint_encode(enc, val, false);
	else
		return cbor_uint_encode(enc, (-1 - val), true);
}


cbor_err_t cbor_int8_encode(struct cbor_encoder *enc, int8_t val)
{
	return cbor_int_encode(enc, val);
}


cbor_err_t cbor_int16_encode(struct cbor_encoder *enc, int16_t val)
{
	return cbor_int_encode(enc, val);
}


cbor_err_t cbor_int32_encode(struct cbor_encoder *enc, int32_t val)
{
	return cbor_int_encode(enc, val);
}


cbor_err_t cbor_int64_encode(struct cbor_encoder *enc, int64_t val)
{
	return cbor_int_encode(enc, val);
}