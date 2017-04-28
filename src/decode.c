/*
 * decode:
 * CBOR decoder
 */

#include "debug.h"
#include "internal.h"
#include "memory.h"

#include <assert.h>
#include <endian.h>
#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>


#define CBOR_ARRAY_INIT_SIZE	8
#define CBOR_BSTACK_INIT_SIZE	4
#define INT64_MIN_ABS		(-(INT64_MIN + 1))


static inline uint8_t lbits_to_nbytes(enum lbits lbits)
{
	assert(lbits >= LBITS_1B && lbits <= LBITS_8B);

	switch (lbits) {
	case LBITS_1B:
		return 1;
	case LBITS_2B:
		return 2;
	case LBITS_4B:
		return 4;
	case LBITS_8B:
		return 8;
	default:
		return 0;
	}
}


static inline cbor_err_t decode_u64(struct cbor_stream *cs, enum lbits lbits, uint64_t *u64)
{
	byte_t bytes[8];
	uint8_t nbytes;
	uint64_t u64be = 0;
	byte_t *u64be_ptr = (byte_t *)&u64be;
	size_t i;
	cbor_err_t err;

	if (lbits <= 23) {
		*u64 = lbits;
		return CBOR_ERR_OK;
	}

	if (lbits > LBITS_8B)
		return error(cs, CBOR_ERR_PARSE, "Invalid value of "
			"Additional Information: 0x%02X.", lbits);

	nbytes = lbits_to_nbytes(lbits);
	if ((err = buf_read(cs->buf, bytes, nbytes)) != CBOR_ERR_OK)
		return err;

	for (i = 0; i < nbytes; i++)
		u64be_ptr[(8 - nbytes) + i] = bytes[i];

	*u64 = be64toh(u64be);
	return CBOR_ERR_OK;
}


/* TODO have a look at the range checks and make sure nothing overflows */
static inline cbor_err_t u64_to_i64(struct cbor_stream *cs, uint64_t u64, int64_t *i64)
{
	if (u64 <= INT64_MIN_ABS) {
		*i64 = -u64 - 1;
		return CBOR_ERR_OK;
	}

	return error(cs, CBOR_ERR_RANGE,
		"Negative integer -%lu less than -%lu cannot be decoded.", u64, INT64_MIN_ABS);
}


static cbor_err_t decode_item_major7(struct cbor_stream *cs, struct cbor_item *item, enum minor minor)
{
	uint64_t u64;

	assert(minor >= 0);

	if (minor <= 23) {
		item->type = CBOR_TYPE_SVAL;
		item->sval = minor;
		item->u64 = item->sval;
		return CBOR_ERR_OK;
	}

	if (minor == CBOR_MINOR_BREAK)
		return CBOR_ERR_BREAK;

	/* leverage decode_u64 to read the appropriate amount of data */
	decode_u64(cs, (enum lbits)minor, &u64);

	switch (minor) {
	case CBOR_MINOR_SVAL:
		item->type = CBOR_TYPE_SVAL;
		item->sval = u64;
		item->u64 = item->sval; /* TODO */
		return CBOR_ERR_OK;
	case CBOR_MINOR_FLOAT16:
	case CBOR_MINOR_FLOAT32:
	case CBOR_MINOR_FLOAT64:
		return error(cs, CBOR_ERR_UNSUP, "Float decoding not supported.");
	default:
		break;
	}

	return error(cs, CBOR_ERR_PARSE, "Invalid Additional Information "
		"value for Major Type 7: 0x%02X.", minor);
}


static cbor_err_t predecode(struct cbor_stream *cs, struct cbor_item *item)
{
	byte_t hdr;
	enum major major;
	byte_t lbits;
	bool indefinite = false;
	uint64_t u64;
	cbor_err_t err;

	struct block *block;

	if ((err = buf_read(cs->buf, &hdr, 1)) != CBOR_ERR_OK)
		return err;

	major = (hdr & 0xE0) >> 5;
	assert(major >= 0 && major <= 7);

	lbits = hdr & 0x1F;

	if (major == CBOR_MAJOR_7)
		return decode_item_major7(cs, item, lbits);

	item->type = (enum cbor_type)major;

	if (lbits == LBITS_INDEFINITE) {
		if (!major_allows_indefinite(major))
			return error(cs, CBOR_ERR_INDEF, "Indefinite-length "
				"encoding is not allowed for %s items.",
				cbor_type_string(major));
		indefinite = true;
	}
	item->indefinite = indefinite;

	if (!indefinite) {
		if ((err = decode_u64(cs, lbits, &u64)) != CBOR_ERR_OK)
			return err;
	}

	if (indefinite || major == CBOR_MAJOR_ARRAY || major == CBOR_MAJOR_MAP) {
		if ((err = push_block(cs, major, indefinite, u64)) != CBOR_ERR_OK)
			return err;

		if (indefinite)
			return CBOR_ERR_OK;
	}

	item->u64 = u64;

	//switch (item->type)
	//{
	//case CBOR_TYPE_UINT:
	//	item->u64 = u64;
	//	break;

	//case CBOR_TYPE_INT:
	//	return u64_to_i64(cs, u64, &item->i64);

	//case CBOR_TYPE_BYTES:
	//case CBOR_TYPE_TEXT:
	//case CBOR_TYPE_ARRAY:
	//case CBOR_TYPE_MAP:
	//	item->len = u64;
	//	break;
	//
	//case CBOR_TYPE_TAG:
	//	item->tag = u64;
	//	break;

	//default:
	//	return error(cs, CBOR_ERR_PARSE, "Invalid major type.");
	//}

	if (item->type == CBOR_TYPE_INT) {
		return u64_to_i64(cs, u64, &item->i64);
	}

	return CBOR_ERR_OK;
}


static inline cbor_err_t predecode_check(struct cbor_stream *cs, struct cbor_item *item, enum cbor_type type)
{
	cbor_err_t err;

	if ((err = predecode(cs, item)) != CBOR_ERR_OK)
		return err;

	if (item->type != type)
		/* TODO msg */
		return error(cs, CBOR_ERR_ITEM, "%s was unexpected, %s was expected.",
			cbor_type_string(item->type), cbor_type_string(type));

	return CBOR_ERR_OK;
}


static uint64_t decode_uint(struct cbor_stream *cs, cbor_err_t *err, uint64_t max)
{
	struct cbor_item item;

	if ((*err = predecode(cs, &item)) != CBOR_ERR_OK)
		return 0;

	if (item.u64 > max) {
		*err = CBOR_ERR_RANGE;
		return 0;
	}

	*err = CBOR_ERR_OK;
	return item.u64;
}


/* TODO have a look at the range checks and make sure nothing overflows */
static int64_t decode_int(struct cbor_stream *cs, cbor_err_t *err, int64_t min, int64_t max)
{
	struct cbor_item item;
	int64_t i64;

	if ((*err = predecode(cs, &item)) != CBOR_ERR_OK)
		return 0;

	if (item.type == CBOR_TYPE_INT) {
		i64 = item.i64;
	}
	else if (item.type == CBOR_TYPE_UINT) {
		if (item.u64 > INT64_MAX) {
			*err = CBOR_ERR_RANGE;
			return 0;
		}

		i64 = (int64_t)item.u64;
	}
	else {
		*err = error(cs, CBOR_ERR_ITEM, "Unsigned or signed integer expected");
		return 0;
	}

	if (i64 >= min && i64 <= max)
		return i64;

	*err = CBOR_ERR_RANGE;
	return 0;
}


cbor_err_t cbor_decode_uint8(struct cbor_stream *cs, uint8_t *val)
{
	cbor_err_t err;
	*val = (uint8_t)decode_uint(cs, &err, UINT8_MAX);
	return err;
}


cbor_err_t cbor_decode_uint16(struct cbor_stream *cs, uint16_t *val)
{
	cbor_err_t err;
	*val = (uint16_t)decode_uint(cs, &err, UINT16_MAX);
	return err;
}


cbor_err_t cbor_decode_uint32(struct cbor_stream *cs, uint32_t *val)
{
	cbor_err_t err;
	*val = (uint32_t)decode_uint(cs, &err, UINT32_MAX);
	return err;
}


cbor_err_t cbor_decode_uint64(struct cbor_stream *cs, uint64_t *val)
{
	cbor_err_t err;
	*val = (uint64_t)decode_uint(cs, &err, UINT64_MAX);
	return err;
}


cbor_err_t cbor_decode_int8(struct cbor_stream *cs, int8_t *val)
{
	cbor_err_t err;
	*val = (int8_t)decode_int(cs, &err, INT8_MIN, INT8_MAX);
	return err;
}


cbor_err_t cbor_decode_int16(struct cbor_stream *cs, int16_t *val)
{
	cbor_err_t err;
	*val = (int16_t)decode_int(cs, &err, INT16_MIN, INT16_MAX);
	return err;
}


cbor_err_t cbor_decode_int32(struct cbor_stream *cs, int32_t *val)
{
	cbor_err_t err;
	*val = (int32_t)decode_int(cs, &err, INT32_MIN, INT32_MAX);
	return err;
}


cbor_err_t cbor_decode_int64(struct cbor_stream *cs, int64_t *val)
{
	cbor_err_t err;
	*val = (int64_t)decode_int(cs, &err, INT64_MIN, INT64_MAX);
	return err;
}


cbor_err_t cbor_decode_tag(struct cbor_stream *cs, uint64_t *tag)
{
	struct cbor_item item;
	cbor_err_t err;

	if ((err = predecode_check(cs, &item, CBOR_TYPE_TAG)) == CBOR_ERR_OK)
		*tag = item.u64;
	return err;
}


cbor_err_t cbor_decode_sval(struct cbor_stream *cs, enum cbor_sval *sval)
{
	struct cbor_item item;
	cbor_err_t err;

	if ((err = predecode_check(cs, &item, CBOR_TYPE_SVAL)) == CBOR_ERR_OK)
		*sval = item.u64;
	return err;
}


static cbor_err_t decode_break(struct cbor_stream *cs)
{
	struct cbor_item item;
	cbor_err_t err;

	if ((err = predecode(cs, &item)) == CBOR_ERR_BREAK)
		return CBOR_ERR_OK;
	return error(cs, CBOR_ERR_ITEM, NULL);
}


//static cbor_err_t decode_block_start(struct cbor_stream *cs, enum cbor_type type, bool indef, uint64_t *len)
//{
//	struct cbor_item item;
//	cbor_err_t err;
//
//	if ((err = predecode_check(cs, &item, type)) != CBOR_ERR_OK)
//		return err;
//
//	if (tmp_indef_flag != indef)
//		return error(cs, CBOR_ERR_INDEF, NULL);
//
//	if (indef)
//		*len = item.u64;
//
//	return CBOR_ERR_OK;
//}
//
//
//static cbor_err_t decode_block_end(struct cbor_stream *cs, enum major major_type)
//{
//	struct block *block;
//
//	if (stack_is_empty(&cs->blocks))
//		return error(cs, CBOR_ERR_OPER, NULL);
//
//	block = stack_pop(&cs->blocks);
//
//	if (block->hdr.major != major_type)
//		return error(cs, CBOR_ERR_OPER, NULL);
//
//	if (block->hdr.indef)
//		return decode_break(cs);
//
//	if (block->num_items != block->hdr.u64)
//		return error(cs, CBOR_ERR_OPER, NULL);
//	
//	return CBOR_ERR_OK;
//}
//
//
//cbor_err_t cbor_decode_array_begin(struct cbor_stream *cs, uint64_t *len)
//{
//	return decode_block_start(cs, CBOR_MAJOR_ARRAY, false, len);
//}
//
//
//cbor_err_t cbor_decode_array_begin_indef(struct cbor_stream *cs)
//{
//	uint64_t tmp;
//	return decode_block_start(cs, CBOR_MAJOR_ARRAY, true, &tmp);
//}
//
//
//cbor_err_t cbor_decode_array_end(struct cbor_stream *cs)
//{
//	return decode_block_end(cs, CBOR_MAJOR_ARRAY);
//}
//
//
//cbor_err_t cbor_decode_map_begin(struct cbor_stream *cs, uint64_t *len)
//{
//	return decode_block_start(cs, CBOR_MAJOR_MAP, false, len);
//}
//
//
//cbor_err_t cbor_decode_map_end(struct cbor_stream *cs)
//{
//	return decode_block_end(cs, CBOR_MAJOR_MAP);
//}


static cbor_err_t read_stream_chunk(struct cbor_stream *cs, struct cbor_item *stream,
	struct cbor_item *chunk, byte_t **bytes, size_t *len)
{
	cbor_err_t err;

	if (chunk->u64 > SIZE_MAX)
		return error(cs, CBOR_ERR_RANGE, "Chunk is too large to be decoded by this implementation.");

	*bytes = nb_realloc(*bytes, 1 + *len + chunk->u64);
	if (!*bytes)
		return error(cs, CBOR_ERR_NOMEM, "No memory to decode another buf chunk.");

	if ((err = buf_read(cs->buf, *bytes + *len, chunk->u64)) != CBOR_ERR_OK) {
		free(*bytes);
		return err;
	}

	*len += chunk->u64;
	return CBOR_ERR_OK;
}


static cbor_err_t read_stream(struct cbor_stream *cs, struct cbor_item *stream,
	byte_t **bytes, size_t *len)
{
	struct cbor_item chunk;
	cbor_err_t err;

	*len = 0;
	*bytes = NULL;

	if (!stream->indefinite)
		return read_stream_chunk(cs, stream, stream, bytes, len);

	while ((err = predecode_check(cs, &chunk, stream->type)) == CBOR_ERR_OK) {
		/* check indef */
		if (chunk.indefinite) {
			err = CBOR_ERR_INDEF;
			break;
		}

		if ((err = read_stream_chunk(cs, stream, &chunk, bytes, len) != CBOR_ERR_OK))
			break;
	}

	if (err == CBOR_ERR_BREAK)
		return CBOR_ERR_OK;

	free(*bytes);
	return err;
}


static cbor_err_t read_stream_0(struct cbor_stream *cs, struct cbor_item *stream, byte_t **bytes, size_t *len)
{
	cbor_err_t err;
	if ((err = read_stream(cs, stream, bytes, len)) == CBOR_ERR_OK)
		(*bytes)[*len] = 0;
	return err;
}


cbor_err_t cbor_decode_bytes(struct cbor_stream *cs, byte_t **str, size_t *len)
{
	struct cbor_item item;
	cbor_err_t err;

	if ((err = predecode_check(cs, &item, CBOR_TYPE_BYTES)) == CBOR_ERR_OK)
		return read_stream(cs, &item, str, len);
	return err;
}


cbor_err_t cbor_decode_text(struct cbor_stream *cs, byte_t **str, size_t *len)
{
	struct cbor_item item;
	cbor_err_t err;

	if ((err = predecode_check(cs, &item, CBOR_TYPE_TEXT)) == CBOR_ERR_OK)
		return read_stream_0(cs, &item, str, len);
	return err;
}


static cbor_err_t decode_array_items(struct cbor_stream *cs, struct cbor_item *arr,
	struct cbor_item **items, uint64_t *nitems)
{
	struct cbor_item *item;
	size_t size;
	cbor_err_t err = CBOR_ERR_OK;
	size_t i;

	size = CBOR_ARRAY_INIT_SIZE;
	if (!arr->indefinite)
		size = arr->u64;

	*items = NULL;
	i = 0;

	do {
		*items = nb_realloc(*items, size * sizeof(**items));
		if (!*items)
			return error(cs, CBOR_ERR_NOMEM, "No memory to decode array items.");

		while (i < size) {
			item = *items + i;

			if ((err = cbor_decode_item(cs, item)) != CBOR_ERR_OK)
				break;

			i++;
		}

		size *= 2;
	} while (arr->indefinite && err != CBOR_ERR_BREAK && err != CBOR_ERR_EOF);

	*nitems = i;

	if (err == CBOR_ERR_BREAK)
		return CBOR_ERR_OK;

	if (err != CBOR_ERR_OK)
		nb_free(*items);
	return err;
}


static cbor_err_t decode_map_items(struct cbor_stream *cs, struct cbor_item *map,
	struct cbor_pair **pairs, uint64_t *npairs)
{
	cbor_err_t err;
	struct cbor_item arr;
	struct cbor_item *items;
	uint64_t nitems;

	/* maps are just arrays, so pretend we have one and reuse decode_array_items */
	arr = (struct cbor_item) {
		.type = CBOR_TYPE_ARRAY,
		.indefinite = map->indefinite,
		.u64 = 2 * map->u64,
	};

	if ((err = decode_array_items(cs, &arr, &items, &nitems)) != CBOR_ERR_OK)
		return err;

	if (nitems % 2 != 0)
		return error(cs, CBOR_ERR_NITEMS, "Odd number of items in a map.");
	*npairs = nitems / 2;

	/* TODO Is this safe? Ask MM */
	*pairs = ((struct cbor_pair *)items);

	return CBOR_ERR_OK;
}


cbor_err_t cbor_decode_item(struct cbor_stream *cs, struct cbor_item *item)
{
	cbor_err_t err;

	if ((err = predecode(cs, item)) != CBOR_ERR_OK)
		return err;

	switch (item->type) {
	case CBOR_TYPE_UINT:
	case CBOR_TYPE_INT:
	case CBOR_TYPE_SVAL:
	case CBOR_TYPE_FLOAT16:
	case CBOR_TYPE_FLOAT32:
	case CBOR_TYPE_FLOAT64:
		return CBOR_ERR_OK;
	case CBOR_TYPE_BYTES:
		return read_stream(cs, item, &item->bytes, &item->len);
	case CBOR_TYPE_TEXT:
		return read_stream_0(cs, item, &item->bytes, &item->len);
	case CBOR_TYPE_ARRAY:
		return decode_array_items(cs, item, &item->items, &item->len);
	case CBOR_TYPE_MAP:
		return decode_map_items(cs, item, &item->pairs, &item->len);

	case CBOR_TYPE_TAG:
		/* TODO clean this up */
		item->tagged_item = nb_malloc(sizeof(*item->tagged_item));
		if (!item->tagged_item)
			return CBOR_ERR_NOMEM;
		return cbor_decode_item(cs, item->tagged_item);

	default:
		return error(cs, CBOR_ERR_UNSUP, NULL);
	}
}
