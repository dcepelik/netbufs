#include "cbor.h"
#include "diag.h"
#include "debug.h"
#include "internal.h"
#include "util.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define BYTES_DUMP_MAXLEN	64


static void diag_vprintf(struct diag *diag, char *msg, va_list args)
{
	size_t i;
	if (diag->indent_next) {
		for (i = 0; i < 4 * diag->level; i++)
			strbuf_putc(&diag->strbuf, ' ');
		diag->indent_next = false;
	}
	strbuf_vprintf_at(&diag->strbuf, diag->strbuf.len, msg, args);
}


static void diag_printf(struct diag *diag, char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	diag_vprintf(diag, msg, args);
	va_end(args);
}


static void diag_printfln(struct diag *diag, char *msg, ...)
{
	va_list args;

	va_start(args, msg);
	diag_vprintf(diag, msg, args);
	strbuf_putc(&diag->strbuf, '\n');
	diag->indent_next = true;
	va_end(args);
}


static void diag_increase(struct diag *diag)
{
	diag->level++;
}


static void diag_decrease(struct diag *diag)
{
	assert(diag->level > 0);
	diag->level--;
}


static void dump_char_printable(struct diag *diag, char c)
{
	switch (c) {
	case '\n':
		diag_printf(diag, "\\n");
		return;
	case '\t':
		diag_printf(diag, "\\t");
		return;
	case '\r':
		diag_printf(diag, "\\r");
		return;
	case '\"':
		diag_printf(diag, "\\\"");
		return;
	case '\\':
		diag_printf(diag, "\\\\");
		return;
	default:
		if (isprint(c) || true)
			diag_printf(diag, "%c", c);
		else
			diag_printf(diag, "\\x%02u", c);
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

	diag_printf(diag, "\"");
	for (i = 0; i < dump_len; i++)
		dump_char_printable(diag, bytes[i]);
	if (nbytes > dump_len)
		diag_printf(diag, "...");
	diag_printf(diag, "\"");

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

	diag_printf(diag, "h'");
	for (i = 0; i < dump_len; i++)
		diag_printf(diag, "%02X", bytes[i]);
	if (nbytes > dump_len)
		diag_printf(diag, "...");
	diag_printf(diag, "'");

	return NB_ERR_OK;
}


static void dump_sval(struct diag *diag, struct cbor_item *sval)
{
	switch (sval->u64) {
	case CBOR_SVAL_FALSE:
		diag_printf(diag, "false");
		break;
	case CBOR_SVAL_TRUE:
		diag_printf(diag, "true");
		break;
	case CBOR_SVAL_NULL:
		diag_printf(diag, "null");
		break;
	case CBOR_SVAL_UNDEF:
		diag_printf(diag, "undefined");
		break;
	default:
		diag_printf(diag, "simple(%lu)", sval->u64);
	}
}


static void dump_item(struct diag *diag, struct cbor_item *item);


static void dump_array(struct diag *diag, struct cbor_item *arr)
{
	struct cbor_item item;
	size_t num_items = 0;
	size_t i = 0;
	nb_err_t err = NB_ERR_OK;

	diag_printf(diag, "[");
	diag_increase(diag);

	if (arr->indefinite)
		cbor_decode_array_begin_indef(diag->cs);
	else
		cbor_decode_array_begin(diag->cs, &num_items);

	while (arr->indefinite || (!arr->indefinite && i < num_items)) {
		if ((err = cbor_peek_item(diag->cs, &item)) != NB_ERR_OK)
			break;

		if (i == 0)
			diag_printfln(diag, "");
		else
			diag_printfln(diag, ",");

		dump_item(diag, &item);
		i++;
	}

	cbor_decode_array_end(diag->cs);

	//if (i > 0)
		diag_printfln(diag, "");
	diag_decrease(diag);
	diag_printf(diag, "]");
}


static void dump_map(struct diag *diag, struct cbor_item *map)
{
	struct cbor_item item;
	size_t i = 0;
	bool is_key;
	nb_err_t err = NB_ERR_OK;
	size_t num_pairs = 0;

	diag_printf(diag, "{");
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
			if (i == 0)
				diag_printfln(diag, "");
			else
				diag_printfln(diag, ",");

			dump_item(diag, &item);
			diag_printfln(diag, ":");
		}
		else {
			diag_increase(diag);
			dump_item(diag, &item);
			diag_decrease(diag);
		}

		is_key = !is_key;
		i++;
	}

	cbor_decode_map_end(diag->cs);

	if (i > 0)
		diag_printfln(diag, "");
	diag_decrease(diag);
	diag_printf(diag, "}");
}


static void dump_item(struct diag *diag, struct cbor_item *item)
{
	struct cbor_item skipped;

	switch (item->type) {
	case CBOR_TYPE_UINT:
		cbor_decode_item(diag->cs, &skipped);
		diag_printf(diag, "%lu", item->u64);
		break;
	case CBOR_TYPE_INT:
		cbor_decode_item(diag->cs, &skipped);
		diag_printf(diag, "%li", item->i64);
		break;
	case CBOR_TYPE_BYTES:
		cbor_skip_header(diag->cs);
		dump_bytes(diag, item);
		break;
	case CBOR_TYPE_TEXT:
		cbor_skip_header(diag->cs);
		dump_text(diag, item);
		break;
	case CBOR_TYPE_ARRAY:
		dump_array(diag, item);
		break;
	case CBOR_TYPE_MAP:
		dump_map(diag, item);
		break;
	case CBOR_TYPE_TAG:
		cbor_decode_item(diag->cs, &skipped);
		diag_printf(diag, "(tag=%lu)", item->u64); /* TODO */
		break;
	case CBOR_TYPE_SVAL:
		cbor_decode_item(diag->cs, &skipped);
		dump_sval(diag, item);
		break;
	default:
		break;
	}
}


void diag_init(struct diag *diag, struct cbor_stream *cs)
{
	strbuf_init(&diag->strbuf, 64);
	diag->level = 0;
	diag->indent_next = true;
	diag->cs = cs;

	diag->str_dump_maxlen = 16;
	diag->bytes_dump_maxlen = 16;
}


nb_err_t diag_dump(struct diag *diag, FILE *out)
{
	struct cbor_item item;
	nb_err_t err = NB_ERR_OK;
	size_t i;

	for (i = 0; !nb_buf_is_eof(diag->cs->buf); i++) {
		if ((err = cbor_peek_item(diag->cs, &item)) != NB_ERR_OK)
			break;

		if (i > 0)
			diag_printfln(diag, ",");
		dump_item(diag, &item);
	}

	if (i > 0)
		diag_printfln(diag, "");

	fputs(strbuf_get_string(&diag->strbuf), out);
	return err;
}


void diag_free(struct diag *diag)
{
	strbuf_free(&diag->strbuf);
}
