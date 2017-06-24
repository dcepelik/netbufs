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
		diag_printf(diag, "%c", c);
	}
}


static nb_err_t dump_text(struct diag *diag, struct cbor_item *stream)
{
	size_t i;
	byte_t *bytes;
	size_t bytes_len;
	size_t dump_len;
	nb_err_t err;

	if ((err = cbor_decode_stream0(diag->cs, stream, &bytes, &bytes_len)) != NB_ERR_OK)
		return err;
	dump_len = MIN(bytes_len, BYTES_DUMP_MAXLEN);

	diag_printf(diag, "\"");
	for (i = 0; i < dump_len; i++)
		dump_char_printable(diag, bytes[i]);
	if (bytes_len > dump_len)
		diag_printf(diag, "...");
	diag_printf(diag, "\"");

	return NB_ERR_OK;
}


static nb_err_t dump_bytes(struct diag *diag, struct cbor_item *stream)
{
	size_t i;
	byte_t *bytes;
	size_t bytes_len;
	size_t dump_len;
	nb_err_t err;

	if ((err = cbor_decode_stream(diag->cs, stream, &bytes, &bytes_len)) != NB_ERR_OK)
		return err;
	dump_len = MIN(bytes_len, BYTES_DUMP_MAXLEN);

	diag_printf(diag, "h'");
	for (i = 0; i < dump_len; i++)
		diag_printf(diag, "%02X", bytes[i]);
	if (bytes_len > dump_len)
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


static void dump_stream(struct diag *diag, struct cbor_item *item)
{

}


static void dump_item(struct diag *diag, struct cbor_item *item);


static void dump_array(struct diag *diag, struct cbor_item *arr)
{
	struct cbor_item item;
	size_t i = 0;
	nb_err_t err = NB_ERR_OK;

	diag_printf(diag, "[");
	diag_increase(diag);

	while (arr->indefinite || (!arr->indefinite && i < arr->u64)) {
		if (i == 0)
			diag_printfln(diag, "");
		else
			diag_printfln(diag, ",");

		if ((err = cbor_decode_header(diag->cs, &item)) != NB_ERR_OK)
			break;

		dump_item(diag, &item);
		i++;
	}

	if (i > 0)
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

	diag_printf(diag, "{");
	diag_increase(diag);

	is_key = true;
	while (map->indefinite || (!map->indefinite && i < map->u64)) {
		if ((err = cbor_decode_header(diag->cs, &item)) != NB_ERR_OK)
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

	if (i > 0)
		diag_printfln(diag, "");
	diag_decrease(diag);
	diag_printf(diag, "}");
}


static void dump_item(struct diag *diag, struct cbor_item *item)
{
	switch (item->type) {
	case CBOR_TYPE_UINT:
		diag_printf(diag, "%lu", item->u64);
		break;
	case CBOR_TYPE_INT:
		diag_printf(diag, "%li", item->i64);
		break;
	case CBOR_TYPE_BYTES:
		dump_bytes(diag, item);
		break;
	case CBOR_TYPE_TEXT:
		dump_text(diag, item);
		break;
	case CBOR_TYPE_ARRAY:
		dump_array(diag, item);
		break;
	case CBOR_TYPE_MAP:
		dump_map(diag, item);
		break;
	case CBOR_TYPE_TAG:
		diag_printf(diag, "(tag=%lu)", item->u64); /* TODO */
		break;
	case CBOR_TYPE_SVAL:
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
}


nb_err_t diag_dump(struct diag *diag, FILE *out)
{
	struct cbor_item item;
	nb_err_t err;
	size_t i;

	for (i = 0; !nb_buf_is_eof(diag->cs->buf); i++) {
		if ((err = cbor_decode_header(diag->cs, &item)) != NB_ERR_OK)
			break;

		if (i > 0)
			diag_printfln(diag, ",");
		dump_item(diag, &item);
	}

	fputs(strbuf_get_string(&diag->strbuf), out);
	return err;
}


void diag_free(struct diag *diag)
{
	strbuf_free(&diag->strbuf);
}
