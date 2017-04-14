#include "internal.h"
#include "memory.h"
#include "stream.h"

#include <assert.h>
#include <limits.h>
#include <stdbool.h>

#include <inttypes.h>


struct cbor_decoder *cbor_decoder_new()
{
	struct cbor_decoder *dec;

	dec = cbor_malloc(sizeof(*dec));
	return dec;
}


void cbor_decoder_delete(struct cbor_decoder *dec)
{
	cbor_free(dec);
}


static void error(struct cbor_decoder *dec, char *str, ...)
{
	(void) dec;
	(void) str;
}


static cbor_err_t cbor_type_decode(struct cbor_decoder *dec, struct cbor_type *type)
{
	unsigned char bytes[9];
	size_t num_extra_bytes;
	int64_t val_be;
	unsigned char *val_be_bytes = (unsigned char *)&val_be;
	cbor_err_t err;
	cbor_extra_t extra_bits;
	size_t i;

	if ((err = cbor_stream_read(&dec->stream, bytes, 0, 1)) != CBOR_ERR_OK)
		return err;

	type->major = bytes[0] & CBOR_MAJOR_MASK;
	extra_bits = bytes[0] & CBOR_EXTRA_MASK;

	type->indef = (extra_bits == CBOR_EXTRA_VAR_LEN);
	if (type->indef)
		return CBOR_ERR_OK;

	if (extra_bits <= 23) {
		type->val = extra_bits;
		return CBOR_ERR_OK;
	}

	switch (extra_bits) {
	case CBOR_EXTRA_1B:
		num_extra_bytes = 1;
		break;

	case CBOR_EXTRA_2B:
		num_extra_bytes = 2;
		break;

	case CBOR_EXTRA_4B:
		num_extra_bytes = 4;
		break;

	case CBOR_EXTRA_8B:
		num_extra_bytes = 8;
		break;

	default:
		error(dec, "invalid value of extra bits: %u", extra_bits);
		return CBOR_ERR_PARSE;
	}

	if ((err = cbor_stream_read(&dec->stream, bytes, 1, num_extra_bytes)) != CBOR_ERR_OK)
		return err;

	for (i = 0; i < num_extra_bytes; i++)
		val_be_bytes[i] = bytes[1 + i];

	type->val = be64toh(val_be);
	return CBOR_ERR_OK;
}


cbor_err_t cbor_uint8_decode(struct cbor_decoder *dec, uint8_t *val)
{
	struct cbor_type type;
	cbor_err_t err;

	if ((err = cbor_type_decode(dec, &type)) != CBOR_ERR_OK)
		return err;

	if (type.major == CBOR_MAJOR_NEGATIVE_INT)
		return CBOR_ERR_NEGATIVE_INT;

	if (type.major != CBOR_MAJOR_UINT)
		return CBOR_ERR_ITEM;

	if (type.val > UINT8_MAX)
		return CBOR_ERR_RANGE;

	*val = (uint8_t)type.val;
	return CBOR_ERR_OK;
}


static cbor_err_t cbor_int_decode(struct cbor_decoder *dec, int64_t *val)
{
	struct cbor_type type;
	cbor_err_t err;

	if ((err = cbor_type_decode(dec, &type)) != CBOR_ERR_OK)
		return err;

	/* TODO check these range checks */
	if (type.major == CBOR_MAJOR_NEGATIVE_INT) {
		if (type.val > -(INT64_MIN + 1))
			return CBOR_ERR_RANGE;

		*val = -1 - type.val;
	}
	else if (type.major == CBOR_MAJOR_UINT) {
		if (type.val > INT64_MAX)
			return CBOR_ERR_RANGE;

		*val = type.val;
	}
	else {
		return CBOR_ERR_ITEM;
	}

	return CBOR_ERR_OK;
}


cbor_err_t cbor_int8_decode(struct cbor_decoder *dec, int8_t *val)
{
	int64_t val64;
	cbor_err_t err;

	if ((err = cbor_int_decode(dec, &val64)) != CBOR_ERR_OK)
		return err;

	if (val64 < INT8_MIN || val64 > INT8_MAX)
		return CBOR_ERR_RANGE;

	*val = (int8_t)val64;
	return CBOR_ERR_OK;
}


cbor_err_t cbor_array_decode_begin(struct cbor_decoder *dec, size_t *len)
{
	/* TODO I assume size_t is at least uint64_t */
	/* TODO support indefinite length arrays (-> refactoring) */

	struct cbor_type type;
	cbor_err_t err;

	if ((err = cbor_type_decode(dec, &type)) != CBOR_ERR_OK)
		return err;

	if (type.major != CBOR_MAJOR_ARRAY)
		return CBOR_ERR_ITEM;

	return CBOR_ERR_OK;
}


cbor_err_t cbor_array_decode_end(struct cbor_decoder *dec)
{
	/* TODO either validate the number of items processed, or eat a break code */
	return CBOR_ERR_OK;
}
