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


static inline void read_stream(struct cbor_stream *cs, nb_byte_t *bytes, size_t nbytes)
{
	diag_log_offset(cs->diag, nb_buffer_tell(cs->buf));
	if (nb_buffer_read(cs->buf, bytes, nbytes) != nbytes)
		error(cs, NB_ERR_EOF, "EOF was unexpected.");
	diag_log_raw(cs->diag, bytes, MIN(nbytes, 4));
}


static uint64_t decode_u64(struct cbor_stream *cs, enum lbits lbits)
{
	assert(lbits != LBITS_INDEFINITE);

	nb_byte_t bytes[8];
	uint8_t nbytes;
	uint64_t u64be = 0;
	nb_byte_t *u64be_ptr = (nb_byte_t *)&u64be;
	size_t i;

	if (lbits <= 23)
		return lbits;

	if (lbits > LBITS_8B)
		error(cs, NB_ERR_PARSE,
			"Invalid value of Additional Information: 0x%02X.", lbits);

	nbytes = lbits_to_nbytes(lbits);
	read_stream(cs, bytes, nbytes);

	for (i = 0; i < nbytes; i++)
		u64be_ptr[(8 - nbytes) + i] = bytes[i]; /* TODO optimize as in encoder */

	return be64toh(u64be);
}


/* TODO have a look at the range checks and make sure nothing overflows */
static inline int64_t u64_to_i64(struct cbor_stream *cs, uint64_t u64)
{
	if (u64 <= INT64_MIN_ABS)
		return -u64 - 1;

	error(cs, NB_ERR_RANGE, "Negative integer -%lu less than -%lu cannot be decoded.",
		u64, INT64_MIN_ABS);
	return 0; /* TODO */
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


static void decode_item_major7(struct cbor_stream *cs, struct cbor_item *item,
	enum minor minor)
{
	uint64_t u64 = 0;

	assert(minor >= 0);
	assert(minor != CBOR_MINOR_BREAK);

	if (minor <= 23) {
		item->type = CBOR_TYPE_SVAL;
		item->sval = minor;
		item->u64 = item->sval;
		print_sval_diag(cs, item->sval);
		return;
	}

	/* leverage decode_u64 to read the appropriate amount of data */
	u64 = decode_u64(cs, (enum lbits)minor);

	switch (minor) {
	case CBOR_MINOR_SVAL:
		item->type = CBOR_TYPE_SVAL; /* deduplicate */
		item->sval = u64;
		item->u64 = item->sval; /* TODO */
		print_sval_diag(cs, item->sval);
		return;
	case CBOR_MINOR_FLOAT16:
	case CBOR_MINOR_FLOAT32:
	case CBOR_MINOR_FLOAT64:
		error(cs, NB_ERR_UNSUP, "Float decoding not supported.");
	default:
		break;
	}

	error(cs, NB_ERR_PARSE, "Invalid Additional Information "
		"value for Major Type 7: 0x%02X.", minor);
}


static void predecode(struct cbor_stream *cs, struct cbor_item *item)
{
	nb_byte_t hdr;
	enum major major;
	nb_byte_t lbits;
	uint64_t u64 = 0;

	if (__builtin_expect(cs->peeking, 0)) {
		*item = cs->peek;
		cs->peeking = false;
		return;
	}

	read_stream(cs, &hdr, 1);
	if (hdr == CBOR_BREAK) {
		item->type = CBOR_TYPE_BREAK;
		return;
	}

	major = (hdr & MAJOR_MASK) >> 5;
	lbits = hdr & LBITS_MASK;
	assert(major >= 0 && major <= 7);

	diag_comma(cs->diag); /* append a comma to previous line if needed */
	item->flags = 0;

	top_block(cs)->num_items++;

	if (major == CBOR_MAJOR_UINT && lbits <= 23) {
		item->type = CBOR_TYPE_UINT;
		u64 = lbits;
	}
	else {
		if (major == CBOR_MAJOR_7) {
			decode_item_major7(cs, item, lbits);
			return;
		}

		item->type = (enum cbor_type)major;

		if (lbits == LBITS_INDEFINITE)
			item->flags |= CBOR_FLAG_INDEFINITE;

		if (!is_indefinite(item)) {
			u64 = decode_u64(cs, lbits);
		}
		else {
			if (!major_allows_indefinite(major))
				error(cs, NB_ERR_INDEF, "Indefinite-length encoding "
					"is not allowed for %s items.", cbor_type_string(major));
			diag_log_item(cs->diag, "%s(_)", cbor_type_string(item->type));
			return;
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
		item->i64 = u64_to_i64(cs, u64);
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
		error(cs, NB_ERR_PARSE, "Invalid major type: 0x%02X.", major);
	}
}


void cbor_peek(struct cbor_stream *cs, struct cbor_item *item)
{
	if (!cs->peeking)
		predecode(cs, &cs->peek);
	cs->peeking = true;
	*item = cs->peek; /* TODO avoid the copy */
}


bool cbor_is_break(struct cbor_stream *cs)
{
	struct cbor_item item;
	cbor_peek(cs, &item);
	return item.type == CBOR_TYPE_BREAK;
}


static void decode_break(struct cbor_stream *cs)
{
	struct cbor_item item;
	predecode(cs, &item);
	if (item.type != CBOR_TYPE_BREAK)
		error(cs, NB_ERR_BREAK, "Break was expected");
}


static inline void predecode_check(struct cbor_stream *cs, struct cbor_item *item,
	enum cbor_type type)
{
	predecode(cs, item);
	if (item->type != type)
		error(cs, NB_ERR_ITEM, "%s was unexpected, %s was expected.",
			cbor_type_string(item->type), cbor_type_string(type));
}


static uint64_t decode_uint(struct cbor_stream *cs, uint64_t max)
{
	struct cbor_item item;

	predecode(cs, &item);
	if (item.u64 > max)
		error(cs, NB_ERR_RANGE, "Expected unsigned integer less than or equal "
			"to %lu, %lu was decoded", max, item.u64);
	return item.u64;
}


/* TODO have a look at the range checks and make sure nothing overflows */
static int64_t decode_int(struct cbor_stream *cs, int64_t min, int64_t max)
{
	struct cbor_item item;
	int64_t i64;

	predecode(cs, &item);

	if (item.type == CBOR_TYPE_INT) {
		i64 = item.i64;
	}
	else if (item.type == CBOR_TYPE_UINT) {
		if (item.u64 > INT64_MAX) {
			error(cs, NB_ERR_RANGE, "Expected unsigned or negative integer "
				"less than or equal to %lu, %lu was decoded",
				INT64_MAX, item.u64);
			return 0;
		}
		i64 = (int64_t)item.u64;
	}
	else {
		error(cs, NB_ERR_ITEM, "%s was unexpected, unsigned or negative "
			"integer was expected", cbor_type_string(item.type));
		return 0;
	}

	if (i64 >= min && i64 <= max)
		return i64;

	error(cs, NB_ERR_RANGE, "Expected unsigned or negative integer in the range "
		"from %li to %lu, %lu was decoded", min, max, i64);
	return 0;
}


void cbor_decode_uint8(struct cbor_stream *cs, uint8_t *val)
{
	*val = (uint8_t)decode_uint(cs, UINT8_MAX);
}


void cbor_decode_uint16(struct cbor_stream *cs, uint16_t *val)
{
	*val = (uint16_t)decode_uint(cs, UINT16_MAX);
}


void cbor_decode_uint32(struct cbor_stream *cs, uint32_t *val)
{
	*val = (uint32_t)decode_uint(cs, UINT32_MAX);
}


void cbor_decode_uint64(struct cbor_stream *cs, uint64_t *val)
{
	*val = (uint64_t)decode_uint(cs, UINT64_MAX);
}


void cbor_decode_int8(struct cbor_stream *cs, int8_t *val)
{
	*val = (int8_t)decode_int(cs, INT8_MIN, INT8_MAX);
}


void cbor_decode_int16(struct cbor_stream *cs, int16_t *val)
{
	*val = (int16_t)decode_int(cs, INT16_MIN, INT16_MAX);
}


void cbor_decode_int32(struct cbor_stream *cs, int32_t *val)
{
	*val = (int32_t)decode_int(cs, INT32_MIN, INT32_MAX);
}


void cbor_decode_int64(struct cbor_stream *cs, int64_t *val)
{
	*val = (int64_t)decode_int(cs, INT64_MIN, INT64_MAX);
}


void cbor_decode_tag(struct cbor_stream *cs, uint32_t *tag)
{
	struct cbor_item item;

	predecode_check(cs, &item, CBOR_TYPE_TAG);
	if (item.u64 > UINT32_MAX)
		error(cs, NB_ERR_RANGE,
			"This implementation only supports uint32 tags in the range "
			"0...%lu; %lu was decoded", UINT32_MAX, item.u64);

	*tag = (uint32_t)item.u64;
}


void cbor_decode_sval(struct cbor_stream *cs, enum cbor_sval *sval)
{
	struct cbor_item item;
	predecode_check(cs, &item, CBOR_TYPE_SVAL);
	*sval = item.u64;
}


void cbor_decode_bool(struct cbor_stream *cs, bool *b)
{
	enum cbor_sval sval;
	cbor_decode_sval(cs, &sval);
	if (sval != CBOR_SVAL_TRUE && sval != CBOR_SVAL_FALSE)
		error(cs, NB_ERR_RANGE, "expected bool, but the decoded simple "
			"value was neither true nor false");
	*b = (sval == CBOR_SVAL_TRUE);
}


static void decode_block_start(struct cbor_stream *cs, enum cbor_type type,
	bool indef, uint64_t *len)
{
	struct cbor_item item;
	size_t block_len = 0;

	predecode_check(cs, &item, type);
	if (is_indefinite(&item) != indef)
		error(cs, NB_ERR_INDEF, NULL);

	*len = item.u64;
	block_len = item.u64;
	if (type == CBOR_TYPE_MAP)
		block_len *= 2;

	push_block(cs, item.type, is_indefinite(&item), block_len);
}


static void decode_block_end(struct cbor_stream *cs, enum cbor_type type)
{
	struct block *block;

	if (cbor_block_stack_empty(cs))
		error(cs, NB_ERR_OPER, "Cannot end block, there's no block open.");

	block = stack_top(&cs->blocks);
	if (block->type != type)
		error(cs, NB_ERR_OPER, "Cannot end block of type %s when block of %s "
			"is open", cbor_type_string(block->type), cbor_type_string(type));

	if (block->indefinite)
		decode_break(cs);

	else if (block->num_items != block->len) {
		error(cs, NB_ERR_OPER,
			"Cannot end %s: designated size is %lu, but %lu items were decoded",
			cbor_type_string(block->type), block->len, block->num_items);
	}

	(void) stack_pop(&cs->blocks);
}


void cbor_decode_array_begin(struct cbor_stream *cs, uint64_t *len)
{
	decode_block_start(cs, CBOR_TYPE_ARRAY, false, len);

	diag_log_cbor(cs->diag, "[");
	diag_eol(cs->diag, false);
	diag_indent_cbor(cs->diag);
}


void cbor_decode_array_begin_indef(struct cbor_stream *cs)
{
	uint64_t foo;

	decode_block_start(cs, CBOR_TYPE_ARRAY, true, &foo);

	diag_log_cbor(cs->diag, cs->diag->print_json ? "[" : "[_");
	diag_eol(cs->diag, false);
	diag_indent_cbor(cs->diag);
}


void cbor_decode_array_end(struct cbor_stream *cs)
{
	bool indefinite;

	indefinite = top_block(cs)->indefinite;
	decode_block_end(cs, CBOR_TYPE_ARRAY);

	if (!indefinite)
		diag_force_newline(cs->diag); /* TODO cleanup */

	diag_dedent_cbor(cs->diag);
	diag_log_cbor(cs->diag, "]");
	diag_finish_item(cs);
}


void cbor_decode_map_begin(struct cbor_stream *cs, uint64_t *len)
{
	decode_block_start(cs, CBOR_TYPE_MAP, false, len);

	diag_log_cbor(cs->diag, "{");
	diag_eol(cs->diag, false);
	diag_indent_cbor(cs->diag);
}


void cbor_decode_map_begin_indef(struct cbor_stream *cs)
{
	uint64_t foo;

	decode_block_start(cs, CBOR_TYPE_MAP, true, &foo);

	diag_log_cbor(cs->diag, cs->diag->print_json ? "{" : "{_");
	diag_eol(cs->diag, false);
	diag_indent_cbor(cs->diag);
}


void cbor_decode_map_end(struct cbor_stream *cs)
{
	bool indefinite;

	indefinite = top_block(cs)->indefinite;
	decode_block_end(cs, CBOR_TYPE_MAP);

	if (!indefinite)
		diag_force_newline(cs->diag); /* TODO cleanup */

	diag_dedent_cbor(cs->diag);
	diag_log_cbor(cs->diag, "}");
	diag_finish_item(cs);
}


static void read_stream_chunk(struct cbor_stream *cs, struct cbor_item *stream,
	struct cbor_item *chunk, nb_byte_t **bytes, size_t *len)
{
	if (chunk->u64 > SIZE_MAX)
		error(cs, NB_ERR_RANGE,
			"Chunk is too large to be decoded by this implementation.");

	*bytes = nb_realloc(*bytes, 1 + *len + chunk->u64);

	read_stream(cs, *bytes + *len, chunk->u64);
	/* TODO free(*bytes); */

	(*bytes)[*len + chunk->u64] = 0;
	*len += chunk->u64;
}


void cbor_decode_stream(struct cbor_stream *cs, struct cbor_item *stream,
	nb_byte_t **bytes, size_t *len)
{
	struct cbor_item chunk;

	*len = 0;
	*bytes = NULL;

	if (!is_indefinite(stream)) {
		read_stream_chunk(cs, stream, stream, bytes, len);
		return;
	}

	while (!cbor_is_break(cs)) {
		predecode_check(cs, &chunk, stream->type);
		if (!is_indefinite(&chunk))
			read_stream_chunk(cs, stream, &chunk, bytes, len);
		else
			error(cs, NB_ERR_INDEF, "Indefinite-length streams cannot "
				"contain indefinite-length chunks");
	}
}


void cbor_decode_stream0(struct cbor_stream *cs, struct cbor_item *stream,
	nb_byte_t **bytes, size_t *len)
{
	cbor_decode_stream(cs, stream, bytes, len);
	(*bytes)[*len] = 0;
}


void cbor_decode_bytes(struct cbor_stream *cs, nb_byte_t **str, size_t *len)
{
	struct cbor_item item;

	predecode_check(cs, &item, CBOR_TYPE_BYTES);
	cbor_decode_stream(cs, &item, str, len);

	diag_log_cbor(cs->diag, "\"%s\"", *str);
	diag_finish_item(cs);
}


static void decode_text(struct cbor_stream *cs, char **str, size_t *len)
{
	struct cbor_item item;

	predecode_check(cs, &item, CBOR_TYPE_TEXT);
	cbor_decode_stream0(cs, &item, (nb_byte_t **)str, len);

	diag_log_cbor(cs->diag, "\"%s\"", *str);
	diag_finish_item(cs);
}


void cbor_decode_text(struct cbor_stream *cs, char **str)
{
	size_t unused;
	decode_text(cs, str, &unused);
	(void) unused;
}


static void decode_array_items(struct cbor_stream *cs, struct cbor_item *arr,
	struct cbor_item **items, uint64_t *nitems)
{
	struct cbor_item *item;
	size_t size;
	size_t i;

	*items = NULL;
	size = is_indefinite(arr) ? CBOR_ARRAY_INIT_SIZE : arr->u64;

	i = 0;
	do {
		*items = nb_realloc(*items, size * sizeof(**items));

		while (i < size) {
			item = *items + i;
			if (cbor_is_break(cs))
				goto out;
			cbor_decode_item(cs, item);
			i++;
		}

		size *= 2;
	} while (is_indefinite(arr));
out:
	*nitems = i;
}


static void decode_map_items(struct cbor_stream *cs, struct cbor_item *map,
	struct cbor_pair **pairs, uint64_t *npairs)
{
	struct cbor_item arr;
	struct cbor_item *items;
	uint64_t nitems;

	/* maps are just arrays, so pretend we have one and reuse decode_array_items */
	arr = (struct cbor_item) {
		.type = CBOR_TYPE_ARRAY,
		.flags = map->flags,
		.u64 = 2 * map->u64,
	};

	decode_array_items(cs, &arr, &items, &nitems);

	if (nitems % 2 != 0)
		error(cs, NB_ERR_NITEMS, "Odd number of items in a map.");
	*npairs = nitems / 2;
	*pairs = ((struct cbor_pair *)items);
}


void cbor_decode_item(struct cbor_stream *cs, struct cbor_item *item)
{
	cbor_peek(cs, item);

	switch (item->type) {
	case CBOR_TYPE_UINT:
		cbor_decode_uint64(cs, &item->u64);
		break;
	case CBOR_TYPE_SVAL:
		cbor_decode_sval(cs, &item->sval);
		break;
	case CBOR_TYPE_INT:
		cbor_decode_int64(cs, &item->i64);
		break;
	case CBOR_TYPE_BYTES:
		cbor_decode_bytes(cs, &item->bytes, &item->len);
		break;
	case CBOR_TYPE_TEXT:
		decode_text(cs, &item->str, &item->len);
		break;
	case CBOR_TYPE_ARRAY:
		if (!is_indefinite(item))
			cbor_decode_array_begin(cs, &item->len);
		else
			cbor_decode_array_begin_indef(cs);
		decode_array_items(cs, item, &item->items, &item->len);
		cbor_decode_array_end(cs);
		break;

	case CBOR_TYPE_MAP:
		if (!is_indefinite(item))
			cbor_decode_map_begin(cs, &item->len);
		else
			cbor_decode_map_begin_indef(cs);

		decode_map_items(cs, item, &item->pairs, &item->len);
		cbor_decode_map_end(cs);
		break;

	case CBOR_TYPE_TAG:
		cbor_decode_tag(cs, &item->tag);
		item->tagged_item = nb_malloc(sizeof(*item->tagged_item));
		cbor_decode_item(cs, item->tagged_item);
		break;

	default:
		error(cs, NB_ERR_UNSUP, "Decoding of %s data type is not supported",
			cbor_type_string(item->type));
	}
}
