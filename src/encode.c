#include "debug.h"
#include "internal.h"
#include "memory.h"
#include "stack.h"

#include <assert.h>
#include <endian.h>
#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>


struct cbor_encoder *cbor_encoder_new(struct cbor_stream *stream)
{
	struct cbor_encoder *enc;

	enc = cbor_malloc(sizeof(*enc));
	enc->stream = stream;

	strbuf_init(&enc->err_buf, 24);

	if (!stack_init(&enc->blocks, 4, sizeof(struct block))) {
		free(enc);
		return NULL;
	}

	return enc;
}


void cbor_encoder_delete(struct cbor_encoder *enc)
{
	strbuf_free(&enc->err_buf);
	cbor_free(enc);
}


static cbor_err_t error(struct cbor_encoder *enc, cbor_err_t err, char *str, ...)
{
	assert(err == CBOR_ERR_OK);

	va_list args;

	enc->err = err;
	va_start(args, str);
	strbuf_vprintf_at(&enc->err_buf, 0, str, args);
	va_end(args);
	return err;
}


static cbor_err_t cbor_encode_type(struct cbor_encoder *enc, struct cbor_hdr hdr)
{
	unsigned char bytes[9];
	size_t num_extra_bytes;
	uint64_t val_be;
	unsigned char *val_be_bytes = (unsigned char *)&val_be;
	struct block *block;
	size_t i;

	num_extra_bytes = 0;
	bytes[0] = hdr.major << 5;

	if (hdr.indef) {
		bytes[0] |= CBOR_EXTRA_VAR_LEN;
	}
	else {
		if (hdr.u64 <= 23) {
			bytes[0] |= hdr.u64;
		}
		else if (hdr.u64 <= UINT8_MAX) {
			num_extra_bytes = 1;
			bytes[0] |= CBOR_EXTRA_1B;
		}
		else if (hdr.u64 <= UINT16_MAX) {
			num_extra_bytes = 2;
			bytes[0] |= CBOR_EXTRA_2B;
		}
		else if (hdr.u64 <= UINT32_MAX) {
			num_extra_bytes = 4;
			bytes[0] |= CBOR_EXTRA_4B;
		}
		else {
			num_extra_bytes = 8;
			bytes[0] |= CBOR_EXTRA_8B;
		}

		/* implicit corner case: won't copy anything when val <= 23 */
		val_be = htobe64(hdr.u64);
		for (i = 0; i < num_extra_bytes; i++)
			bytes[1 + i] = val_be_bytes[(8 - num_extra_bytes) + i];
	}

	return cbor_stream_write(enc->stream, bytes, 1 + num_extra_bytes);
}


cbor_err_t cbor_encode_uint(struct cbor_encoder *enc, uint64_t val)
{
	return cbor_encode_type(enc, (struct cbor_hdr) {
		.major = CBOR_MAJOR_UINT,
		.indef = false,
		.u64 = val
	});
}


cbor_err_t cbor_encode_uint8(struct cbor_encoder *enc, uint8_t val)
{
	return cbor_encode_uint(enc, val);
}


cbor_err_t cbor_encode_uint16(struct cbor_encoder *enc, uint16_t val)
{
	return cbor_encode_uint(enc, val);
}


cbor_err_t cbor_encode_uint32(struct cbor_encoder *enc, uint32_t val)
{
	return cbor_encode_uint(enc, val);
}


cbor_err_t cbor_encode_uint64(struct cbor_encoder *enc, uint64_t val)
{
	return cbor_encode_uint(enc, val);
}


static cbor_err_t cbor_encode_int(struct cbor_encoder *enc, int64_t val)
{
	if (val >= 0)
		return cbor_encode_uint(enc, val);
	else
		return cbor_encode_type(enc, (struct cbor_hdr) {
			.major = CBOR_MAJOR_NEGATIVE_INT,
			.indef = false,
			.u64 = (-1 - val)
		});
}


cbor_err_t cbor_encode_int8(struct cbor_encoder *enc, int8_t val)
{
	return cbor_encode_int(enc, val);
}


cbor_err_t cbor_encode_int16(struct cbor_encoder *enc, int16_t val)
{
	return cbor_encode_int(enc, val);
}


cbor_err_t cbor_encode_int32(struct cbor_encoder *enc, int32_t val)
{
	return cbor_encode_int(enc, val);
}


cbor_err_t cbor_encode_int64(struct cbor_encoder *enc, int64_t val)
{
	return cbor_encode_int(enc, val);
}


static cbor_err_t cbor_encode_break(struct cbor_encoder *enc)
{
	return cbor_stream_write(enc->stream, (unsigned char[]) { CBOR_BREAK }, 1);
}


static cbor_err_t cbor_block_begin(struct cbor_encoder *enc, enum cbor_major major_type, bool indef, uint64_t len)
{
	struct block *block;

	block = stack_push(&enc->blocks);
	if (!block)
		return error(enc, CBOR_ERR_NOMEM, "No memory to allocate new nesting block.");

	block->hdr.major = major_type;
	block->hdr.indef = indef;
	block->hdr.u64 = len;
	block->num_items = 0;

	return cbor_encode_type(enc, block->hdr);
}


static cbor_err_t cbor_block_end(struct cbor_encoder *enc, enum cbor_major major_type)
{
	struct block *block;

	if (stack_is_empty(&enc->blocks))
		return error(enc, CBOR_ERR_OPER, "Cannot end block, no block is open.");

	block = stack_pop(&enc->blocks);

	if (block->hdr.major != major_type)
		/* TODO msg */
		return error(enc, CBOR_ERR_OPER, "Attempting to close block of ..., but ... is open.");

	if (block->hdr.indef)
		return cbor_encode_break(enc);

	//if (block->num_items != block->hdr.u64) {
	//	return error(enc, CBOR_ERR_OPER, NULL);
	//}

	return error(enc, CBOR_ERR_OK, NULL);
}


cbor_err_t cbor_encode_array_begin(struct cbor_encoder *enc, uint64_t len)
{
	return cbor_block_begin(enc, CBOR_MAJOR_ARRAY, false, len);
}


cbor_err_t cbor_encode_array_begin_indef(struct cbor_encoder *enc)
{
	return cbor_block_begin(enc, CBOR_MAJOR_ARRAY, true, 0);
}


cbor_err_t cbor_encode_array_end(struct cbor_encoder *enc)
{
	return cbor_block_end(enc, CBOR_MAJOR_ARRAY);
}


cbor_err_t cbor_encode_map_begin(struct cbor_encoder *enc, uint64_t len)
{
	return cbor_block_begin(enc, CBOR_MAJOR_MAP, false, len);
}


cbor_err_t cbor_encode_map_begin_indef(struct cbor_encoder *enc)
{
	return cbor_block_begin(enc, CBOR_MAJOR_MAP, true, 0);
}


cbor_err_t cbor_encode_map_end(struct cbor_encoder *enc)
{
	return cbor_block_end(enc, CBOR_MAJOR_MAP);
}


static cbor_err_t cbor_encode_string(struct cbor_encoder *enc, enum cbor_major major, unsigned char *bytes, size_t len)
{
	cbor_err_t err;

	err = cbor_encode_type(enc, (struct cbor_hdr) {
		.major = major,
		.indef = false,
		.u64 = len
	});

	if (err)
		return err;

	return cbor_stream_write(enc->stream, bytes, len);
}


cbor_err_t cbor_encode_bytes(struct cbor_encoder *enc, unsigned char *bytes, size_t len)
{
	return cbor_encode_string(enc, CBOR_MAJOR_BYTES, bytes, len);
}


cbor_err_t cbor_encode_bytes_begin_indef(struct cbor_encoder *enc)
{
	return cbor_block_begin(enc, CBOR_MAJOR_BYTES, true, 0);
}


cbor_err_t cbor_encode_bytes_end(struct cbor_encoder *enc)
{
	return cbor_block_end(enc, CBOR_MAJOR_BYTES);
}


cbor_err_t cbor_encode_text(struct cbor_encoder *enc, unsigned char *str, size_t len)
{
	return cbor_encode_string(enc, CBOR_MAJOR_TEXT, str, len);
}


cbor_err_t cbor_encode_text_begin_indef(struct cbor_encoder *enc)
{
	return cbor_block_begin(enc, CBOR_MAJOR_TEXT, true, 0);
}


cbor_err_t cbor_encode_text_end(struct cbor_encoder *enc)
{
	return cbor_block_end(enc, CBOR_MAJOR_TEXT);
}


cbor_err_t cbor_encode_tag(struct cbor_encoder *enc, uint64_t tagno)
{
	return cbor_encode_type(enc, (struct cbor_hdr) {
		.major = CBOR_MAJOR_TAG,
		.indef = false,
		.u64 = tagno
	});
}


cbor_err_t cbor_encode_sval(struct cbor_encoder *enc, enum cbor_sval sval)
{
	if (sval > 255)
		return error(enc, CBOR_ERR_RANGE, "Simple value %u is greater than 255.", sval);

	return cbor_encode_type(enc, (struct cbor_hdr) {
		.major = CBOR_MAJOR_OTHER,
		.minor = CBOR_MINOR_SVAL,
		.indef = false,
		.u64 = sval
	});
}


cbor_err_t cbor_encode_array(struct cbor_encoder *enc, struct cbor_item *array);
cbor_err_t cbor_encode_map(struct cbor_encoder *enc, struct cbor_item *map);


cbor_err_t cbor_encode_item(struct cbor_encoder *enc, struct cbor_item *item)
{
	switch (item->hdr.major)
	{
	case CBOR_MAJOR_UINT:
		return cbor_encode_uint64(enc, item->hdr.u64);

	case CBOR_MAJOR_NEGATIVE_INT:
		return cbor_encode_int64(enc, item->i64);

	case CBOR_MAJOR_BYTES:
		return cbor_encode_bytes(enc, item->bytes, item->len);

	case CBOR_MAJOR_TEXT:
		return cbor_encode_text(enc, (unsigned char *)item->str, item->len);

	case CBOR_MAJOR_ARRAY:
		return cbor_encode_array(enc, item);

	case CBOR_MAJOR_MAP:
		return cbor_encode_map(enc, item);

	case CBOR_MAJOR_TAG:
		return cbor_encode_tag(enc, item->hdr.u64);

	default:
		return error(enc, CBOR_ERR_UNSUP, NULL);
	}
}


static inline cbor_err_t encode_array_items(struct cbor_encoder *enc, struct cbor_item *items, size_t len)
{
	size_t i;
	cbor_err_t err;

	for (i = 0; i < len; i++)
		if ((err = cbor_encode_item(enc, &items[i])) != CBOR_ERR_OK)
			return err;

	return CBOR_ERR_OK;
}


cbor_err_t cbor_encode_array(struct cbor_encoder *enc, struct cbor_item *array)
{
	size_t i;
	cbor_err_t err;

	if (array->hdr.indef)
		err = cbor_encode_array_begin_indef(enc);
	else
		err = cbor_encode_array_begin(enc, array->len);

	if (err != CBOR_ERR_OK)
		return err;

	if ((err = encode_array_items(enc, array->items, array->len)) != CBOR_ERR_OK)
		return err;

	if ((err = cbor_encode_array_end(enc)) != CBOR_ERR_OK)
		return err;

	return CBOR_ERR_OK;
}


cbor_err_t cbor_encode_map(struct cbor_encoder *enc, struct cbor_item *map)
{
	size_t i;
	cbor_err_t err;

	if (map->hdr.indef)
		err = cbor_encode_map_begin_indef(enc);
	else
		err = cbor_encode_map_begin(enc, map->len);

	if (err != CBOR_ERR_OK)
		return err;

	if ((err = encode_array_items(enc, (struct cbor_item *)map->pairs, 2 * map->len)) != CBOR_ERR_OK)
		return err;

	if ((err = cbor_encode_map_end(enc)) != CBOR_ERR_OK)
		return err;

	return CBOR_ERR_OK;
}
