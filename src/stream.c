#include "cbor.h"
#include "debug.h"
#include "internal.h"
#include "memory.h"
#include "strbuf.h"
#include "util.h"

#include <assert.h>


nb_err_t error(struct cbor_stream *cs, nb_err_t err, char *str, ...)
{
	assert(err != NB_ERR_OK);

	va_list args;

	cs->err = err;
	va_start(args, str);
	strbuf_reset(&cs->err_buf);
	strbuf_vprintf_at(&cs->err_buf, 0, str, args);
	va_end(args);

	if (cs->error_handler)
		cs->error_handler(cs, err, cs->error_handler_arg);

	return err;
}


nb_err_t push_block(struct cbor_stream *cs, enum cbor_type type, bool indefinite, uint64_t len)
{
	struct block *block;

	block = stack_push(&cs->blocks);
	if (!block)
		return error(cs, NB_ERR_NOMEM, "No memory to allocate new nesting block.");

	block->type = type;
	block->indefinite = indefinite;
	block->len = len;
	block->num_items = 0;

	return NB_ERR_OK;
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
	cs->err = NB_ERR_OK;
	cs->peeking = false;
	cs->error_handler = NULL; /* don't use an error handler */

	strbuf_init(&cs->err_buf, 24); /* TODO change strbuf API and return error flags? */

	if (!stack_init(&cs->blocks, 4, sizeof(struct block))) {
		xfree(cs);
		return NULL; /* lift to NB_ERR_NOMEM */
	}
	
	/* cannot fail, stack was initialized to size >= 1 */
	assert(push_block(cs, -1, true, 0) == NB_ERR_OK);

	return cs;
}


void cbor_stream_delete(struct cbor_stream *cs)
{
	strbuf_free(&cs->err_buf);
	stack_free(&cs->blocks);
	xfree(cs);
}


char *cbor_stream_strerror(struct cbor_stream *cs)
{
	return strbuf_get_string(&cs->err_buf);
}


bool cbor_block_stack_empty(struct cbor_stream *cs)
{
	return top_block(cs)->type == -1;
}


void cbor_stream_set_error_handler(struct cbor_stream *cs, cbor_error_handler_t *handler,
	void *arg)
{
	cs->error_handler = handler;
	cs->error_handler_arg = arg;
}
