/*
 * decode:
 * CBOR Decoder
 *
 * TODO Get rid of all recursion, or limit allowed nesting of maps and arrays.
 * TODO Open a nesting block when using cbor_decode_item and we run into an array, too.
 * TODO Enforce memory limits (globally or per item).
 * TODO Decoding of tagged items
 */

#include "buffer.h"
#include "cbor-internal.h"
#include "cbor.h"
#include "debug.h"
#include "diag.h"
#include "memory.h"
#include "util.h"

#include <assert.h>
#include <endian.h>
#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>

#define CBOR_ARRAY_INIT_SIZE	8
#define CBOR_BSTACK_INIT_SIZE	4
#define INT64_MIN_ABS			(-(INT64_MIN + 1))

#define	MAJOR_MASK	0xE0
#define	LBITS_MASK	0x1F

#define diag_finish_item(cs)	diag_if_on((cs)->diag, diag_finish_item_do(cs))


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


static inline nb_err_t read_stream(struct cbor_stream *cs, nb_byte_t *bytes, size_t nbytes)
{
	nb_err_t err;
	
	diag_log_offset(cs->diag, nb_buffer_tell(cs->buf));
	if ((err = nb_buffer_read(cs->buf, bytes, nbytes)) != NB_ERR_OK)
		return error(cs, err,
			err == NB_ERR_EOF ? "EOF was unexpected." : "(buffer error).");
	diag_log_raw(cs->diag, bytes, MIN(nbytes, 4));
	return NB_ERR_OK;
}


static inline nb_err_t decode_u64(struct cbor_stream *cs, enum lbits lbits, uint64_t *u64)
{
	nb_byte_t bytes[8];
	uint8_t nbytes;
	uint64_t u64be = 0;
	nb_byte_t *u64be_ptr = (nb_byte_t *)&u64be;
	size_t i;
	nb_err_t err;

	if (lbits <= 23) {
		*u64 = lbits;
		return NB_ERR_OK;
	}

	assert(lbits != LBITS_INDEFINITE);
	if (lbits > LBITS_8B)
		return error(cs, NB_ERR_PARSE,
			"Invalid value of Additional Information: 0x%02X.", lbits);

	nbytes = lbits_to_nbytes(lbits);
	if ((err = read_stream(cs, bytes, nbytes)) != NB_ERR_OK)
		return error(cs, err, "Error occured while reading the stream.");

	for (i = 0; i < nbytes; i++)
		u64be_ptr[(8 - nbytes) + i] = bytes[i];

	*u64 = be64toh(u64be);
	return NB_ERR_OK;
}


/* TODO have a look at the range checks and make sure nothing overflows */
static inline nb_err_t u64_to_i64(struct cbor_stream *cs, uint64_t u64, int64_t *i64)
{
	if (u64 <= INT64_MIN_ABS) {
		*i64 = -u64 - 1;
		return NB_ERR_OK;
	}

	return error(cs, NB_ERR_RANGE,
		"Negative integer -%lu less than -%lu cannot be decoded.", u64, INT64_MIN_ABS);
}


static void diag_finish_item_do(struct cbor_stream *cs)
{
	switch (top_block(cs)->type) {
	case CBOR_TYPE_ARRAY:
		diag_eol(cs->diag, true);
		break;

	case CBOR_TYPE_MAP: /* TODO */
		if (top_block(cs)->num_items % 2 == 0) {
			diag_dedent_cbor(cs->diag);
			diag_eol(cs->diag, true);
		}
		else {
			diag_log_cbor(cs->diag, ":");
			diag_eol(cs->diag, false);
			diag_indent_cbor(cs->diag);
		}
		break;

	default:
		if (cbor_block_stack_empty(cs))
			diag_eol(cs->diag, true);
	}
}


static void print_sval_diag(struct cbor_stream *cs, enum cbor_sval sval)
{
	const char *sval_name;

	diag_log_item(cs->diag, "sval(%u)", sval);

	sval_name = diag_get_sval_name(cs->diag, sval);
	if (sval_name)
		diag_log_cbor(cs->diag, "%s", sval_name);
	else
		diag_log_cbor(cs->diag, "simple(%lu)", sval);

	diag_finish_item(cs);
}


static nb_err_t decode_item_major7(struct cbor_stream *cs, struct cbor_item *item,
	enum minor minor)
{
	uint64_t u64 = 0;
	nb_err_t err;

	assert(minor >= 0);

	if (minor <= 23) {
		item->type = CBOR_TYPE_SVAL;
		item->sval = minor;
		item->u64 = item->sval;
		print_sval_diag(cs, item->sval);
		return NB_ERR_OK;
	}

	if (minor == CBOR_MINOR_BREAK) {
		diag_log_item(cs->diag, "break");
		diag_eol(cs->diag, false);
		return cs->err = NB_ERR_BREAK;
	}

	/* leverage decode_u64 to read the appropriate amount of data */
	if ((err = decode_u64(cs, (enum lbits)minor, &u64)) != NB_ERR_OK)
		return err;

	switch (minor) {
	case CBOR_MINOR_SVAL:
		item->type = CBOR_TYPE_SVAL; /* deduplicate */
		item->sval = u64;
		item->u64 = item->sval; /* TODO */
		print_sval_diag(cs, item->sval);
		return NB_ERR_OK;
	case CBOR_MINOR_FLOAT16:
	case CBOR_MINOR_FLOAT32:
	case CBOR_MINOR_FLOAT64:
		return error(cs, NB_ERR_UNSUP, "Float decoding not supported.");
	default:
		break;
	}

	return error(cs, NB_ERR_PARSE, "Invalid Additional Information "
		"value for Major Type 7: 0x%02X.", minor);
}


static nb_err_t predecode(struct cbor_stream *cs, struct cbor_item *item)
{
	nb_byte_t hdr;
	enum major major;
	nb_byte_t lbits;
	uint64_t u64 = 0;
	nb_err_t err;

	/* TODO hacky hacky */
	if (cs->peeking) {
		*item = cs->peek; /* TODO avoid the copy */
		cs->peeking = false;
		err = cs->err;
		cs->err = NB_ERR_OK;
		return err;
	}

	top_block(cs)->num_items++;

	if ((err = read_stream(cs, &hdr, 1)) != NB_ERR_OK)
		return err;

	major = (hdr & MAJOR_MASK) >> 5;
	lbits = hdr & LBITS_MASK;
	assert(major >= 0 && major <= 7);

	diag_comma(cs->diag); /* append a comma to previous line if needed */
	item->flags = 0;

	if (major == CBOR_MAJOR_UINT && lbits <= 23) {
		item->type = CBOR_TYPE_UINT;
		u64 = lbits;
	}
	else {
		if (major == CBOR_MAJOR_7)
			return decode_item_major7(cs, item, lbits);

		item->type = (enum cbor_type)major;

		if (lbits == LBITS_INDEFINITE)
			item->flags |= CBOR_FLAG_INDEFINITE;

		if (!is_indefinite(item)) {
			if ((err = decode_u64(cs, lbits, &u64)) != NB_ERR_OK)
				return err;
		}
		else {
			if (!major_allows_indefinite(major))
				return error(cs, NB_ERR_INDEF, "Indefinite-length encoding "
					"is not allowed for %s items.", cbor_type_string(major));
			diag_log_item(cs->diag, "%s(_)", cbor_type_string(item->type));
			return NB_ERR_OK;
		}
	}

	diag_log_item(cs->diag, "%s(%lu)", cbor_type_string(item->type), u64);

	item->u64 = u64; /* TODO remove */
	switch (item->type)
	{
	case CBOR_TYPE_UINT:
		item->u64 = u64;
		diag_log_cbor(cs->diag, "%lu", item->u64);
		diag_finish_item(cs);
		break;

	case CBOR_TYPE_INT:
		if ((err = u64_to_i64(cs, u64, &item->i64)) != NB_ERR_OK)
			return err;
		diag_log_cbor(cs->diag, "%li", item->i64);
		diag_finish_item(cs);
		break;

	case CBOR_TYPE_BYTES:
	case CBOR_TYPE_TEXT:
	case CBOR_TYPE_ARRAY:
	case CBOR_TYPE_MAP:
		item->len = u64;
		break;
	
	case CBOR_TYPE_TAG:
		item->tag = u64;
		diag_finish_item(cs);
		break;

	default:
		return error(cs, NB_ERR_PARSE, "Invalid major type.");
	}

	return NB_ERR_OK;
}


nb_err_t cbor_peek(struct cbor_stream *cs, struct cbor_item *item)
{
	nb_err_t err;

	err = predecode(cs, &cs->peek);
	*item = cs->peek; /* TODO avoid the copy */
	cs->peeking = true;
	return err;
}


static inline nb_err_t predecode_check(struct cbor_stream *cs, struct cbor_item *item,
	enum cbor_type type)
{
	nb_err_t err;

	if ((err = predecode(cs, item)) != NB_ERR_OK)
		return err;

	if (item->type != type)
		/* TODO msg */
		return error(cs, NB_ERR_ITEM, "%s was unexpected, %s was expected.",
			cbor_type_string(item->type), cbor_type_string(type));

	return NB_ERR_OK;
}


static uint64_t decode_uint(struct cbor_stream *cs, nb_err_t *err, uint64_t max)
{
	struct cbor_item item;

	if ((*err = predecode(cs, &item)) != NB_ERR_OK)
		return 0;

	if (item.u64 > max) {
		*err = NB_ERR_RANGE;
		return 0;
	}

	*err = NB_ERR_OK;
	return item.u64;
}


/* TODO have a look at the range checks and make sure nothing overflows */
static int64_t decode_int(struct cbor_stream *cs, nb_err_t *err, int64_t min, int64_t max)
{
	struct cbor_item item;
	int64_t i64;

	if ((*err = predecode(cs, &item)) != NB_ERR_OK)
		return 0;

	if (item.type == CBOR_TYPE_INT) {
		i64 = item.i64;
	}
	else if (item.type == CBOR_TYPE_UINT) {
		if (item.u64 > INT64_MAX) {
			*err = NB_ERR_RANGE;
			return 0;
		}

		i64 = (int64_t)item.u64;
	}
	else {
		*err = error(cs, NB_ERR_ITEM, "%s was unexpected, (unsigned) integer was expected",
			cbor_type_string(item.type));
		return 0;
	}

	if (i64 >= min && i64 <= max)
		return i64;

	*err = NB_ERR_RANGE;
	return 0;
}


nb_err_t cbor_decode_uint8(struct cbor_stream *cs, uint8_t *val)
{
	nb_err_t err;
	*val = (uint8_t)decode_uint(cs, &err, UINT8_MAX);
	return err;
}


nb_err_t cbor_decode_uint16(struct cbor_stream *cs, uint16_t *val)
{
	nb_err_t err;
	*val = (uint16_t)decode_uint(cs, &err, UINT16_MAX);
	return err;
}


nb_err_t cbor_decode_uint32(struct cbor_stream *cs, uint32_t *val)
{
	nb_err_t err;
	*val = (uint32_t)decode_uint(cs, &err, UINT32_MAX);
	return err;
}


nb_err_t cbor_decode_uint64(struct cbor_stream *cs, uint64_t *val)
{
	nb_err_t err;
	*val = (uint64_t)decode_uint(cs, &err, UINT64_MAX);
	return err;
}


nb_err_t cbor_decode_int8(struct cbor_stream *cs, int8_t *val)
{
	nb_err_t err;
	*val = (int8_t)decode_int(cs, &err, INT8_MIN, INT8_MAX);
	return err;
}


nb_err_t cbor_decode_int16(struct cbor_stream *cs, int16_t *val)
{
	nb_err_t err;
	*val = (int16_t)decode_int(cs, &err, INT16_MIN, INT16_MAX);
	return err;
}


nb_err_t cbor_decode_int32(struct cbor_stream *cs, int32_t *val)
{
	nb_err_t err;
	*val = (int32_t)decode_int(cs, &err, INT32_MIN, INT32_MAX);
	return err;
}


nb_err_t cbor_decode_int64(struct cbor_stream *cs, int64_t *val)
{
	nb_err_t err;
	*val = (int64_t)decode_int(cs, &err, INT64_MIN, INT64_MAX);
	return err;
}


nb_err_t cbor_decode_tag(struct cbor_stream *cs, uint32_t *tag)
{
	struct cbor_item item;
	nb_err_t err;

	if ((err = predecode_check(cs, &item, CBOR_TYPE_TAG)) != NB_ERR_OK)
		return err;

	if (item.u64 > UINT32_MAX)
		return error(cs, NB_ERR_RANGE,
			"This implementation only supports uint32 tags in the range "
			"0, ..., %lu; %lu was given", UINT32_MAX, item.u64);

	*tag = (uint32_t)item.u64;
	return NB_ERR_OK;
}


nb_err_t cbor_decode_sval(struct cbor_stream *cs, enum cbor_sval *sval)
{
	struct cbor_item item;
	nb_err_t err;

	if ((err = predecode_check(cs, &item, CBOR_TYPE_SVAL)) == NB_ERR_OK)
		*sval = item.u64;
	return err;
}


nb_err_t cbor_decode_bool(struct cbor_stream *cs, bool *b)
{
	enum cbor_sval sval;
	nb_err_t err;

	if ((err = cbor_decode_sval(cs, &sval)) != NB_ERR_OK)
		return err;

	if (sval != CBOR_SVAL_TRUE && sval != CBOR_SVAL_FALSE)
		return error(cs, NB_ERR_RANGE, "expected bool, but the decoded simple "
			"value was neither true nor false");

	*b = (sval == CBOR_SVAL_TRUE);
	return NB_ERR_OK;
}


static nb_err_t decode_break(struct cbor_stream *cs)
{
	struct cbor_item item;
	nb_err_t err;

	if ((cs->err == NB_ERR_BREAK && !cs->peeking)
		|| (err = predecode(cs, &item)) == NB_ERR_BREAK)
		return cs->err = NB_ERR_OK;

	return error(cs, NB_ERR_ITEM, "unexpected %s, break was expected",
		cbor_type_string(item.type));
}


static nb_err_t decode_block_start(struct cbor_stream *cs, enum cbor_type type,
	bool indef, uint64_t *len)
{
	struct cbor_item item;
	size_t block_len = 0;
	nb_err_t err;

	if ((err = predecode_check(cs, &item, type)) != NB_ERR_OK)
		return err;

	if (is_indefinite(&item) != indef)
		return error(cs, NB_ERR_INDEF, NULL);

	if (!is_indefinite(&item)) {
		*len = item.u64;
		block_len = item.u64;
		if (type == CBOR_TYPE_MAP)
			block_len *= 2;
	}

	return push_block(cs, item.type, is_indefinite(&item), block_len);
}


/*
 * TODO Make the checks work.
 */
static nb_err_t decode_block_end(struct cbor_stream *cs, enum cbor_type type)
{
	struct block *block;
	size_t err;

	if (cbor_block_stack_empty(cs))
		return error(cs, NB_ERR_OPER, "Cannot end block, there's no block open.");

	block = stack_top(&cs->blocks);
	if (block->type != type)
		return error(cs, NB_ERR_OPER, NULL);

	if (block->indefinite) {
		if ((err = decode_break(cs)) != NB_ERR_OK)
			return err;
	}
	else if (block->num_items != block->len) {
		return error(cs, NB_ERR_OPER,
			"Cannot end %s: designated size is %lu, but %lu items were decoded",
			cbor_type_string(block->type), block->len, block->num_items);
	}

	(void) stack_pop(&cs->blocks);
	return NB_ERR_OK;
}


nb_err_t cbor_decode_array_begin(struct cbor_stream *cs, uint64_t *len)
{
	nb_err_t err;

	if ((err = decode_block_start(cs, CBOR_TYPE_ARRAY, false, len)) != NB_ERR_OK)
		return err;

	diag_log_cbor(cs->diag, "[");
	diag_eol(cs->diag, false);
	diag_indent_cbor(cs->diag);

	return NB_ERR_OK;
}


nb_err_t cbor_decode_array_begin_indef(struct cbor_stream *cs)
{
	nb_err_t err;
	uint64_t foo;

	if ((err = decode_block_start(cs, CBOR_TYPE_ARRAY, true, &foo)) != NB_ERR_OK)
		return err;

	diag_log_cbor(cs->diag, cs->diag->print_json ? "[" : "[_");
	diag_eol(cs->diag, false);
	diag_indent_cbor(cs->diag);

	return NB_ERR_OK;
}


nb_err_t cbor_decode_array_end(struct cbor_stream *cs)
{
	nb_err_t err;
	bool indefinite;

	indefinite = top_block(cs)->indefinite;
	if ((err = decode_block_end(cs, CBOR_TYPE_ARRAY)) != NB_ERR_OK)
		return err;

	if (!indefinite)
		diag_force_newline(cs->diag); /* TODO cleanup */

	diag_dedent_cbor(cs->diag);
	diag_log_cbor(cs->diag, "]");
	diag_finish_item(cs);

	return NB_ERR_OK;
}


nb_err_t cbor_decode_map_begin(struct cbor_stream *cs, uint64_t *len)
{
	nb_err_t err;

	if ((err = decode_block_start(cs, CBOR_TYPE_MAP, false, len)) != NB_ERR_OK)
		return err;

	diag_log_cbor(cs->diag, "{");
	diag_eol(cs->diag, false);
	diag_indent_cbor(cs->diag);

	return NB_ERR_OK;
}


nb_err_t cbor_decode_map_begin_indef(struct cbor_stream *cs)
{
	nb_err_t err;
	uint64_t foo;

	if ((err = decode_block_start(cs, CBOR_TYPE_MAP, true, &foo)) != NB_ERR_OK)
		return err;

	diag_log_cbor(cs->diag, cs->diag->print_json ? "{" : "{_");
	diag_eol(cs->diag, false);
	diag_indent_cbor(cs->diag);

	return NB_ERR_OK;
}


nb_err_t cbor_decode_map_end(struct cbor_stream *cs)
{
	nb_err_t err;
	bool indefinite;

	indefinite = top_block(cs)->indefinite;
	if ((err = decode_block_end(cs, CBOR_TYPE_MAP)) != NB_ERR_OK)
		return err;

	if (!indefinite)
		diag_force_newline(cs->diag); /* TODO cleanup */

	diag_dedent_cbor(cs->diag);
	diag_log_cbor(cs->diag, "}");
	diag_finish_item(cs);

	return NB_ERR_OK;
}


static nb_err_t read_stream_chunk(struct cbor_stream *cs, struct cbor_item *stream,
	struct cbor_item *chunk, nb_byte_t **bytes, size_t *len)
{
	nb_err_t err;

	if (chunk->u64 > SIZE_MAX)
		return error(cs, NB_ERR_RANGE,
			"Chunk is too large to be decoded by this implementation.");

	*bytes = nb_realloc(*bytes, 1 + *len + chunk->u64);
	if (!*bytes)
		return error(cs, NB_ERR_NOMEM, "No memory to decode another buf chunk.");

	if ((err = read_stream(cs, *bytes + *len, chunk->u64)) != NB_ERR_OK) {
		free(*bytes);
		return err;
	}

	(*bytes)[*len + chunk->u64] = 0;
	*len += chunk->u64;
	return NB_ERR_OK;
}


nb_err_t cbor_decode_stream(struct cbor_stream *cs, struct cbor_item *stream,
	nb_byte_t **bytes, size_t *len)
{
	struct cbor_item chunk;
	nb_err_t err;

	*len = 0;
	*bytes = NULL;

	if (!is_indefinite(stream))
		return read_stream_chunk(cs, stream, stream, bytes, len);

	while ((err = predecode_check(cs, &chunk, stream->type)) == NB_ERR_OK) {
		/* check indef */
		if (is_indefinite(&chunk)) {
			err = NB_ERR_INDEF;
			break;
		}

		if ((err = read_stream_chunk(cs, stream, &chunk, bytes, len) != NB_ERR_OK))
			break;
	}

	if (err == NB_ERR_BREAK)
		return NB_ERR_OK;

	free(*bytes);
	return err;
}


nb_err_t cbor_decode_stream0(struct cbor_stream *cs, struct cbor_item *stream,
	nb_byte_t **bytes, size_t *len)
{
	nb_err_t err;
	if ((err = cbor_decode_stream(cs, stream, bytes, len)) == NB_ERR_OK)
		(*bytes)[*len] = 0;
	return err;
}


nb_err_t cbor_decode_bytes(struct cbor_stream *cs, nb_byte_t **str, size_t *len)
{
	struct cbor_item item;
	nb_err_t err;

	if ((err = predecode_check(cs, &item, CBOR_TYPE_BYTES)) != NB_ERR_OK)
		return err;

	if ((err = cbor_decode_stream(cs, &item, str, len)) != NB_ERR_OK)
		return err;

	diag_log_cbor(cs->diag, "\"%s\"", *str);
	diag_finish_item(cs);
	return NB_ERR_OK;
}


static nb_err_t decode_text(struct cbor_stream *cs, char **str, size_t *len)
{
	struct cbor_item item;
	nb_err_t err;

	if ((err = predecode_check(cs, &item, CBOR_TYPE_TEXT)) != NB_ERR_OK)
		return err;

	if ((err = cbor_decode_stream0(cs, &item, (nb_byte_t **)str, len)) != NB_ERR_OK)
		return err;

	diag_log_cbor(cs->diag, "\"%s\"", *str);
	diag_finish_item(cs);
	return NB_ERR_OK;
}


nb_err_t cbor_decode_text(struct cbor_stream *cs, char **str)
{
	size_t unused;
	return decode_text(cs, str, &unused);
}


static nb_err_t decode_array_items(struct cbor_stream *cs, struct cbor_item *arr,
	struct cbor_item **items, uint64_t *nitems, bool map_diag)
{
	struct cbor_item *item;
	size_t size;
	nb_err_t err = NB_ERR_OK;
	size_t i;

	size = CBOR_ARRAY_INIT_SIZE;
	if (!is_indefinite(arr))
		size = arr->u64;

	*items = NULL;
	i = 0;

	do {
		*items = nb_realloc(*items, size * sizeof(**items));
		if (!*items)
			return error(cs, NB_ERR_NOMEM, "No memory to decode array items.");

		while (i < size) {
			item = *items + i;
			if ((err = cbor_decode_item(cs, item)) != NB_ERR_OK)
				break;
			i++;
		}

		size *= 2;
	} while (is_indefinite(arr) && err != NB_ERR_BREAK && err != NB_ERR_EOF);

	*nitems = i;

	if (err == NB_ERR_BREAK)
		return NB_ERR_OK;

	if (err != NB_ERR_OK)
		xfree(*items);
	return err;
}


static nb_err_t decode_map_items(struct cbor_stream *cs, struct cbor_item *map,
	struct cbor_pair **pairs, uint64_t *npairs)
{
	nb_err_t err;
	struct cbor_item arr;
	struct cbor_item *items;
	uint64_t nitems;

	/* maps are just arrays, so pretend we have one and reuse decode_array_items */
	arr = (struct cbor_item) {
		.type = CBOR_TYPE_ARRAY,
		.flags = map->flags,
		.u64 = 2 * map->u64,
	};

	if ((err = decode_array_items(cs, &arr, &items, &nitems, true)) != NB_ERR_OK)
		return err;

	if (nitems % 2 != 0)
		return error(cs, NB_ERR_NITEMS, "Odd number of items in a map.");
	*npairs = nitems / 2;

	*pairs = ((struct cbor_pair *)items);

	return NB_ERR_OK;
}


nb_err_t cbor_decode_item(struct cbor_stream *cs, struct cbor_item *item)
{
	nb_err_t err;

	if ((err = cbor_peek(cs, item)) != NB_ERR_OK)
		return err;

	switch (item->type) {
	case CBOR_TYPE_UINT:
		return cbor_decode_uint64(cs, &item->u64);
	case CBOR_TYPE_SVAL:
		return cbor_decode_sval(cs, &item->sval);
	case CBOR_TYPE_INT:
		return cbor_decode_int64(cs, &item->i64);
	case CBOR_TYPE_FLOAT16:
	case CBOR_TYPE_FLOAT32:
	case CBOR_TYPE_FLOAT64:
		return error(cs, NB_ERR_UNSUP, "Float decoding not supported."); /* TODO redundant error()? */

	case CBOR_TYPE_BYTES:
		return cbor_decode_bytes(cs, &item->bytes, &item->len);
	case CBOR_TYPE_TEXT:
		return decode_text(cs, &item->str, &item->len);
	case CBOR_TYPE_ARRAY:
		if (!is_indefinite(item))
			err = cbor_decode_array_begin(cs, &item->len);
		else
			err = cbor_decode_array_begin_indef(cs);

		if (err != NB_ERR_OK)
			return err;

		if ((err = decode_array_items(cs, item, &item->items, &item->len, false)))
			return err;

		return cbor_decode_array_end(cs);

	case CBOR_TYPE_MAP:
		if (!is_indefinite(item))
			err = cbor_decode_map_begin(cs, &item->len);
		else
			err = cbor_decode_map_begin_indef(cs);

		if (err != NB_ERR_OK)
			return err;

		if ((err = decode_map_items(cs, item, &item->pairs, &item->len)))
			return err;

		return cbor_decode_map_end(cs);

	case CBOR_TYPE_TAG:
		cbor_decode_tag(cs, &item->tag);
		item->tagged_item = nb_malloc(sizeof(*item->tagged_item));
		if (!item->tagged_item)
			return NB_ERR_NOMEM;
		return cbor_decode_item(cs, item->tagged_item);

	default:
		return error(cs, NB_ERR_UNSUP, NULL);
	}
}
