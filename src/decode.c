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


static cbor_err_t error(struct cbor_stream *cs, cbor_err_t err, char *str, ...)
{
	//assert(err == CBOR_ERR_OK); /* uncomment for easier debugging */

	va_list args;

	cs->err = err;
	va_start(args, str);
	strbuf_vprintf_at(&cs->err_buf, 0, str, args);
	va_end(args);
	return err;
}


static cbor_err_t decode_hdr(struct cbor_stream *cs, struct cbor_hdr *hdr)
{
	byte_t bytes[9];
	size_t num_extra_bytes;
	uint64_t val_be = 0;
	byte_t *val_be_bytes = (byte_t *)&val_be;
	enum cbor_minor minor;
	cbor_err_t err;
	size_t i;

	if ((err = buf_read(cs->buf, bytes, 0, 1)) != CBOR_ERR_OK)
		return err;

	hdr->major = (bytes[0] & 0xE0) >> 5;
	minor = bytes[0] & 0x1F;

	/* TODO */
	hdr->minor = minor;

	hdr->indef = (minor == CBOR_MINOR_INDEFINITE_BREAK);
	if (hdr->indef) {
		if (hdr->major == CBOR_MAJOR_7 || major_allows_indef(hdr->major))
			return CBOR_ERR_OK;
		return error(cs, CBOR_ERR_INDEF, "%s does not allow for "
			"indefinite-length encoding", cbor_major_to_string(hdr->major));
	}

	if (minor <= 23) {
		hdr->u64 = minor;
		hdr->minor = CBOR_MINOR_1B_SVAL;	/* TODO */
		return CBOR_ERR_OK;
	}

	switch (minor) {
	case CBOR_MINOR_1B_SVAL:
		num_extra_bytes = 1;
		break;

	case CBOR_MINOR_2B_FLOAT16:
		num_extra_bytes = 2;
		break;

	case CBOR_MINOR_4B_FLOAT32:
		num_extra_bytes = 4;
		break;

	case CBOR_MINOR_8B_FLOAT64:
		num_extra_bytes = 8;
		break;

	default:
		return error(cs, CBOR_ERR_PARSE, "invalid value of extra bits: %u", minor);
	}

	if ((err = buf_read(cs->buf, bytes, 1, num_extra_bytes)) != CBOR_ERR_OK)
		return err;

	for (i = 0; i < num_extra_bytes; i++)
		val_be_bytes[(8 - num_extra_bytes) + i] = bytes[1 + i];

	hdr->u64 = be64toh(val_be);
	return CBOR_ERR_OK;
}


static cbor_err_t decode_hdr_check(struct cbor_stream *cs, struct cbor_hdr *hdr, enum cbor_major major)
{
	cbor_err_t err;

	if ((err = decode_hdr(cs, hdr)) != CBOR_ERR_OK)
		return err;

	if (hdr->major != major)
		/* TODO msg */
		return error(cs, CBOR_ERR_ITEM, "%s was unexpected, %s was expected.",
			hdr->major, major);
	
	return CBOR_ERR_OK;
}


static uint64_t decode_uint(struct cbor_stream *cs, cbor_err_t *err, uint64_t max)
{
	struct cbor_hdr hdr;

	if ((*err = decode_hdr_check(cs, &hdr, CBOR_MAJOR_UINT)) != CBOR_ERR_OK)
		return 0;

	if (hdr.u64 > max) {
		*err = CBOR_ERR_RANGE;
		return 0;
	}

	*err = CBOR_ERR_OK;
	return hdr.u64;
}


/* TODO have a look at the range checks and make sure nothing overflows */
static inline cbor_err_t uint64_to_negint(struct cbor_stream *cs, uint64_t u64, int64_t *i64)
{
	uint64_t minabs = -(INT64_MIN + 1);

	if (u64 > minabs)
		return error(cs, CBOR_ERR_RANGE, "Negative integer -%lu is less than -%lu and cannot be decoded.",
			u64, minabs);

	*i64 = (-1 - u64);
	return CBOR_ERR_OK;
}


/* TODO have a look at the range checks and make sure nothing overflows */
static int64_t decode_int(struct cbor_stream *cs, cbor_err_t *err, int64_t min, int64_t max)
{
	struct cbor_hdr hdr;
	int64_t i64;

	if ((*err = decode_hdr(cs, &hdr)) != CBOR_ERR_OK)
		return 0;

	if (hdr.major != CBOR_MAJOR_NEGINT && hdr.major != CBOR_MAJOR_UINT) {
		*err = CBOR_ERR_ITEM;
		return 0;
	}

	if (hdr.major == CBOR_MAJOR_NEGINT) {
		*err = uint64_to_negint(cs, hdr.u64, &i64);
	}
	else {
		if (hdr.u64 > INT64_MAX) {
			*err = CBOR_ERR_RANGE;
			return 0;
		}

		i64 = hdr.u64;
	}

	if (i64 < min || i64 > max) {
		*err = CBOR_ERR_RANGE;
		return 0;
	}

	*err = CBOR_ERR_OK;
	return i64;
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


cbor_err_t cbor_decode_tag(struct cbor_stream *cs, uint64_t *tagno)
{
	struct cbor_hdr hdr;
	cbor_err_t err;

	if ((err = decode_hdr_check(cs, &hdr, CBOR_MAJOR_TAG)) == CBOR_ERR_OK)
		*tagno = hdr.u64;

	return err;
}


cbor_err_t cbor_decode_sval(struct cbor_stream *cs, enum cbor_sval *sval)
{
	struct cbor_hdr hdr;
	cbor_err_t err;

	if ((err = decode_hdr(cs, &hdr)) != CBOR_ERR_OK)
		return err;

	if (hdr.major != CBOR_MAJOR_7 || hdr.minor != CBOR_MINOR_1B_SVAL)
		return error(cs, CBOR_ERR_ITEM, "A Simple Value was expected.");

	*sval = hdr.u64;
	return CBOR_ERR_OK;

}


static cbor_err_t decode_break(struct cbor_stream *cs)
{
	struct cbor_hdr hdr;
	cbor_err_t err;

	if ((err = decode_hdr(cs, &hdr)) == CBOR_ERR_OK)
		if (!is_break(&hdr))
			return error(cs, CBOR_ERR_ITEM, "Break was expected.");

	return CBOR_ERR_OK;
}


static cbor_err_t block_begin(struct cbor_stream *cs, enum cbor_major major_type, bool indef, uint64_t *len)
{
	struct block *block;
	cbor_err_t err;

	block = stack_push(&cs->blocks);
	if (!block)
		return error(cs, CBOR_ERR_NOMEM, "No memory to allocate new nesting block.");

	block->num_items = 0;
	if ((err = decode_hdr_check(cs, &block->hdr, major_type)) != CBOR_ERR_OK)
		return err;

	if (block->hdr.indef != indef)
		return error(cs, CBOR_ERR_INDEF, NULL);

	*len = block->hdr.u64;
	return CBOR_ERR_OK;
}


static cbor_err_t block_end(struct cbor_stream *cs, enum cbor_major major_type)
{
	struct block *block;

	if (stack_is_empty(&cs->blocks))
		return error(cs, CBOR_ERR_OPER, NULL);

	block = stack_pop(&cs->blocks);

	if (block->hdr.major != major_type)
		return error(cs, CBOR_ERR_OPER, NULL);

	if (block->hdr.indef)
		return decode_break(cs);

	if (block->num_items != block->hdr.u64)
		return error(cs, CBOR_ERR_OPER, NULL);
	
	return CBOR_ERR_OK;
}


cbor_err_t cbor_decode_array_begin(struct cbor_stream *cs, uint64_t *len)
{
	return block_begin(cs, CBOR_MAJOR_ARRAY, false, len);
}


cbor_err_t cbor_decode_array_begin_indef(struct cbor_stream *cs)
{
	uint64_t tmp;
	return block_begin(cs, CBOR_MAJOR_ARRAY, true, &tmp);
}


cbor_err_t cbor_decode_array_end(struct cbor_stream *cs)
{
	return block_end(cs, CBOR_MAJOR_ARRAY);
}


cbor_err_t cbor_decode_map_begin(struct cbor_stream *cs, uint64_t *len)
{
	return block_begin(cs, CBOR_MAJOR_MAP, false, len);
}


cbor_err_t cbor_decode_map_end(struct cbor_stream *cs)
{
	return block_end(cs, CBOR_MAJOR_MAP);
}


static cbor_err_t decode_chunk(struct cbor_stream *cs, struct cbor_hdr *stream_hdr,
	struct cbor_hdr *chunk_hdr, byte_t **bytes, size_t *len)
{
	cbor_err_t err;

	if (chunk_hdr->u64 > SIZE_MAX)
		return error(cs, CBOR_ERR_RANGE, "Chunk is too large to be decoded by this implementation.");

	*bytes = cbor_realloc(*bytes, 1 + *len + chunk_hdr->u64);
	if (!*bytes)
		return error(cs, CBOR_ERR_NOMEM, "No memory to decode another buf chunk.");

	if ((err = buf_read(cs->buf, *bytes + *len, 0, chunk_hdr->u64)) != CBOR_ERR_OK) {
		free(*bytes);
		return err;
	}

	*len += chunk_hdr->u64;
	return CBOR_ERR_OK;
}


static cbor_err_t decode_stream(struct cbor_stream *cs, struct cbor_hdr *stream_hdr,
	byte_t **bytes, size_t *len)
{
	struct cbor_hdr chunk_hdr;
	cbor_err_t err;

	*len = 0;
	*bytes = NULL;

	if (!stream_hdr->indef)
		return decode_chunk(cs, stream_hdr, stream_hdr, bytes, len);

	while ((err = decode_hdr(cs, &chunk_hdr)) == CBOR_ERR_OK) {
		if (is_break(&chunk_hdr))
			break;

		if (chunk_hdr.major != stream_hdr->major || chunk_hdr.indef) {
			err = CBOR_ERR_ITEM;
			goto out_free;
		}

		if ((err = decode_chunk(cs, stream_hdr, &chunk_hdr, bytes, len) != CBOR_ERR_OK))
			goto out_free;
	}

	return err;

out_free:
	free(*bytes);
	return err;
}


static cbor_err_t decode_stream_null(struct cbor_stream *cs, struct cbor_hdr *stream_hdr, byte_t **bytes, size_t *len)
{
	cbor_err_t err;
	if ((err = decode_stream(cs, stream_hdr, bytes, len)) == CBOR_ERR_OK)
		(*bytes)[*len] = 0;
	return err;
}


cbor_err_t cbor_decode_bytes(struct cbor_stream *cs, byte_t **str, size_t *len)
{
	struct cbor_hdr hdr;
	cbor_err_t err;

	if ((err = decode_hdr_check(cs, &hdr, CBOR_MAJOR_BYTES)) != CBOR_ERR_OK)
		return err;

	return decode_stream(cs, &hdr, str, len);
}


cbor_err_t cbor_decode_text(struct cbor_stream *cs, byte_t **str, size_t *len)
{
	struct cbor_hdr hdr;
	cbor_err_t err;

	if ((err = decode_hdr_check(cs, &hdr, CBOR_MAJOR_TEXT)) != CBOR_ERR_OK)
		return err;

	return decode_stream_null(cs, &hdr, str, len);
}


static cbor_err_t decode_item(struct cbor_stream *cs, struct cbor_item *item);


static cbor_err_t decode_array_items(struct cbor_stream *cs, struct cbor_hdr *array_hdr,
	struct cbor_item **items, uint64_t *nitems)
{
	struct cbor_item *item;
	size_t size;
	bool end = false;
	cbor_err_t err;
	size_t i;

	size = CBOR_ARRAY_INIT_SIZE;
	if (!array_hdr->indef)
		size = array_hdr->u64;

	*items = NULL;
	i = 0;

more_items:
	*items = cbor_realloc(*items, size * sizeof(**items));
	if (!*items)
		return error(cs, CBOR_ERR_NOMEM, "No memory to decode array items.");

	for (; i < size; i++) {
		item = *items + i;

		if ((err = decode_hdr(cs, &item->hdr)) != CBOR_ERR_OK)
			goto out_free;

		if (is_break(&item->hdr)) {
			end = true;
			break;
		}

		if ((err = decode_item(cs, item)) != CBOR_ERR_OK)
			goto out_free;
	}

	if (!end && array_hdr->indef) {
		size *= 2;
		goto more_items;
	}

	*nitems = i;
	return CBOR_ERR_OK;

out_free:
	cbor_free(*items);
	return err;
}


static cbor_err_t decode_map_items(struct cbor_stream *cs, struct cbor_hdr *map_hdr,
	struct cbor_pair **pairs, uint64_t *npairs)
{
	cbor_err_t err;
	struct cbor_hdr array_hdr;
	struct cbor_item *items;

	/* maps are just arrays, so pretend we have one and reuse decode_array_items */
	array_hdr = (struct cbor_hdr) {
		.major = CBOR_MAJOR_ARRAY,
		.indef = map_hdr->indef,
		.u64 = 2 * map_hdr->u64,
	};

	if ((err = decode_array_items(cs, &array_hdr, &items, npairs)) != CBOR_ERR_OK)
		return err;

	/* TODO (A lot of) further checks ... */
	TEMP_ASSERT(*npairs % 2 == 0);
	*npairs /= 2;

	/* TODO Is this safe? Ask MM */
	*pairs = ((struct cbor_pair *)items);


	return CBOR_ERR_OK;
}


static cbor_err_t decode_item(struct cbor_stream *cs, struct cbor_item *item)
{
	cbor_err_t err;

	switch (item->hdr.major) {
	case CBOR_MAJOR_NEGINT:
		return uint64_to_negint(cs, item->hdr.u64, &item->i64);

	case CBOR_MAJOR_BYTES:
		return decode_stream(cs, &item->hdr, &item->bytes, &item->len);

	case CBOR_MAJOR_TEXT:
		return decode_stream_null(cs, &item->hdr, &item->bytes, &item->len);

	case CBOR_MAJOR_ARRAY:
		return decode_array_items(cs, &item->hdr, &item->items, &item->len);

	case CBOR_MAJOR_MAP:
		return decode_map_items(cs, &item->hdr, &item->pairs, &item->len);

	default:
		return CBOR_ERR_OK;
	}
}


cbor_err_t cbor_decode_item(struct cbor_stream *cs, struct cbor_item *item)
{
	cbor_err_t err;

	if ((err = decode_hdr(cs, &item->hdr)) == CBOR_ERR_OK)
		return decode_item(cs, item);
	return err;
}
