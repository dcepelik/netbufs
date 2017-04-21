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


static cbor_err_t error(struct cbor_stream *cs, cbor_err_t err, char *str, ...)
{
	assert(err == CBOR_ERR_OK);

	va_list args;

	cs->err = err;
	va_start(args, str);
	strbuf_vprintf_at(&cs->err_buf, 0, str, args);
	va_end(args);
	return err;
}


static cbor_err_t cbor_encode_type(struct cbor_stream *cs, struct cbor_hdr hdr)
{
	byte_t bytes[9];
	size_t num_extra_bytes;
	uint64_t val_be;
	byte_t *val_be_bytes = (byte_t *)&val_be;
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

	return buf_write(cs->buf, bytes, 1 + num_extra_bytes);
}


cbor_err_t cbor_encode_uint(struct cbor_stream *cs, uint64_t val)
{
	return cbor_encode_type(cs, (struct cbor_hdr) {
		.major = CBOR_MAJOR_UINT,
		.indef = false,
		.u64 = val
	});
}


cbor_err_t cbor_encode_uint8(struct cbor_stream *cs, uint8_t val)
{
	return cbor_encode_uint(cs, val);
}


cbor_err_t cbor_encode_uint16(struct cbor_stream *cs, uint16_t val)
{
	return cbor_encode_uint(cs, val);
}


cbor_err_t cbor_encode_uint32(struct cbor_stream *cs, uint32_t val)
{
	return cbor_encode_uint(cs, val);
}


cbor_err_t cbor_encode_uint64(struct cbor_stream *cs, uint64_t val)
{
	return cbor_encode_uint(cs, val);
}


static cbor_err_t cbor_encode_int(struct cbor_stream *cs, int64_t val)
{
	if (val >= 0)
		return cbor_encode_uint(cs, val);
	else
		return cbor_encode_type(cs, (struct cbor_hdr) {
			.major = CBOR_MAJOR_NEGATIVE_INT,
			.indef = false,
			.u64 = (-1 - val)
		});
}


cbor_err_t cbor_encode_int8(struct cbor_stream *cs, int8_t val)
{
	return cbor_encode_int(cs, val);
}


cbor_err_t cbor_encode_int16(struct cbor_stream *cs, int16_t val)
{
	return cbor_encode_int(cs, val);
}


cbor_err_t cbor_encode_int32(struct cbor_stream *cs, int32_t val)
{
	return cbor_encode_int(cs, val);
}


cbor_err_t cbor_encode_int64(struct cbor_stream *cs, int64_t val)
{
	return cbor_encode_int(cs, val);
}


static cbor_err_t cbor_encode_break(struct cbor_stream *cs)
{
	return buf_write(cs->buf, (byte_t[]) { CBOR_BREAK }, 1);
}


static cbor_err_t cbor_block_begin(struct cbor_stream *cs, enum cbor_major major_type, bool indef, uint64_t len)
{
	struct block *block;

	block = stack_push(&cs->blocks);
	if (!block)
		return error(cs, CBOR_ERR_NOMEM, "No memory to allocate new nesting block.");

	block->hdr.major = major_type;
	block->hdr.indef = indef;
	block->hdr.u64 = len;
	block->num_items = 0;

	return cbor_encode_type(cs, block->hdr);
}


static cbor_err_t cbor_block_end(struct cbor_stream *cs, enum cbor_major major_type)
{
	struct block *block;

	if (stack_is_empty(&cs->blocks))
		return error(cs, CBOR_ERR_OPER, "Cannot end block, no block is open.");

	block = stack_pop(&cs->blocks);

	if (block->hdr.major != major_type)
		/* TODO msg */
		return error(cs, CBOR_ERR_OPER, "Attempting to close block of ..., but ... is open.");

	if (block->hdr.indef)
		return cbor_encode_break(cs);

	//if (block->num_items != block->hdr.u64) {
	//	return error(cs, CBOR_ERR_OPER, NULL);
	//}

	return error(cs, CBOR_ERR_OK, NULL);
}


cbor_err_t cbor_encode_array_begin(struct cbor_stream *cs, uint64_t len)
{
	return cbor_block_begin(cs, CBOR_MAJOR_ARRAY, false, len);
}


cbor_err_t cbor_encode_array_begin_indef(struct cbor_stream *cs)
{
	return cbor_block_begin(cs, CBOR_MAJOR_ARRAY, true, 0);
}


cbor_err_t cbor_encode_array_end(struct cbor_stream *cs)
{
	return cbor_block_end(cs, CBOR_MAJOR_ARRAY);
}


cbor_err_t cbor_encode_map_begin(struct cbor_stream *cs, uint64_t len)
{
	return cbor_block_begin(cs, CBOR_MAJOR_MAP, false, len);
}


cbor_err_t cbor_encode_map_begin_indef(struct cbor_stream *cs)
{
	return cbor_block_begin(cs, CBOR_MAJOR_MAP, true, 0);
}


cbor_err_t cbor_encode_map_end(struct cbor_stream *cs)
{
	return cbor_block_end(cs, CBOR_MAJOR_MAP);
}


static cbor_err_t cbor_encode_string(struct cbor_stream *cs, enum cbor_major major, byte_t *bytes, size_t len)
{
	cbor_err_t err;

	err = cbor_encode_type(cs, (struct cbor_hdr) {
		.major = major,
		.indef = false,
		.u64 = len
	});

	if (err)
		return err;

	return buf_write(cs->buf, bytes, len);
}


cbor_err_t cbor_encode_bytes(struct cbor_stream *cs, byte_t *bytes, size_t len)
{
	return cbor_encode_string(cs, CBOR_MAJOR_BYTES, bytes, len);
}


cbor_err_t cbor_encode_bytes_begin_indef(struct cbor_stream *cs)
{
	return cbor_block_begin(cs, CBOR_MAJOR_BYTES, true, 0);
}


cbor_err_t cbor_encode_bytes_end(struct cbor_stream *cs)
{
	return cbor_block_end(cs, CBOR_MAJOR_BYTES);
}


cbor_err_t cbor_encode_text(struct cbor_stream *cs, byte_t *str, size_t len)
{
	return cbor_encode_string(cs, CBOR_MAJOR_TEXT, str, len);
}


cbor_err_t cbor_encode_text_begin_indef(struct cbor_stream *cs)
{
	return cbor_block_begin(cs, CBOR_MAJOR_TEXT, true, 0);
}


cbor_err_t cbor_encode_text_end(struct cbor_stream *cs)
{
	return cbor_block_end(cs, CBOR_MAJOR_TEXT);
}


cbor_err_t cbor_encode_tag(struct cbor_stream *cs, uint64_t tagno)
{
	return cbor_encode_type(cs, (struct cbor_hdr) {
		.major = CBOR_MAJOR_TAG,
		.indef = false,
		.u64 = tagno
	});
}


cbor_err_t cbor_encode_sval(struct cbor_stream *cs, enum cbor_sval sval)
{
	if (sval > 255)
		return error(cs, CBOR_ERR_RANGE, "Simple value %u is greater than 255.", sval);

	return cbor_encode_type(cs, (struct cbor_hdr) {
		.major = CBOR_MAJOR_OTHER,
		.minor = CBOR_MINOR_SVAL,
		.indef = false,
		.u64 = sval
	});
}


cbor_err_t cbor_encode_array(struct cbor_stream *cs, struct cbor_item *array);
cbor_err_t cbor_encode_map(struct cbor_stream *cs, struct cbor_item *map);


cbor_err_t cbor_encode_item(struct cbor_stream *cs, struct cbor_item *item)
{
	switch (item->hdr.major)
	{
	case CBOR_MAJOR_UINT:
		return cbor_encode_uint64(cs, item->hdr.u64);

	case CBOR_MAJOR_NEGATIVE_INT:
		return cbor_encode_int64(cs, item->i64);

	case CBOR_MAJOR_BYTES:
		return cbor_encode_bytes(cs, item->bytes, item->len);

	case CBOR_MAJOR_TEXT:
		return cbor_encode_text(cs, (byte_t *)item->str, item->len);

	case CBOR_MAJOR_ARRAY:
		return cbor_encode_array(cs, item);

	case CBOR_MAJOR_MAP:
		return cbor_encode_map(cs, item);

	case CBOR_MAJOR_TAG:
		return cbor_encode_tag(cs, item->hdr.u64);

	default:
		return error(cs, CBOR_ERR_UNSUP, NULL);
	}
}


static inline cbor_err_t encode_array_items(struct cbor_stream *cs, struct cbor_item *items, size_t len)
{
	size_t i;
	cbor_err_t err;

	for (i = 0; i < len; i++)
		if ((err = cbor_encode_item(cs, &items[i])) != CBOR_ERR_OK)
			return err;

	return CBOR_ERR_OK;
}


cbor_err_t cbor_encode_array(struct cbor_stream *cs, struct cbor_item *array)
{
	size_t i;
	cbor_err_t err;

	if (array->hdr.indef)
		err = cbor_encode_array_begin_indef(cs);
	else
		err = cbor_encode_array_begin(cs, array->len);

	if (err != CBOR_ERR_OK)
		return err;

	if ((err = encode_array_items(cs, array->items, array->len)) != CBOR_ERR_OK)
		return err;

	if ((err = cbor_encode_array_end(cs)) != CBOR_ERR_OK)
		return err;

	return CBOR_ERR_OK;
}


cbor_err_t cbor_encode_map(struct cbor_stream *cs, struct cbor_item *map)
{
	size_t i;
	cbor_err_t err;

	if (map->hdr.indef)
		err = cbor_encode_map_begin_indef(cs);
	else
		err = cbor_encode_map_begin(cs, map->len);

	if (err != CBOR_ERR_OK)
		return err;

	if ((err = encode_array_items(cs, (struct cbor_item *)map->pairs, 2 * map->len)) != CBOR_ERR_OK)
		return err;

	if ((err = cbor_encode_map_end(cs)) != CBOR_ERR_OK)
		return err;

	return CBOR_ERR_OK;
}
