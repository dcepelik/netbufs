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


static cbor_err_t write_hdr(struct cbor_stream *cs, enum major major, byte_t lbits)
{
	byte_t hdr;
	struct block *block;

	hdr = (major << 5) + lbits;
	return buf_write(cs->buf, &hdr, 1);
}


static cbor_err_t write_hdr_count(struct cbor_stream *cs, enum major major, byte_t lbits)
{
	top_block(cs)->num_items++;
	return write_hdr(cs, major, lbits);
}


static cbor_err_t write_hdr_indef(struct cbor_stream *cs, enum major major)
{
	return write_hdr_count(cs, major, LBITS_INDEFINITE);
}


static cbor_err_t write_hdr_major7(struct cbor_stream *cs, enum minor minor)
{
	return write_hdr_count(cs, CBOR_MAJOR_7, minor);
}


static cbor_err_t write_break(struct cbor_stream *cs)
{
	return write_hdr(cs, CBOR_MAJOR_7, CBOR_MINOR_BREAK);
}


static cbor_err_t write_hdr_u64(struct cbor_stream *cs, enum major major, uint64_t u64)
{
	byte_t lbits;
	byte_t bytes[8];
	size_t len;
	uint64_t u64be;
	byte_t *u64be_ptr = (byte_t *)&u64be;
	cbor_err_t err;
	size_t i;

	assert(major >= 0 && major < 7);

	if (u64 <= 23) {
		return write_hdr_count(cs, major, (byte_t)u64);
	}

	if (u64 <= UINT8_MAX) {
		len = 1;
		lbits = LBITS_1B;
	}
	else if (u64 <= UINT16_MAX) {
		len = 2;
		lbits = LBITS_2B;
	}
	else if (u64 <= UINT32_MAX) {
		len = 4;
		lbits = LBITS_4B;
	}
	else {
		len = 8;
		lbits = LBITS_8B;
	}

	if ((err = write_hdr_count(cs, major, lbits)) != CBOR_ERR_OK)
		return err;

	u64be = htobe64(u64);
	for (i = 0; i < len; i++)
		bytes[i] = u64be_ptr[(8 - len) + i];

	return buf_write(cs->buf, bytes, len);
}


cbor_err_t cbor_encode_uint(struct cbor_stream *cs, uint64_t val)
{
	return write_hdr_u64(cs, CBOR_MAJOR_UINT, val);
}


cbor_err_t cbor_encode_uint8(struct cbor_stream *cs, uint8_t val)
{
	return write_hdr_u64(cs, CBOR_MAJOR_UINT, val);
}


cbor_err_t cbor_encode_uint16(struct cbor_stream *cs, uint16_t val)
{
	return write_hdr_u64(cs, CBOR_MAJOR_UINT, val);
}


cbor_err_t cbor_encode_uint32(struct cbor_stream *cs, uint32_t val)
{
	return write_hdr_u64(cs, CBOR_MAJOR_UINT, val);
}


cbor_err_t cbor_encode_uint64(struct cbor_stream *cs, uint64_t val)
{
	return write_hdr_u64(cs, CBOR_MAJOR_UINT, val);
}


static uint64_t i64_to_u64(int64_t i64)
{
	return -(i64 + 1);
}


static cbor_err_t cbor_encode_int(struct cbor_stream *cs, int64_t val)
{
	if (val >= 0)
		return write_hdr_u64(cs, CBOR_MAJOR_UINT, val);
	else
		return write_hdr_u64(cs, CBOR_MAJOR_NEGINT, i64_to_u64(val));
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


static cbor_err_t start_block(struct cbor_stream *cs, enum major major, uint64_t len)
{
	cbor_err_t err;
	if ((err = write_hdr_u64(cs, major, len)) == CBOR_ERR_OK)
		return push_block(cs, major, false, len);
	return err;
}


static cbor_err_t start_block_indef(struct cbor_stream *cs, enum major major)
{
	cbor_err_t err;
	if ((err = write_hdr_indef(cs, major)) == CBOR_ERR_OK)
		return push_block(cs, major, true, 0);
	return err;
}


static cbor_err_t end_block(struct cbor_stream *cs, enum major major_type)
{
	struct block *block;

	if (top_block(cs)->major == -1)
		return error(cs, CBOR_ERR_OPER, "Cannot end block, no block is open.");

	block = stack_pop(&cs->blocks);
	if (block->major != major_type)
		/* TODO msg */
		return error(cs, CBOR_ERR_OPER, "Attempting to close %s "
			"block when %s is open.", cbor_type_string(major_type),
			cbor_type_string(block->major));

	if (block->indefinite)
		return write_break(cs);

	//if (block->num_items != block->hdr.u64) {
	//	return error(cs, CBOR_ERR_NITEMS, NULL);
	//}

	return CBOR_ERR_OK;
}


cbor_err_t cbor_encode_array_begin(struct cbor_stream *cs, uint64_t len)
{
	return start_block(cs, CBOR_MAJOR_ARRAY, len);
}


cbor_err_t cbor_encode_array_begin_indef(struct cbor_stream *cs)
{
	return start_block_indef(cs, CBOR_MAJOR_ARRAY);
}


cbor_err_t cbor_encode_array_end(struct cbor_stream *cs)
{
	return end_block(cs, CBOR_MAJOR_ARRAY);
}


cbor_err_t cbor_encode_map_begin(struct cbor_stream *cs, uint64_t len)
{
	return start_block(cs, CBOR_MAJOR_MAP, len);
}


cbor_err_t cbor_encode_map_begin_indef(struct cbor_stream *cs)
{
	return start_block_indef(cs, CBOR_MAJOR_MAP);
}


cbor_err_t cbor_encode_map_end(struct cbor_stream *cs)
{
	if (top_block(cs)->num_items % 2 != 0) {
		DEBUG_EXPR("%lu", top_block(cs)->num_items);
		return error(cs, CBOR_ERR_NITEMS, "Odd number of items in a map.");
	}

	return end_block(cs, CBOR_MAJOR_MAP);
}


static cbor_err_t encode_bytes(struct cbor_stream *cs, enum major major, byte_t *bytes, size_t len)
{
	cbor_err_t err;
	if ((err = write_hdr_u64(cs, major, len)) == CBOR_ERR_OK)
		return buf_write(cs->buf, bytes, len);
	return err;
}


cbor_err_t cbor_encode_bytes(struct cbor_stream *cs, byte_t *bytes, size_t len)
{
	return encode_bytes(cs, CBOR_MAJOR_BYTES, bytes, len);
}


cbor_err_t cbor_encode_bytes_begin_indef(struct cbor_stream *cs)
{
	return start_block_indef(cs, CBOR_MAJOR_BYTES);
}


cbor_err_t cbor_encode_bytes_end(struct cbor_stream *cs)
{
	return end_block(cs, CBOR_MAJOR_BYTES);
}


cbor_err_t cbor_encode_text(struct cbor_stream *cs, byte_t *str, size_t len)
{
	return encode_bytes(cs, CBOR_MAJOR_TEXT, str, len);
}


cbor_err_t cbor_encode_text_begin_indef(struct cbor_stream *cs)
{
	return start_block_indef(cs, CBOR_MAJOR_TEXT);
}


cbor_err_t cbor_encode_text_end(struct cbor_stream *cs)
{
	return end_block(cs, CBOR_MAJOR_TEXT);
}


cbor_err_t cbor_encode_tag(struct cbor_stream *cs, uint64_t tag)
{
	return write_hdr_u64(cs, CBOR_MAJOR_TAG, tag);
}


cbor_err_t cbor_encode_sval(struct cbor_stream *cs, enum cbor_sval sval)
{
	cbor_err_t err;

	if (sval >= 0 && sval <= 255) {
		if (sval <= 23)
			return write_hdr_major7(cs, (byte_t)sval);
		
		if ((err = write_hdr_major7(cs, CBOR_MINOR_SVAL)) == CBOR_ERR_OK)
			return buf_write(cs->buf, (byte_t *)&sval, 1);
		return err;
	}

	return error(cs, CBOR_ERR_RANGE, "Simple value %u must be non-negative"
		"and less than or equal to 255, %i was given.", sval);
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


/* TODO Don't encode undef arrays */
cbor_err_t encode_array(struct cbor_stream *cs, struct cbor_item *array)
{
	size_t i;
	cbor_err_t err;

	if (array->indefinite)
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


/* TODO Don't encode undef maps */
cbor_err_t encode_map(struct cbor_stream *cs, struct cbor_item *map)
{
	size_t i;
	cbor_err_t err;

	if (map->indefinite)
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


cbor_err_t cbor_encode_item(struct cbor_stream *cs, struct cbor_item *item)
{
	switch (item->type)
	{
	case CBOR_TYPE_UINT:
		return cbor_encode_uint64(cs, item->u64);
	case CBOR_TYPE_INT:
		return cbor_encode_int64(cs, item->i64);
	case CBOR_TYPE_BYTES:
		return cbor_encode_bytes(cs, item->bytes, item->len);
	case CBOR_TYPE_TEXT:
		return cbor_encode_text(cs, (byte_t *)item->str, item->len);
	case CBOR_TYPE_ARRAY:
		return encode_array(cs, item);
	case CBOR_TYPE_MAP:
		return encode_map(cs, item);
	case CBOR_TYPE_TAG:
		return cbor_encode_tag(cs, item->u64);
	case CBOR_TYPE_SVAL:
		return cbor_encode_sval(cs, item->sval);

	default:
		return error(cs, CBOR_ERR_UNSUP, NULL);
	}
}
