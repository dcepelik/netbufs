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


static void diag_dump_line_internal(struct diag *diag)
{
	if (!diag->have_output)
		return;
	diag->have_output = false;

	fprintf(diag->fout, "%-16s %-20s %-40s %-40s\n",
		//diag->output.offset,
		strbuf_get_string(&diag->output.raw),
		strbuf_get_string(&diag->output.item),
		strbuf_get_string(&diag->output.cbor),
		strbuf_get_string(&diag->output.proto));

	diag->output.offset[0] = '\0';
	strbuf_reset(&diag->output.raw);
	strbuf_reset(&diag->output.item);
	strbuf_reset(&diag->output.cbor);
	strbuf_reset(&diag->output.proto);
	
	diag->indent_next = true;
	diag->eol_on_next_raw = false;
}


void diag_log_offset(struct diag *diag, size_t offset)
{
	snprintf(diag->output.offset, sizeof(diag->output.offset), "0x%zx", offset);
}


void diag_log_raw(struct diag *diag, byte_t *bytes, size_t count)
{
	size_t i;

	if (diag->eol_on_next_raw)
		diag_dump_line_internal(diag);

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
		for (i = 0; i < 4 * diag->indent_level; i++)
			strbuf_putc(&diag->output.cbor, i % 2 == 0 ? '.' : ' ');
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
	diag->indent_level++;
}


void diag_decrease(struct diag *diag)
{
	assert(diag->indent_level > 0);
	diag->indent_level--;
}


void diag_dump_line(struct diag *diag)
{
	diag->eol_on_next_raw = true;
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


void diag_init(struct diag *diag, struct cbor_stream *cs, FILE *fout)
{
	diag->output.offset[0] = '\0';
	strbuf_init(&diag->output.raw, 8);
	strbuf_init(&diag->output.item, 16);
	strbuf_init(&diag->output.cbor, 32);
	strbuf_init(&diag->output.proto, 24);

	diag->cs = cs;
	diag->fout = fout;

	diag->bytes_dump_maxlen = 16;
	diag->have_output = false;
	diag->indent_next = true;
	diag->indent_level = 0;
	diag->eol_on_next_raw = false;
	diag->str_dump_maxlen = 16;
}


nb_err_t diag_dump(struct diag *diag, FILE *out)
{
	struct cbor_item item;
	nb_err_t err = NB_ERR_OK;
	size_t i;

	for (i = 0; !nb_buf_is_eof(diag->cs->buf); i++) {
		if (i > 0) {
			diag_log_cbor(diag, ",");
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
