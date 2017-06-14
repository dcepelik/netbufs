#include "cbor.h"
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


static bool is_simple_item(struct cbor_item *item)
{
	size_t i;
	bool all_simple = true;

	switch (item->type)
	{
		case CBOR_TYPE_ARRAY:
			for (i = 0; i < item->len; i++)
				all_simple &= is_simple_item(&item->items[i]);
			return all_simple && item->len <= 4;

		case CBOR_TYPE_MAP:
			return item->len == 0;

		case CBOR_TYPE_BYTES:
		case CBOR_TYPE_TEXT:
			return item->len < 8;

		default:
			return true;
	}
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


static void dump_item(struct isbuf *isbuf, struct cbor_item *item);


static void dump_map(struct isbuf *isbuf, struct cbor_item *map)
{
	size_t i;

	if (is_simple_item(map)) {
		isbuf_printf(isbuf, "{");
		for (i = 0; i < map->len; i++) {
			if (i > 0)
				isbuf_printf(isbuf, ", ");

			dump_item(isbuf, &map->pairs[i].key);
			isbuf_printf(isbuf, ": ");
			dump_item(isbuf, &map->pairs[i].value);
		}
		isbuf_printf(isbuf, "}");
	}
	else {
		isbuf_printfln(isbuf, "{");
		isbuf_increase(isbuf);

		for (i = 0; i < map->len; i++) {
			if (i > 0)
				isbuf_printfln(isbuf, ",");

			dump_item(isbuf, &map->pairs[i].key);
			isbuf_printf(isbuf, ": ");
			dump_item(isbuf, &map->pairs[i].value);
		}

		isbuf_printfln(isbuf, "");
		isbuf_decrease(isbuf);
		isbuf_printf(isbuf, "}");
	}
}


static void dump_array(struct isbuf *isbuf, struct cbor_item *array)
{
	size_t i;

	if (is_simple_item(array)) {
		isbuf_printf(isbuf, "[");
		for (i = 0; i < array->len; i++) {
			if (i > 0)
				isbuf_printf(isbuf, ", ");
			dump_item(isbuf, &array->items[i]);
		}
		isbuf_printf(isbuf, "]");
	}
	else {
		isbuf_printfln(isbuf, "[");
		isbuf_increase(isbuf);

		for (i = 0; i < array->len; i++) {
			if (i > 0)
				isbuf_printfln(isbuf, ",");
			dump_item(isbuf, &array->items[i]);
		}
		isbuf_printfln(isbuf, "");
		isbuf_decrease(isbuf);
		isbuf_printf(isbuf, "]");
	}
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


static void dump_tag(struct isbuf *isbuf, struct cbor_item *tag)
{
	isbuf_printf(isbuf, "%u(", tag->u64);
	dump_item(isbuf, tag->tagged_item);
	isbuf_printf(isbuf, ")");
}


static void dump_item(struct isbuf *isbuf, struct cbor_item *item)
{
	switch (item->type) {
	case CBOR_TYPE_UINT:
		isbuf_printf(isbuf, "%lu", item->u64);
		break;
	case CBOR_TYPE_INT:
		isbuf_printf(isbuf, "%li", item->i64);
		break;
	case CBOR_TYPE_BYTES:
		dump_bytes(isbuf, item->bytes, item->len);
		break;
	case CBOR_TYPE_TEXT:
		dump_text(isbuf, item->str, item->len);
		break;
	case CBOR_TYPE_ARRAY:
		dump_array(isbuf, item);
		break;
	case CBOR_TYPE_MAP:
		dump_map(isbuf, item);
		break;
	case CBOR_TYPE_TAG:
		dump_tag(isbuf, item);
		break;
	case CBOR_TYPE_SVAL:
		dump_sval(isbuf, item);
		break;
	default:
		break;
	}
}


void cbor_item_dump(struct cbor_item *item, FILE *file)
{
	struct isbuf isbuf;

	isbuf_init(&isbuf);
	dump_item(&isbuf, item);
	fputs(strbuf_get_string(&isbuf.strbuf), file);
	isbuf_free(&isbuf);
}


nb_err_t cbor_stream_dump(struct cbor_stream *cs, FILE *file)
{
	struct isbuf isbuf;
	struct cbor_item item;
	nb_err_t err = NB_ERR_OK;
	size_t i = 0;

	isbuf_init(&isbuf);

	while (!nb_buf_is_eof(cs->buf)) {
		if ((err = cbor_decode_item(cs, &item)) != NB_ERR_OK)
			break;

		if (i > 0)
			isbuf_printfln(&isbuf, ",");

		dump_item(&isbuf, &item);
		i++;
	}

	fputs(strbuf_get_string(&isbuf.strbuf), file);
	isbuf_free(&isbuf);
	return err;
}


const char *cbor_type_string(enum cbor_type type)
{
	switch (type) {
	case CBOR_TYPE_UINT:
		return "unsigned integer";
	case CBOR_TYPE_INT:
		return "integer";
	case CBOR_TYPE_BYTES:
		return "byte string";
	case CBOR_TYPE_TEXT:
		return "text string";
	case CBOR_TYPE_ARRAY:
		return "array";
	case CBOR_TYPE_MAP:
		return "map";
	case CBOR_TYPE_TAG:
		return "tag";
	case CBOR_TYPE_SVAL:
		return "simple value";
	case CBOR_TYPE_FLOAT16:
		return "16-bit float";
	case CBOR_TYPE_FLOAT32:
		return "32-bit float";
	case CBOR_TYPE_FLOAT64:
		return "64-bit float";
	}

	return NULL;
}
