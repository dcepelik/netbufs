#include "cbor.h"
#include "diag.h"
#include "debug.h"
#include "internal.h"
#include "strbuf.h"
#include "util.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define BYTES_DUMP_MAXLEN	64


struct isbuf {
	struct strbuf strbuf;
	unsigned level;
	bool indent_next;
};


static void isbuf_init(struct isbuf *isbuf)
{
	strbuf_init(&isbuf->strbuf, 64);
	isbuf->level = 0;
	isbuf->indent_next = true;
}


static void isbuf_free(struct isbuf *isbuf)
{
	strbuf_free(&isbuf->strbuf);
}


static void isbuf_vprintf(struct isbuf *isbuf, char *msg, va_list args)
{
	size_t i;
	if (isbuf->indent_next) {
		for (i = 0; i < 4 * isbuf->level; i++)
			strbuf_putc(&isbuf->strbuf, ' ');
		isbuf->indent_next = false;
	}
	strbuf_vprintf_at(&isbuf->strbuf, isbuf->strbuf.len, msg, args);
}


static void isbuf_printf(struct isbuf *isbuf, char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	isbuf_vprintf(isbuf, msg, args);
	va_end(args);
}


static void isbuf_printfln(struct isbuf *isbuf, char *msg, ...)
{
	va_list args;

	va_start(args, msg);
	isbuf_vprintf(isbuf, msg, args);
	strbuf_putc(&isbuf->strbuf, '\n');
	isbuf->indent_next = true;
	va_end(args);
}


static void isbuf_increase(struct isbuf *isbuf)
{
	isbuf->level++;
}


static void isbuf_decrease(struct isbuf *isbuf)
{
	assert(isbuf->level > 0);
	isbuf->level--;
}


static void dump_char_printable(struct isbuf *isbuf, char c)
{
	switch (c) {
	case '\n':
		isbuf_printf(isbuf, "\\n");
		return;
	case '\t':
		isbuf_printf(isbuf, "\\t");
		return;
	case '\r':
		isbuf_printf(isbuf, "\\r");
		return;
	case '\"':
		isbuf_printf(isbuf, "\\\"");
		return;
	case '\\':
		isbuf_printf(isbuf, "\\\\");
		return;
	default:
		isbuf_printf(isbuf, "%c", c);
	}
}


static void dump_text(struct isbuf *isbuf, char *bytes, size_t len)
{
	size_t i;
	size_t dump_len = MIN(len, BYTES_DUMP_MAXLEN);

	isbuf_printf(isbuf, "\"");
	for (i = 0; i < dump_len; i++)
		dump_char_printable(isbuf, bytes[i]);
	if (len > dump_len)
		isbuf_printf(isbuf, "...");
	isbuf_printf(isbuf, "\"");
}


static void dump_bytes(struct isbuf *isbuf, byte_t *bytes, size_t len)
{
	size_t i;
	size_t dump_len = MIN(len, BYTES_DUMP_MAXLEN);

	isbuf_printf(isbuf, "h'");
	for (i = 0; i < dump_len; i++)
		isbuf_printf(isbuf, "%02X", bytes[i]);
	if (len > dump_len)
		isbuf_printf(isbuf, "...");
	isbuf_printf(isbuf, "'");
}


static void dump_sval(struct isbuf *isbuf, struct cbor_item *sval)
{
	switch (sval->u64) {
	case CBOR_SVAL_FALSE:
		isbuf_printf(isbuf, "false");
		break;
	case CBOR_SVAL_TRUE:
		isbuf_printf(isbuf, "true");
		break;
	case CBOR_SVAL_NULL:
		isbuf_printf(isbuf, "null");
		break;
	case CBOR_SVAL_UNDEF:
		isbuf_printf(isbuf, "undefined");
		break;
	default:
		isbuf_printf(isbuf, "simple(%lu)", sval->u64);
	}
}


static void dump_item(struct cbor_stream *cs, struct isbuf *isbuf,
	struct cbor_item *item);


static void dump_array(struct cbor_stream *cs, struct isbuf *isbuf,
	struct cbor_item *arr)
{
	struct cbor_item item;
	size_t i = 0;
	nb_err_t err = NB_ERR_OK;

	isbuf_printf(isbuf, "[");
	isbuf_increase(isbuf);

	while ((!arr->indefinite && i < arr->u64) || (arr->indefinite && err == NB_ERR_OK)) {
		if (i == 0)
			isbuf_printfln(isbuf, "");
		else
			isbuf_printfln(isbuf, ",");

		if ((err = cbor_decode_header(cs, &item)) == NB_ERR_OK)
			dump_item(cs, isbuf, &item);
		i++;
	}

	if (i > 0)
		isbuf_printfln(isbuf, "");
	isbuf_decrease(isbuf);
	isbuf_printf(isbuf, "]");
}


static void dump_map_sequential(struct cbor_stream *cs, struct isbuf *isbuf,
	struct cbor_item *map)
{
	struct cbor_item item;
	size_t i;
	bool is_key;
	nb_err_t err = NB_ERR_OK;

	isbuf_printfln(isbuf, "{");
	isbuf_increase(isbuf);

	is_key = true;
	while ((!map->indefinite && i < map->len) || err == NB_ERR_OK) {
		if (i > 0)
			isbuf_printfln(isbuf, ",");

		cbor_decode_header(cs, &item);

		if (is_key) {
			dump_item(cs, isbuf, &item);
			isbuf_printfln(isbuf, ":");
		}
		else {
			isbuf_increase(isbuf);
			dump_item(cs, isbuf, &item);
			isbuf_decrease(isbuf);
		}

		is_key = !is_key;
		i++;
	}

	isbuf_printfln(isbuf, "");
	isbuf_decrease(isbuf);
	isbuf_printf(isbuf, "}");
}


static void dump_item(struct cbor_stream *cs, struct isbuf *isbuf,
	struct cbor_item *item)
{
	switch (item->type) {
	case CBOR_TYPE_UINT:
		isbuf_printf(isbuf, "%lu", item->u64);
		break;
	case CBOR_TYPE_INT:
		isbuf_printf(isbuf, "%li", item->i64);
		break;
	case CBOR_TYPE_BYTES:
		//dump_bytes(isbuf, item->bytes, item->len);
		break;
	case CBOR_TYPE_TEXT:
		//dump_text(isbuf, item->str, item->len);
		break;
	case CBOR_TYPE_ARRAY:
		dump_array(cs, isbuf, item);
		break;
	case CBOR_TYPE_MAP:
		//dump_map_sequential(cs, isbuf, item);
		break;
	case CBOR_TYPE_TAG:
		/* TODO print tags somehow */
		break;
	case CBOR_TYPE_SVAL:
		dump_sval(isbuf, item);
		break;
	default:
		break;
	}
}


nb_err_t cbor_stream_dump(struct cbor_stream *cs, FILE *file)
{
	struct isbuf isbuf;
	struct cbor_item item;
	nb_err_t err;
	size_t i;

	isbuf_init(&isbuf);

	for (i = 0; !nb_buf_is_eof(cs->buf); i++) {
		if ((err = cbor_decode_header(cs, &item)) != NB_ERR_OK)
			break;

		if (i > 0)
			isbuf_printfln(&isbuf, ",");
		dump_item(cs, &isbuf, &item);
	}

	fputs(strbuf_get_string(&isbuf.strbuf), file);
	isbuf_free(&isbuf);
	return err;
}
