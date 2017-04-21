#include "debug.h"
#include "internal.h"
#include "memory.h"

#include <assert.h>
#include <endian.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>


#define CBOR_ARRAY_INIT_SIZE	8
#define CBOR_BSTACK_INIT_SIZE	4


struct cbor_decoder *cbor_decoder_new(struct cbor_stream *stream)
{
	struct cbor_decoder *dec;

	dec = cbor_malloc(sizeof(*dec));
	dec->stream = stream;

	if (!stack_init(&dec->blocks, CBOR_BSTACK_INIT_SIZE, sizeof(struct block))) {
		free(dec);
		return NULL;
	}

	return dec;
}


void cbor_decoder_delete(struct cbor_decoder *dec)
{
	cbor_free(dec);
}


static cbor_err_t error(struct cbor_decoder *dec, cbor_err_t err, char *str, ...)
{
	(void) dec;
	(void) str;

	return err;
}


static cbor_err_t decode_hdr(struct cbor_decoder *dec, struct cbor_hdr *type)
{
	byte bytes[9];
	size_t num_extra_bytes;
	uint64_t val_be = 0;
	byte *val_be_bytes = (byte *)&val_be;
	cbor_extra_t extra_bits;
	cbor_err_t err;
	size_t i;

	if ((err = cbor_stream_read(dec->stream, bytes, 0, 1)) != CBOR_ERR_OK)
		return err;

	type->major = (bytes[0] & 0xE0) >> 5;
	extra_bits = bytes[0] & 0x1F;

	/* TODO */
	type->minor = extra_bits;

	type->indef = (extra_bits == CBOR_EXTRA_VAR_LEN);
	if (type->indef)
		return CBOR_ERR_OK;

	if (extra_bits <= 23) {
		type->val = extra_bits;
		type->minor = CBOR_MINOR_SVAL;	/* TODO */
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
		return error(dec, CBOR_ERR_PARSE, "invalid value of extra bits: %u", extra_bits);
	}

	if ((err = cbor_stream_read(dec->stream, bytes, 1, num_extra_bytes)) != CBOR_ERR_OK)
		return err;

	for (i = 0; i < num_extra_bytes; i++)
		val_be_bytes[(8 - num_extra_bytes) + i] = bytes[1 + i];

	type->val = be64toh(val_be);
	return CBOR_ERR_OK;
}


static cbor_err_t decode_hdr_check(struct cbor_decoder *dec, struct cbor_hdr *type, enum cbor_major major)
{
	cbor_err_t err;

	if ((err = decode_hdr(dec, type)) != CBOR_ERR_OK)
		return err;

	if (type->major != major)
		/* TODO msg */
		return error(dec, CBOR_ERR_ITEM, "... was unexpected, ... was expected.");
	
	return CBOR_ERR_OK;
}


static uint64_t decode_uint(struct cbor_decoder *dec, cbor_err_t *err, uint64_t max)
{
	struct cbor_hdr type;

	if ((*err = decode_hdr_check(dec, &type, CBOR_MAJOR_UINT)) != CBOR_ERR_OK)
		return 0;

	if (type.val > max) {
		*err = CBOR_ERR_RANGE;
		return 0;
	}

	*err = CBOR_ERR_OK;
	return type.val;
}


/* TODO have a look at the range checks and make sure nothing overflows */
static inline cbor_err_t uint64_to_negint(struct cbor_decoder *dec, uint64_t u64, int64_t *i64)
{
	uint64_t minabs = -(INT64_MIN + 1);

	if (u64 > minabs)
		return error(dec, CBOR_ERR_RANGE, "Negative integer -%lu is less than -%lu and cannot be decoded.",
			u64, minabs);

	*i64 = (-1 - u64);
	return CBOR_ERR_OK;
}


/* TODO have a look at the range checks and make sure nothing overflows */
static int64_t decode_int(struct cbor_decoder *dec, cbor_err_t *err, int64_t min, int64_t max)
{
	struct cbor_hdr type;
	int64_t i64;

	if ((*err = decode_hdr(dec, &type)) != CBOR_ERR_OK)
		return 0;

	if (type.major != CBOR_MAJOR_NEGATIVE_INT && type.major != CBOR_MAJOR_UINT) {
		*err = CBOR_ERR_ITEM;
		return 0;
	}

	if (type.major == CBOR_MAJOR_NEGATIVE_INT) {
		*err = uint64_to_negint(dec, type.val, &i64);
	}
	else {
		if (type.val > INT64_MAX) {
			*err = CBOR_ERR_RANGE;
			return 0;
		}

		i64 = type.val;
	}

	if (i64 < min || i64 > max) {
		*err = CBOR_ERR_RANGE;
		return 0;
	}

	*err = CBOR_ERR_OK;
	return i64;
}


cbor_err_t cbor_decode_uint8(struct cbor_decoder *dec, uint8_t *val)
{
	cbor_err_t err;
	*val = (uint8_t)decode_uint(dec, &err, UINT8_MAX);
	return err;
}


cbor_err_t cbor_decode_uint16(struct cbor_decoder *dec, uint16_t *val)
{
	cbor_err_t err;
	*val = (uint16_t)decode_uint(dec, &err, UINT16_MAX);
	return err;
}


cbor_err_t cbor_decode_uint32(struct cbor_decoder *dec, uint32_t *val)
{
	cbor_err_t err;
	*val = (uint32_t)decode_uint(dec, &err, UINT32_MAX);
	return err;
}


cbor_err_t cbor_decode_uint64(struct cbor_decoder *dec, uint64_t *val)
{
	cbor_err_t err;
	*val = (uint64_t)decode_uint(dec, &err, UINT64_MAX);
	return err;
}


cbor_err_t cbor_decode_int8(struct cbor_decoder *dec, int8_t *val)
{
	cbor_err_t err;
	*val = (int8_t)decode_int(dec, &err, INT8_MIN, INT8_MAX);
	return err;
}


cbor_err_t cbor_decode_int16(struct cbor_decoder *dec, int16_t *val)
{
	cbor_err_t err;
	*val = (int16_t)decode_int(dec, &err, INT16_MIN, INT16_MAX);
	return err;
}


cbor_err_t cbor_decode_int32(struct cbor_decoder *dec, int32_t *val)
{
	cbor_err_t err;
	*val = (int32_t)decode_int(dec, &err, INT32_MIN, INT32_MAX);
	return err;
}


cbor_err_t cbor_decode_int64(struct cbor_decoder *dec, int64_t *val)
{
	cbor_err_t err;
	*val = (int64_t)decode_int(dec, &err, INT64_MIN, INT64_MAX);
	return err;
}


cbor_err_t cbor_decode_tag(struct cbor_decoder *dec, uint64_t *tagno)
{
	struct cbor_hdr hdr;
	cbor_err_t err;

	if ((err = decode_hdr_check(dec, &hdr, CBOR_MAJOR_TAG)) == CBOR_ERR_OK)
		*tagno = hdr.val;

	return err;
}


cbor_err_t cbor_decode_sval(struct cbor_decoder *dec, enum cbor_sval *sval)
{
	struct cbor_hdr hdr;
	cbor_err_t err;

	if ((err = decode_hdr(dec, &hdr)) != CBOR_ERR_OK)
		return err;

	if (hdr.major != CBOR_MAJOR_OTHER || hdr.minor != CBOR_MINOR_SVAL)
		return error(dec, CBOR_ERR_ITEM, "A Simple Value was expected.");

	*sval = hdr.val;
	return CBOR_ERR_OK;

}


static inline bool is_break(struct cbor_hdr *type)
{
	return (type->major == CBOR_MAJOR_OTHER && type->minor == CBOR_MINOR_BREAK);
}


static cbor_err_t decode_break(struct cbor_decoder *dec)
{
	struct cbor_hdr hdr;
	cbor_err_t err;

	if ((err = decode_hdr(dec, &hdr)) == CBOR_ERR_OK)
		if (!is_break(&hdr))
			return error(dec, CBOR_ERR_ITEM, "Break Code was expected.");

	return CBOR_ERR_OK;
}


static cbor_err_t block_begin(struct cbor_decoder *dec, enum cbor_major major_type, bool indef, uint64_t *len)
{
	struct block *block;
	cbor_err_t err;

	block = stack_push(&dec->blocks);
	if (!block)
		return error(dec, CBOR_ERR_NOMEM, "No memory to allocate new nesting block");

	block->num_items = 0;
	if ((err = decode_hdr_check(dec, &block->type, major_type)) != CBOR_ERR_OK)
		return err;

	if (block->type.indef != indef)
		return error(dec, CBOR_ERR_INDEF, NULL);

	*len = block->type.val;
	return CBOR_ERR_OK;
}


static cbor_err_t block_end(struct cbor_decoder *dec, enum cbor_major major_type)
{
	struct block *block;

	if (stack_is_empty(&dec->blocks))
		return error(dec, CBOR_ERR_OPER, NULL);

	block = stack_pop(&dec->blocks);

	if (block->type.major != major_type)
		return error(dec, CBOR_ERR_OPER, NULL);

	if (block->type.indef)
		return decode_break(dec);

	if (block->num_items != block->type.val)
		return error(dec, CBOR_ERR_OPER, NULL);
	
	return CBOR_ERR_OK;
}


cbor_err_t cbor_decode_array_begin(struct cbor_decoder *dec, uint64_t *len)
{
	return block_begin(dec, CBOR_MAJOR_ARRAY, false, len);
}


cbor_err_t cbor_decode_array_begin_indef(struct cbor_decoder *dec)
{
	uint64_t tmp;
	return block_begin(dec, CBOR_MAJOR_ARRAY, true, &tmp);
}


cbor_err_t cbor_decode_array_end(struct cbor_decoder *dec)
{
	return block_end(dec, CBOR_MAJOR_ARRAY);
}


cbor_err_t cbor_decode_map_begin(struct cbor_decoder *dec, uint64_t *len)
{
	return block_begin(dec, CBOR_MAJOR_MAP, false, len);
}


cbor_err_t cbor_decode_map_end(struct cbor_decoder *dec)
{
	return block_end(dec, CBOR_MAJOR_MAP);
}


static cbor_err_t decode_chunk(struct cbor_decoder *dec, struct cbor_hdr *stream_hdr,
	struct cbor_hdr *chunk_hdr, byte **bytes, size_t *len)
{
	cbor_err_t err;

	*bytes = cbor_realloc(*bytes, 1 + *len + chunk_hdr->val);
	if (!*bytes)
		return error(dec, CBOR_ERR_NOMEM, "No memory to decode another stream chunk.");

	if ((err = cbor_stream_read(dec->stream, *bytes + *len, 0, chunk_hdr->val)) != CBOR_ERR_OK) {
		free(*bytes);
		return err;
	}

	*len += chunk_hdr->val;
	return CBOR_ERR_OK;
}


static cbor_err_t decode_stream(struct cbor_decoder *dec, struct cbor_hdr *stream_hdr,
	byte **bytes, size_t *len)
{
	struct cbor_hdr chunk_hdr;
	cbor_err_t err;

	*len = 0;
	*bytes = NULL;

	if (!stream_hdr->indef)
		return decode_chunk(dec, stream_hdr, stream_hdr, bytes, len);

	while ((err = decode_hdr(dec, &chunk_hdr)) == CBOR_ERR_OK) {
		if (is_break(&chunk_hdr))
			break;

		if (chunk_hdr.major != stream_hdr->major || chunk_hdr.indef) {
			err = CBOR_ERR_ITEM;
			goto out_free;
		}

		if ((err = decode_chunk(dec, stream_hdr, &chunk_hdr, bytes, len) != CBOR_ERR_OK))
			goto out_free;
	}

	return err;

out_free:
	free(*bytes);
	return err;
}


static cbor_err_t decode_stream_null(struct cbor_decoder *dec, struct cbor_hdr *stream_hdr, byte **bytes, size_t *len)
{
	cbor_err_t err;
	if ((err = decode_stream(dec, stream_hdr, bytes, len)) == CBOR_ERR_OK)
		(*bytes)[*len] = 0;
	return err;
}


cbor_err_t cbor_decode_bytes(struct cbor_decoder *dec, byte **str, size_t *len)
{
	struct cbor_hdr hdr;
	cbor_err_t err;

	if ((err = decode_hdr_check(dec, &hdr, CBOR_MAJOR_BYTES)) != CBOR_ERR_OK)
		return err;

	return decode_stream(dec, &hdr, str, len);
}


cbor_err_t cbor_decode_text(struct cbor_decoder *dec, byte **str, size_t *len)
{
	struct cbor_hdr hdr;
	cbor_err_t err;

	if ((err = decode_hdr_check(dec, &hdr, CBOR_MAJOR_TEXT)) != CBOR_ERR_OK)
		return err;

	return decode_stream_null(dec, &hdr, str, len);
}


static cbor_err_t decode_item(struct cbor_decoder *dec, struct cbor_item *item);


static cbor_err_t decode_array_items(struct cbor_decoder *dec, struct cbor_hdr *array_hdr,
	struct cbor_item **items, uint64_t *nitems)
{
	struct cbor_item *item;
	size_t size;
	bool end = false;
	cbor_err_t err;
	size_t i;

	size = CBOR_ARRAY_INIT_SIZE;
	if (!array_hdr->indef) {
		size = array_hdr->val;
	}
	*items = NULL;

	*items = cbor_realloc(*items, size * sizeof(**items));
	if (!*items)
		return error(dec, CBOR_ERR_NOMEM, "No memory to decode array items.");

	i = 0;

more_items:
	for (; i < size; i++) {
		item = *items + i;

		if ((err = decode_hdr(dec, &item->type)) != CBOR_ERR_OK)
			goto out_free;

		if (is_break(&item->type)) {
			end = true;
			break;
		}

		if ((err = decode_item(dec, item)) != CBOR_ERR_OK)
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


static cbor_err_t decode_map_items(struct cbor_decoder *dec, struct cbor_hdr *map_hdr,
	struct cbor_pair **pairs, uint64_t *npairs)
{
	cbor_err_t err;
	struct cbor_hdr array_hdr;
	struct cbor_item *items;

	/* maps are just arrays, so pretend we have one and reuse decode_array_items */
	array_hdr = (struct cbor_hdr) {
		.major = CBOR_MAJOR_ARRAY,
		.indef = map_hdr->indef,
		.val = 2 * map_hdr->val,
	};

	if ((err = decode_array_items(dec, &array_hdr, &items, npairs)) != CBOR_ERR_OK)
		return err;

	/* TODO (A lot of) further checks ... */
	TEMP_ASSERT(*npairs % 2 == 0);
	*npairs /= 2;

	/* TODO Is this safe? Ask MM */
	*pairs = ((struct cbor_pair *)items);


	return CBOR_ERR_OK;
}


static cbor_err_t decode_item(struct cbor_decoder *dec, struct cbor_item *item)
{
	cbor_err_t err;

	switch (item->type.major) {
	case CBOR_MAJOR_NEGATIVE_INT:
		return uint64_to_negint(dec, item->type.val, &item->i64);

	case CBOR_MAJOR_BYTES:
		return decode_stream(dec, &item->type, &item->bytes, &item->len);

	case CBOR_MAJOR_TEXT:
		return decode_stream_null(dec, &item->type, &item->bytes, &item->len);

	case CBOR_MAJOR_ARRAY:
		return decode_array_items(dec, &item->type, &item->items, &item->len);

	case CBOR_MAJOR_MAP:
		return decode_map_items(dec, &item->type, &item->pairs, &item->len);

	default:
		return CBOR_ERR_OK;
	}
}


cbor_err_t cbor_decode_item(struct cbor_decoder *dec, struct cbor_item *item)
{
	cbor_err_t err;

	if ((err = decode_hdr(dec, &item->type)) == CBOR_ERR_OK)
		return decode_item(dec, item);
	return err;
}
