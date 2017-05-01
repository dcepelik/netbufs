#include "cbor.h"
#include "debug.h"
#include "internal.h"
#include "memory.h"
#include "strbuf.h"
#include "util.h"

#include <assert.h>


cbor_err_t error(struct cbor_stream *cs, cbor_err_t err, char *str, ...)
{
	va_list args;

	cs->err = err;
	va_start(args, str);
	strbuf_reset(&cs->err_buf);
	strbuf_vprintf_at(&cs->err_buf, 0, str, args);
	va_end(args);

	if (cs->fail_on_error) {
		die("Error: %s\n", cs->err_buf.str);
	}

	return err;
}


cbor_err_t push_block(struct cbor_stream *cs, enum cbor_type type, bool indefinite, uint64_t len)
{
	struct block *block;

	block = stack_push(&cs->blocks);
	if (!block)
		return error(cs, CBOR_ERR_NOMEM, "No memory to allocate new nesting block.");

	block->type = type;
	block->indefinite = indefinite;
	block->len = len;
	block->num_items = 0;

	return CBOR_ERR_OK;
}


struct block *top_block(struct cbor_stream *cs)
{
	assert(!stack_is_empty(&cs->blocks));
	return stack_top(&cs->blocks);
}


struct cbor_stream *cbor_stream_new(struct nb_buf *buf)
{
	struct cbor_stream *cs;

	cs = nb_malloc(sizeof(*cs));
	cs->buf = buf;
	cs->err = CBOR_ERR_OK;

	if (!stack_init(&cs->blocks, 4, sizeof(struct block))) {
		free(cs);
		return NULL;
	}

	 /* bottom stack block: avoids corner-cases */
	push_block(cs, -1, true, 0);

	strbuf_init(&cs->err_buf, 24); /* TODO change API, what if this fails? */

	cs->fail_on_error = true;

	return cs;
}


void cbor_stream_delete(struct cbor_stream *cs)
{
	strbuf_free(&cs->err_buf);
	nb_free(cs);
}


char *cbor_stream_strerror(struct cbor_stream *cs)
{
	return strbuf_get_string(&cs->err_buf);
}
