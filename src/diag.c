#include "cbor.h"
#include "debug.h"
#include "diag.h"
#include "internal.h"
#include "util.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BYTES_DUMP_MAXLEN	64


void diag_log_offset(struct diag *diag, size_t offset)
{
	snprintf(diag->output.offset, sizeof(diag->output.offset), "0x%zx", offset);
}


void diag_log_raw(struct diag *diag, byte_t *bytes, size_t count)
{
	size_t i;
	for (i = 0; i < count; i++)
		strbuf_printf(&diag->output.raw, "%02X", bytes[i]);
	diag->have_output = true;
}


void diag_log_item(struct diag *diag, char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	strbuf_vprintf(&diag->output.item, msg, args);
	diag->have_output = true;
	va_end(args);
}


void diag_log_cbor(struct diag *diag, char *msg, ...)
{
	va_list args;
	size_t i;

	if (diag->indent_next) {
		for (i = 0; i < 4 * diag->level; i++)
			strbuf_putc(&diag->output.cbor, ' ');
		diag->indent_next = false;
	}

	va_start(args, msg);
	strbuf_vprintf(&diag->output.cbor, msg, args);
	diag->have_output = true;
	va_end(args);
}


void diag_log_proto(struct diag *diag, char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	strbuf_vprintf(&diag->output.proto, msg, args);
	diag->have_output = true;
	va_end(args);
}


void diag_increase(struct diag *diag)
{
	diag->level++;
}


void diag_decrease(struct diag *diag)
{
	assert(diag->level > 0);
	diag->level--;
}


void diag_dump_line(struct diag *diag)
{
	if (!diag->have_output)
		return;
	diag->have_output = false;

	fprintf(diag->fout, "%-16s %-20s %s\n",
		//diag->output.offset,
		strbuf_get_string(&diag->output.raw),
		strbuf_get_string(&diag->output.item),
		strbuf_get_string(&diag->output.cbor));
		//strbuf_get_string(&diag->output.proto));

	diag->output.offset[0] = '\0';
	strbuf_reset(&diag->output.raw);
	strbuf_reset(&diag->output.item);
	strbuf_reset(&diag->output.cbor);
	strbuf_reset(&diag->output.proto);
	
	diag->indent_next = true;
}


void diag_log_sval(struct diag *diag, uint64_t sval)
{
	switch (sval) {
	case CBOR_SVAL_FALSE:
		diag_log_cbor(diag, "false");
		break;
	case CBOR_SVAL_TRUE:
		diag_log_cbor(diag, "true");
		break;
	case CBOR_SVAL_NULL:
		diag_log_cbor(diag, "null");
		break;
	case CBOR_SVAL_UNDEF:
		diag_log_cbor(diag, "undefined");
		break;
	default:
		diag_log_cbor(diag, "simple(%lu)", sval);
	}
}


static void dump_char_printable(struct diag *diag, char c)
{
	switch (c) {
	case '\n':
		diag_log_cbor(diag, "\\n");
		return;
	case '\t':
		diag_log_cbor(diag, "\\t");
		return;
	case '\r':
		diag_log_cbor(diag, "\\r");
		return;
	case '\"':
		diag_log_cbor(diag, "\\\"");
		return;
	case '\\':
		diag_log_cbor(diag, "\\\\");
		return;
	default:
		if (isprint(c) || true)
			diag_log_cbor(diag, "%c", c);
		else
			diag_log_cbor(diag, "\\x%02u", c);
	}
}


static nb_err_t dump_text(struct diag *diag, struct cbor_item *stream)
{
	size_t i;
	byte_t *bytes;
	size_t nbytes;
	size_t dump_len;
	nb_err_t err;

	if ((err = cbor_decode_stream0(diag->cs, stream, &bytes, &nbytes)) != NB_ERR_OK)
		return err;

	dump_len = nbytes;
	if (diag->str_dump_maxlen > 0 && diag->str_dump_maxlen < dump_len)
		dump_len = diag->str_dump_maxlen;

	diag_log_cbor(diag, "\"");
	for (i = 0; i < dump_len; i++)
		dump_char_printable(diag, bytes[i]);
	if (nbytes > dump_len)
		diag_log_cbor(diag, "...");
	diag_log_cbor(diag, "\"");

	return NB_ERR_OK;
}


static nb_err_t dump_bytes(struct diag *diag, struct cbor_item *stream)
{
	size_t i;
	byte_t *bytes;
	size_t nbytes;
	size_t dump_len;
	nb_err_t err;

	if ((err = cbor_decode_stream(diag->cs, stream, &bytes, &nbytes)) != NB_ERR_OK)
		return err;

	dump_len = nbytes;
	if (diag->bytes_dump_maxlen > 0 && diag->str_dump_maxlen < dump_len)
		dump_len = diag->bytes_dump_maxlen;

	diag_log_cbor(diag, "h'");
	for (i = 0; i < dump_len; i++)
		diag_log_cbor(diag, "%02X", bytes[i]);
	if (nbytes > dump_len)
		diag_log_cbor(diag, "...");
	diag_log_cbor(diag, "'");

	return NB_ERR_OK;
}


static void dump_sval(struct diag *diag, struct cbor_item *sval)
{
	switch (sval->u64) {
	case CBOR_SVAL_FALSE:
		diag_log_cbor(diag, "false");
		break;
	case CBOR_SVAL_TRUE:
		diag_log_cbor(diag, "true");
		break;
	case CBOR_SVAL_NULL:
		diag_log_cbor(diag, "null");
		break;
	case CBOR_SVAL_UNDEF:
		diag_log_cbor(diag, "undefined");
		break;
	default:
		diag_log_cbor(diag, "simple(%lu)", sval->u64);
	}
}


static void dump_item(struct diag *diag, struct cbor_item *item);


static void dump_array(struct diag *diag, struct cbor_item *arr)
{
	struct cbor_item item;
	size_t num_items = 0;
	size_t i = 0;
	nb_err_t err = NB_ERR_OK;

	diag_log_cbor(diag, "[");
	diag_increase(diag);

	if (arr->indefinite)
		cbor_decode_array_begin_indef(diag->cs);
	else
		cbor_decode_array_begin(diag->cs, &num_items);

	while (arr->indefinite || (!arr->indefinite && i < num_items)) {
		if ((err = cbor_peek_item(diag->cs, &item)) != NB_ERR_OK)
			break;

		if (i == 0)
			diag_log_cbor(diag, "");
		else
			diag_log_cbor(diag, ",");

		dump_item(diag, &item);
		i++;
	}

	cbor_decode_array_end(diag->cs);

	//if (i > 0)
		diag_log_cbor(diag, "");
	diag_decrease(diag);
	diag_log_cbor(diag, "]");
}


static void dump_map(struct diag *diag, struct cbor_item *map)
{
	struct cbor_item item;
	size_t i = 0;
	bool is_key;
	nb_err_t err = NB_ERR_OK;
	size_t num_pairs = 0;

	diag_log_cbor(diag, "{");
	diag_increase(diag);

	if (map->indefinite)
		cbor_decode_map_begin_indef(diag->cs);
	else
		cbor_decode_map_begin(diag->cs, &num_pairs);

	is_key = true;
	while (map->indefinite || (!map->indefinite && i < 2 * num_pairs)) {
		if ((err = cbor_peek_item(diag->cs, &item)) != NB_ERR_OK)
			break;

		if (is_key) {
			dump_item(diag, &item);
			diag_log_cbor(diag, ":");
		}
		else {
			diag_increase(diag);
			dump_item(diag, &item);
			diag_log_cbor(diag, ",");
			diag_decrease(diag);
		}

		is_key = !is_key;
		i++;
	}

	cbor_decode_map_end(diag->cs);

	diag_decrease(diag);
	diag_dump_line(diag);
	diag_log_cbor(diag, "}");
}


static void dump_item(struct diag *diag, struct cbor_item *item)
{
	struct cbor_item skipped;

	switch (item->type) {
	case CBOR_TYPE_UINT:
		cbor_decode_item(diag->cs, &skipped);
		diag_log_item(diag, "uint(%lu)", item->u64);
		diag_log_cbor(diag, "%lu", item->u64);
		break;
	case CBOR_TYPE_INT:
		cbor_decode_item(diag->cs, &skipped);
		diag_log_item(diag, "int(%li)", item->i64);
		diag_log_cbor(diag, "%li", item->i64);
		break;
	case CBOR_TYPE_BYTES:
		diag_log_item(diag, "bytestream(%luB)\t", item->u64);
		cbor_skip_header(diag->cs);
		dump_bytes(diag, item);
		break;
	case CBOR_TYPE_TEXT:
		diag_log_item(diag, "string(%luB)", item->u64);
		cbor_skip_header(diag->cs);
		dump_text(diag, item);
		break;
	case CBOR_TYPE_ARRAY:
		diag_log_item(diag, "array(num_items=%lu)", item->u64);
		dump_array(diag, item);
		break;
	case CBOR_TYPE_MAP:
		diag_log_item(diag, "map(num_pairs=%lu)", item->u64);
		dump_map(diag, item);
		break;
	case CBOR_TYPE_TAG:
		cbor_decode_item(diag->cs, &skipped);
		diag_log_item(diag, "tag(%lu)", item->u64);
		diag_log_cbor(diag, "(tag=%lu)", item->u64); /* TODO */
		break;
	case CBOR_TYPE_SVAL:
		diag_log_item(diag, "simple(%lu)", item->u64);
		cbor_decode_item(diag->cs, &skipped);
		dump_sval(diag, item);
		break;
	default:
		break;
	}
}


void diag_init(struct diag *diag, struct cbor_stream *cs, FILE *fout)
{
	diag->output.offset[0] = '\0';
	strbuf_init(&diag->output.raw, 8);
	strbuf_init(&diag->output.item, 16);
	strbuf_init(&diag->output.cbor, 32);
	strbuf_init(&diag->output.proto, 24);

	diag->level = 0;
	diag->indent_next = true;
	diag->cs = cs;
	diag->fout = fout;

	diag->str_dump_maxlen = 16;
	diag->bytes_dump_maxlen = 16;
}


nb_err_t diag_dump(struct diag *diag, FILE *out)
{
	struct cbor_item item;
	nb_err_t err = NB_ERR_OK;
	size_t i;

	for (i = 0; !nb_buf_is_eof(diag->cs->buf); i++) {
		if (i > 0) {
			diag_log_cbor(diag, ",");
			diag_dump_line(diag);
		}

		if ((err = cbor_decode_item(diag->cs, &item)) != NB_ERR_OK)
			break;
	}

	if (i > 0)
		diag_dump_line(diag);

	return err;
}


void diag_free(struct diag *diag)
{
	strbuf_free(&diag->output.raw);
	strbuf_free(&diag->output.item);
	strbuf_free(&diag->output.cbor);
	strbuf_free(&diag->output.proto);
}
