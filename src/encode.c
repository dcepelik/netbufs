#include "internal.h"
#include "memory.h"
#include "stack.h"
#include "stream.h"

#include <assert.h>
#include <endian.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>


struct cbor_encoder *cbor_encoder_new(struct cbor_stream *stream)
{
	struct cbor_encoder *enc;

	enc = cbor_malloc(sizeof(*enc));
	enc->stream = stream;

	if (!stack_init(&enc->blocks, 4, sizeof(struct block))) {
		free(enc);
		return NULL;
	}

	return enc;
}


void cbor_encoder_delete(struct cbor_encoder *enc)
{
	cbor_free(enc);
}


static cbor_err_t cbor_type_encode(struct cbor_encoder *enc, struct cbor_type type)
{
	unsigned char bytes[9];
	size_t num_extra_bytes;
	uint64_t val_be;
	unsigned char *val_be_bytes = (unsigned char *)&val_be;
	size_t i;

	num_extra_bytes = 0;
	bytes[0] = type.major << 5;

	if (type.indef) {
		bytes[0] |= CBOR_EXTRA_VAR_LEN;
	}
	else {
		if (type.val <= 23) {
			bytes[0] |= type.val;
		}
		else if (type.val <= UINT8_MAX) {
			num_extra_bytes = 1;
			bytes[0] |= CBOR_EXTRA_1B;
		}
		else if (type.val <= UINT16_MAX) {
			num_extra_bytes = 2;
			bytes[0] |= CBOR_EXTRA_2B;
		}
		else if (type.val <= UINT32_MAX) {
			num_extra_bytes = 4;
			bytes[0] |= CBOR_EXTRA_4B;
		}
		else {
			num_extra_bytes = 8;
			bytes[0] |= CBOR_EXTRA_8B;
		}

		/* implicit corner case: won't copy anything when val <= 23 */
		val_be = htobe64(type.val);
		for (i = 0; i < num_extra_bytes; i++)
			bytes[1 + i] = val_be_bytes[(8 - num_extra_bytes) + i];
	}

	return cbor_stream_write(enc->stream, bytes, 1 + num_extra_bytes);
}


cbor_err_t cbor_uint_encode(struct cbor_encoder *enc, uint64_t val)
{
	return cbor_type_encode(enc, (struct cbor_type) {
		.major = CBOR_MAJOR_UINT,
		.indef = false,
		.val = val
	});
}


cbor_err_t cbor_uint8_encode(struct cbor_encoder *enc, uint8_t val)
{
	return cbor_uint_encode(enc, val);
}


cbor_err_t cbor_uint16_encode(struct cbor_encoder *enc, uint16_t val)
{
	return cbor_uint_encode(enc, val);
}


cbor_err_t cbor_uint32_encode(struct cbor_encoder *enc, uint32_t val)
{
	return cbor_uint_encode(enc, val);
}


cbor_err_t cbor_uint64_encode(struct cbor_encoder *enc, uint64_t val)
{
	return cbor_uint_encode(enc, val);
}


static cbor_err_t cbor_int_encode(struct cbor_encoder *enc, int64_t val)
{
	if (val >= 0)
		return cbor_uint_encode(enc, val);
	else
		return cbor_type_encode(enc, (struct cbor_type) {
			.major = CBOR_MAJOR_NEGATIVE_INT,
			.indef = false,
			.val = (-1 - val)
		});
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


static cbor_err_t cbor_break_encode(struct cbor_encoder *enc)
{
	return cbor_stream_write(enc->stream, (unsigned char[]) { CBOR_BREAK }, 1);
}


static cbor_err_t cbor_block_begin(struct cbor_encoder *enc, enum cbor_major major_type, bool indef, uint64_t len)
{
	struct block *block;

	block = stack_push(&enc->blocks);
	if (!block)
		return CBOR_ERR_NOMEM;

	block->type.major = major_type;
	block->type.indef = indef;
	block->type.val = len;
	block->num_items = 0;

	return cbor_type_encode(enc, block->type);
}


static cbor_err_t cbor_block_end(struct cbor_encoder *enc, enum cbor_major major_type)
{
	struct block *block;

	if (stack_is_empty(&enc->blocks))
		return CBOR_ERR_OPER;

	block = stack_pop(&enc->blocks);

	if (block->type.major != major_type)
		return CBOR_ERR_OPER;

	if (block->type.indef) {
		return cbor_break_encode(enc);
	}

	if (block->num_items != block->type.val)
		return CBOR_ERR_OPER;

	return CBOR_ERR_OK;
}


cbor_err_t cbor_array_encode_begin(struct cbor_encoder *enc, uint64_t len)
{
	return cbor_block_begin(enc, CBOR_MAJOR_ARRAY, false, len);
}


cbor_err_t cbor_array_encode_begin_indef(struct cbor_encoder *enc)
{
	return cbor_block_begin(enc, CBOR_MAJOR_ARRAY, true, 0);
}


cbor_err_t cbor_array_encode_end(struct cbor_encoder *enc)
{
	return cbor_block_end(enc, CBOR_MAJOR_ARRAY);
}


cbor_err_t cbor_map_encode_begin(struct cbor_encoder *enc, uint64_t len)
{
	return cbor_block_begin(enc, CBOR_MAJOR_MAP, false, len);
}


cbor_err_t cbor_map_encode_begin_indef(struct cbor_encoder *enc)
{
	return cbor_block_begin(enc, CBOR_MAJOR_MAP, true, 0);
}


cbor_err_t cbor_map_encode_end(struct cbor_encoder *enc)
{
	return cbor_block_end(enc, CBOR_MAJOR_MAP);
}


static cbor_err_t cbor_string_encode(struct cbor_encoder *enc, enum cbor_major major, unsigned char *bytes, size_t len)
{
	cbor_err_t err;

	err = cbor_type_encode(enc, (struct cbor_type) {
		.major = major,
		.indef = false,
		.val = len
	});

	if (err)
		return err;

	return cbor_stream_write(enc->stream, bytes, len);
}


cbor_err_t cbor_bytes_encode(struct cbor_encoder *enc, unsigned char *bytes, size_t len)
{
	return cbor_string_encode(enc, CBOR_MAJOR_BYTES, bytes, len);
}


cbor_err_t cbor_bytes_encode_begin_indef(struct cbor_encoder *enc)
{
	return cbor_block_begin(enc, CBOR_MAJOR_BYTES, true, 0);
}


cbor_err_t cbor_bytes_encode_end(struct cbor_encoder *enc)
{
	return cbor_block_end(enc, CBOR_MAJOR_BYTES);
}


cbor_err_t cbor_text_encode(struct cbor_encoder *enc, unsigned char *str, size_t len)
{
	return cbor_string_encode(enc, CBOR_MAJOR_TEXT, (unsigned char *)str, len);
}


cbor_err_t cbor_text_encode_begin_indef(struct cbor_encoder *enc)
{
	return cbor_block_begin(enc, CBOR_MAJOR_TEXT, true, 0);
}


cbor_err_t cbor_text_encode_end(struct cbor_encoder *enc)
{
	return cbor_block_end(enc, CBOR_MAJOR_TEXT);
}
