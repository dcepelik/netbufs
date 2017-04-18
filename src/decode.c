#include "debug.h"
#include "internal.h"
#include "memory.h"
#include "stream.h"

#include <assert.h>
#include <endian.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>


struct cbor_decoder *cbor_decoder_new(struct cbor_stream *stream)
{
	struct cbor_decoder *dec;

	dec = cbor_malloc(sizeof(*dec));
	dec->stream = stream;

	if (!stack_init(&dec->blocks, 4, sizeof(struct block))) {
		free(dec);
		return NULL;
	}

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

	if ((err = cbor_stream_read(dec->stream, bytes, 0, 1)) != CBOR_ERR_OK)
		return err;

	type->major = (bytes[0] & CBOR_MAJOR_MASK) >> 5;
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

	if ((err = cbor_stream_read(dec->stream, bytes, 1, num_extra_bytes)) != CBOR_ERR_OK)
		return err;

	for (i = 0; i < num_extra_bytes; i++)
		val_be_bytes[i] = bytes[1 + i];

	type->val = be64toh(val_be);
	return CBOR_ERR_OK;
}


static uint64_t cbor_uint_decode(struct cbor_decoder *dec, cbor_err_t *err, uint64_t max_value)
{
	struct cbor_type type;

	if ((*err = cbor_type_decode(dec, &type)) != CBOR_ERR_OK)
		return 0;

	if (type.major == CBOR_MAJOR_NEGATIVE_INT) {
		*err = CBOR_ERR_NEGATIVE_INT;
		return 0;
	}

	if (type.major != CBOR_MAJOR_UINT) {
		*err = CBOR_ERR_ITEM;
		return 0;
	}

	if (type.val > max_value) {
		*err = CBOR_ERR_RANGE;
		return 0;
	}

	*err = CBOR_ERR_OK;
	return type.val;
}


cbor_err_t cbor_uint8_decode(struct cbor_decoder *dec, uint8_t *val)
{
	cbor_err_t err;
	*val = (uint8_t)cbor_uint_decode(dec, &err, UINT8_MAX);
	return err;
}


cbor_err_t cbor_uint16_decode(struct cbor_decoder *dec, uint16_t *val)
{
	cbor_err_t err;
	*val = (uint16_t)cbor_uint_decode(dec, &err, UINT16_MAX);
	return err;
}


cbor_err_t cbor_uint32_decode(struct cbor_decoder *dec, uint32_t *val)
{
	cbor_err_t err;
	*val = (uint32_t)cbor_uint_decode(dec, &err, UINT32_MAX);
	return err;
}


cbor_err_t cbor_uint64_decode(struct cbor_decoder *dec, uint64_t *val)
{
	cbor_err_t err;
	*val = (uint64_t)cbor_uint_decode(dec, &err, UINT64_MAX);
	return err;
}


/*
 * TODO have a look at the range checks and make sure nothing overflows
 */
static uint64_t cbor_int_decode(struct cbor_decoder *dec, cbor_err_t *err,
	int64_t min_value, uint64_t max_value)
{
	struct cbor_type type;

	if ((*err = cbor_type_decode(dec, &type)) != CBOR_ERR_OK)
		return 0;

	if (type.major == CBOR_MAJOR_NEGATIVE_INT) {
		if (type.val > -(INT64_MIN + 1) || type.val < min_value) {
			*err = CBOR_ERR_RANGE;
			return 0;
		}

		return (-1 - type.val);
	}
	
	if (type.major == CBOR_MAJOR_UINT) {
		if (type.val > max_value) {
			*err = CBOR_ERR_RANGE;
			return 0;
		}

		return type.val;
	}

	*err = CBOR_ERR_ITEM;
	return 0;
}


cbor_err_t cbor_int8_decode(struct cbor_decoder *dec, int8_t *val)
{
	cbor_err_t err;
	*val = (int8_t)cbor_int_decode(dec, &err, INT8_MIN, INT8_MAX);
	return err;
}


cbor_err_t cbor_int16_decode(struct cbor_decoder *dec, int16_t *val)
{
	cbor_err_t err;
	*val = (int16_t)cbor_int_decode(dec, &err, INT16_MIN, INT16_MAX);
	return err;
}


cbor_err_t cbor_int32_decode(struct cbor_decoder *dec, int32_t *val)
{
	cbor_err_t err;
	*val = (int32_t)cbor_int_decode(dec, &err, INT32_MIN, INT32_MAX);
	return err;
}


cbor_err_t cbor_int64_decode(struct cbor_decoder *dec, int64_t *val)
{
	cbor_err_t err;
	*val = (int64_t)cbor_int_decode(dec, &err, INT64_MIN, INT64_MAX);
	return err;
}


static cbor_err_t cbor_decode_break(struct cbor_decoder *dec)
{
	struct cbor_type type;

	cbor_type_decode(dec, &type);
	if (type.major != CBOR_MAJOR_OTHER || type.val != 31) // TODO
		return CBOR_ERR_ITEM;

	return CBOR_ERR_OK;
}


static cbor_err_t cbor_block_begin(struct cbor_decoder *dec, enum cbor_major major_type, bool indef, uint64_t *len)
{
	struct block *block;
	cbor_err_t err;

	block = stack_push(&dec->blocks);
	if (!block)
		return CBOR_ERR_NOMEM;

	block->num_items = 0;
	if ((err = cbor_type_decode(dec, &block->type)) != CBOR_ERR_OK)
		return err;

	if (block->type.major != major_type)
		return CBOR_ERR_ITEM;

	if (block->type.indef != indef)
		return CBOR_ERR_INDEF;

	*len = block->type.val;
	return CBOR_ERR_OK;
}


static cbor_err_t cbor_block_end(struct cbor_decoder *dec, enum cbor_major major_type)
{
	struct block *block;

	if (stack_is_empty(&dec->blocks))
		return CBOR_ERR_OPER;

	block = stack_pop(&dec->blocks);

	if (block->type.major != major_type)
		return CBOR_ERR_OPER;

	if (block->type.indef)
		return cbor_decode_break(dec);

	if (block->num_items != block->type.val)
		return CBOR_ERR_OPER;
	
	return CBOR_ERR_OK;
}


cbor_err_t cbor_array_decode_begin(struct cbor_decoder *dec, uint64_t *len)
{
	return cbor_block_begin(dec, CBOR_MAJOR_ARRAY, false, len);
}


cbor_err_t cbor_array_decode_begin_indef(struct cbor_decoder *dec)
{
	uint64_t tmp;
	return cbor_block_begin(dec, CBOR_MAJOR_ARRAY, true, &tmp);
}


cbor_err_t cbor_array_decode_end(struct cbor_decoder *dec)
{
	return cbor_block_end(dec, CBOR_MAJOR_ARRAY);
}


cbor_err_t cbor_map_decode_begin(struct cbor_decoder *dec, uint64_t *len)
{
	return cbor_block_begin(dec, CBOR_MAJOR_MAP, false, len);
}


cbor_err_t cbor_map_decode_end(struct cbor_decoder *dec)
{
	return cbor_block_end(dec, CBOR_MAJOR_MAP);
}


static cbor_err_t cbor_string_decode_indef(struct cbor_decoder *dec, enum cbor_major major, unsigned char **bytes, size_t *len)
{
	struct cbor_type type;
	size_t num_read;
	cbor_err_t err;

	*bytes = NULL;
	num_read = 0;

	while ((err = cbor_type_decode(dec, &type)) == CBOR_ERR_OK) {
		if (type.major == CBOR_MAJOR_OTHER && type.val == 31)
			break;

		if (type.major != major || type.indef) {
			free(*bytes);
			return CBOR_ERR_ITEM;
		}

		*bytes = cbor_realloc(*bytes, num_read + type.val + 1);
		cbor_stream_read(dec->stream, *bytes, num_read, type.val);
		num_read += type.val;
	}

	*len = num_read;
	return err;
}


static cbor_err_t cbor_string_decode(struct cbor_decoder *dec, enum cbor_major major, unsigned char **bytes, size_t *len)
{
	struct cbor_type type;
	cbor_err_t err;

	if ((err = cbor_type_decode(dec, &type)) != CBOR_ERR_OK)
		return err;

	if (type.major != major)
		return CBOR_ERR_ITEM;

	if (!type.indef) {
		*bytes = cbor_malloc(type.val + 1);
		*len = type.val;
		return cbor_stream_read(dec->stream, *bytes, 0, type.val);
	}

	return cbor_string_decode_indef(dec, major, bytes, len);
}


cbor_err_t cbor_bytes_decode(struct cbor_decoder *dec, unsigned char **str, size_t *len)
{
	return cbor_string_decode(dec, CBOR_MAJOR_BYTES, str, len);
}


cbor_err_t cbor_text_decode(struct cbor_decoder *dec, unsigned char **str, size_t *len)
{
	cbor_err_t err;

	if ((err = cbor_string_decode(dec, CBOR_MAJOR_TEXT, str, len)) == CBOR_ERR_OK)
		(*str)[*len] = '\0';

	return err;
}


cbor_err_t cbor_tag_decode(struct cbor_decoder *dec, uint64_t *tagno)
{
	cbor_err_t err;
	struct cbor_type type;

	if ((err = cbor_type_decode(dec, &type)) != CBOR_ERR_OK)
		return err;

	if (type.major != CBOR_MAJOR_TAG)
		return CBOR_ERR_ITEM;

	*tagno = type.val;
	return CBOR_ERR_OK;
}
