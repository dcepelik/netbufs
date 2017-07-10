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


static void reset_line(struct diag_line *line)
{
	line->offset[0] = '\0';
	strbuf_reset(&line->raw);
	strbuf_reset(&line->item);
	isbuf_reset(&line->cbor);
	isbuf_reset(&line->proto);
	line->empty = true;
	line->cbor_comma = false;
}


static void switch_lines(struct diag *diag)
{
	assert(ARRAY_SIZE(diag->lines) == 2);

	unsigned char new_idx;

	/* TODO is it possible to avoid these copies? */
	new_idx = 1 - diag->line_idx;
	assert(new_idx != diag->line_idx);

	diag->lines[new_idx].cbor.indent_level = diag->line->cbor.indent_level;
	diag->lines[new_idx].proto.indent_level = diag->line->proto.indent_level;

	diag->line_idx = new_idx;
	diag->line = &diag->lines[diag->line_idx];
}


static void diag_dump_line_internal(struct diag *diag)
{
	switch_lines(diag);

	if (!diag->line->empty) {
		fprintf(diag->fout, "%-16s %-20s %-60s %-60s\n",
			//diag->line->offset,
			strbuf_get_string(&diag->line->raw),
			strbuf_get_string(&diag->line->item),
			isbuf_get_string(&diag->line->cbor),
			isbuf_get_string(&diag->line->proto));

		reset_line(diag->line);
	}

	diag->eol_on_next_raw = false;
}


void diag_log_offset_internal(struct diag *diag, size_t offset)
{
	snprintf(diag->line->offset, sizeof(diag->line->offset), "0x%zx", offset);
}


void diag_log_raw_internal(struct diag *diag, nb_byte_t *bytes, size_t count)
{
	size_t i;

	if (diag->eol_on_next_raw)
		diag_dump_line_internal(diag);

	for (i = 0; i < count; i++)
		strbuf_printf(&diag->line->raw, "%02X", bytes[i]);
	diag->line->empty = false;
}


void diag_log_item_internal(struct diag *diag, char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	strbuf_vprintf(&diag->line->item, msg, args);
	diag->line->empty = false;
	va_end(args);
}


void diag_log_cbor_internal(struct diag *diag, char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	isbuf_vprintf(&diag->line->cbor, msg, args);
	diag->line->empty = false;
	va_end(args);
}


void diag_log_proto_internal(struct diag *diag, char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	isbuf_vprintf(&diag->line->proto, msg, args);
	diag->line->empty = false;
	va_end(args);
}


void diag_dump_line(struct diag *diag)
{
	diag->eol_on_next_raw = true;
}


void diag_eol(struct diag *diag, bool cbor_comma)
{
	diag->line->cbor_comma = cbor_comma;
	diag->eol_on_next_raw = true;
}


void diag_comma(struct diag *diag)
{
	if (diag->lines[1 - diag->line_idx].cbor_comma) {
		strbuf_printf(&diag->lines[1 - diag->line_idx].cbor.sb, ",");
		diag->lines[1 - diag->line_idx].cbor_comma = false;
	}
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


static void init_line(struct diag_line *line)
{
	line->offset[0] = '\0';
	strbuf_init(&line->raw, 8);
	strbuf_init(&line->item, 16);
	isbuf_init(&line->cbor, 32);
	isbuf_init(&line->proto, 24);
	reset_line(line);
}


void diag_init(struct diag *diag, struct cbor_stream *cs, FILE *fout)
{
	size_t i;
	
	for (i = 0; i < ARRAY_SIZE(diag->lines); i++)
		init_line(&diag->lines[i]);

	diag->line_idx = 0;
	diag->line = &diag->lines[diag->line_idx];

	diag->cs = cs;
	diag->fout = fout;
	diag->enabled = true;
	diag->bytes_dump_maxlen = BYTES_DUMP_MAXLEN_DEFAULT;
	diag->str_dump_maxlen = BYTES_DUMP_MAXLEN_DEFAULT;
	diag->eol_on_next_raw = false;
}


void diag_increase(struct diag *diag)
{
	isbuf_indent(&diag->line->cbor);
}


void diag_decrease(struct diag *diag)
{
	isbuf_dedent(&diag->line->cbor);
}


void diag_indent_proto(struct diag *diag)
{
	isbuf_indent(&diag->line->proto);
}


void diag_dedent_proto(struct diag *diag)
{
	isbuf_dedent(&diag->line->proto);
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
	strbuf_free(&diag->line->raw);
	strbuf_free(&diag->line->item);
	isbuf_free(&diag->line->cbor);
	isbuf_free(&diag->line->proto);
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
