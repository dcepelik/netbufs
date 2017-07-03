#include "buf.h"
#include "cbor-internal.h"
#include "cbor.h"
#include "debug.h"
#include "diag.h"
#include "netbufs.h"
#include "util.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BYTES_DUMP_MAXLEN_DEFAULT	64


static void isbuf_init(struct isbuf *is, size_t init_size)
{
	strbuf_init(&is->sb, init_size);
	is->indent_level = 0;
	is->indent_next = true;
}


static void isbuf_free(struct isbuf *is)
{
	strbuf_free(&is->sb);
}


static void isbuf_indent(struct isbuf *is)
{
	is->indent_level++;
}


static void isbuf_dedent(struct isbuf *is)
{
	assert(is->indent_level > 0);
	is->indent_level--;
}


static void isbuf_vprintf(struct isbuf *is, char *msg, va_list args)
{
	size_t i;

	if (is->indent_next) {
		for (i = 0; i < 4 * is->indent_level; i++)
			strbuf_putc(&is->sb, i % 2 == 0 ? '.' : ' ');
		is->indent_next = false;
	}

	strbuf_vprintf(&is->sb, msg, args);
}


static void isbuf_reset(struct isbuf *is)
{
	is->indent_next = true;
	strbuf_reset(&is->sb);
}


static char *isbuf_get_string(struct isbuf *is)
{
	return strbuf_get_string(&is->sb);
}


static void diag_dump_line_internal(struct diag *diag)
{
	if (!diag->have_output)
		return;
	diag->have_output = false;

	fprintf(diag->fout, "%-16s %-20s %-40s %-40s\n",
		//diag->output.offset,
		strbuf_get_string(&diag->output.raw),
		strbuf_get_string(&diag->output.item),
		isbuf_get_string(&diag->output.cbor_is),
		isbuf_get_string(&diag->output.proto_is));

	diag->output.offset[0] = '\0';
	strbuf_reset(&diag->output.raw);
	strbuf_reset(&diag->output.item);
	isbuf_reset(&diag->output.cbor_is);
	isbuf_reset(&diag->output.proto_is);
	
	diag->eol_on_next_raw = false;
}


void diag_log_offset(struct diag *diag, size_t offset)
{
	snprintf(diag->output.offset, sizeof(diag->output.offset), "0x%zx", offset);
}


void diag_log_raw(struct diag *diag, nb_byte_t *bytes, size_t count)
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

	va_start(args, msg);
	isbuf_vprintf(&diag->output.cbor_is, msg, args);
	diag->have_output = true;
	va_end(args);
}


void diag_log_proto(struct diag *diag, char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	isbuf_vprintf(&diag->output.proto_is, msg, args);
	diag->have_output = true;
	va_end(args);
}


void diag_dump_line(struct diag *diag)
{
	diag->eol_on_next_raw = true;
}


void diag_force_newline(struct diag *diag)
{
	diag_dump_line_internal(diag);
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
	isbuf_init(&diag->output.cbor_is, 32);
	isbuf_init(&diag->output.proto_is, 24);

	diag->cs = cs;
	diag->fout = fout;

	diag->bytes_dump_maxlen = BYTES_DUMP_MAXLEN_DEFAULT;
	diag->str_dump_maxlen = BYTES_DUMP_MAXLEN_DEFAULT;
	diag->eol_on_next_raw = false;
	diag->have_output = false;
}


void diag_increase(struct diag *diag)
{
	isbuf_indent(&diag->output.cbor_is);
}


void diag_decrease(struct diag *diag)
{
	isbuf_dedent(&diag->output.cbor_is);
}


void diag_indent_proto(struct diag *diag)
{
	isbuf_indent(&diag->output.proto_is);
}


void diag_dedent_proto(struct diag *diag)
{
	isbuf_dedent(&diag->output.proto_is);
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
	isbuf_free(&diag->output.cbor_is);
	isbuf_free(&diag->output.proto_is);
}


void diag_print_block_stack(struct diag *diag)
{
	struct block *block;
	size_t i = 0;

	stack_foreach(&diag->cs->blocks, block) {
		fprintf(stderr, "Block #%lu, type=%s, active_group=%s\n",
			i, cbor_type_string(block->type), block->group ? block->group->name : "<none>");
		i++;
	}
}
