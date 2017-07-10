#include "cbor-internal.h"
#include "cbor.h"
#include "debug.h"
#include "diag.h"
#include "memory.h"
#include "strbuf.h"
#include "util.h"

#include <assert.h>


const char *cbor_type_string(enum cbor_type type)
{
	switch (type) {
	case CBOR_TYPE_UINT:
		return "uint";
	case CBOR_TYPE_INT:
		return "int";
	case CBOR_TYPE_BYTES:
		return "bytestream";
	case CBOR_TYPE_TEXT:
		return "text";
	case CBOR_TYPE_ARRAY:
		return "array";
	case CBOR_TYPE_MAP:
		return "map";
	case CBOR_TYPE_TAG:
		return "tag";
	case CBOR_TYPE_SVAL:
		return "sval";
	case CBOR_TYPE_FLOAT16:
		return "16-bit float";
	case CBOR_TYPE_FLOAT32:
		return "32-bit float";
	case CBOR_TYPE_FLOAT64:
		return "64-bit float";
	}

	return NULL;
}


struct cbor_stream *cbor_stream_new(struct nb_buf *buf)
{
	struct cbor_stream *cs;

	cs = nb_malloc(sizeof(*cs));
	cs->buf = buf;
	cs->err = NB_ERR_OK;
	cs->error_handler = cbor_default_error_handler;
	cs->peeking = false;

	strbuf_init(&cs->err_buf, 24); /* TODO change strbuf API and return error flags? */

	if (!stack_init(&cs->blocks, 4, sizeof(struct block))) {
		xfree(cs);
		return NULL; /* lift to NB_ERR_NOMEM */
	}
	
	/* assert: cannot fail, stack was initialized to size >= 1 */
	assert(push_block(cs, -1, true, 0) == NB_ERR_OK);
	top_block(cs)->group = NULL;

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
	assert(handler != NULL);
	cs->error_handler = handler;
	cs->error_handler_arg = arg;
}


void cbor_stream_set_diag(struct cbor_stream *cs, struct diag *diag)
{
	cs->diag = diag;
}


nb_err_t error(struct cbor_stream *cs, nb_err_t err, char *str, ...)
{
	assert(err != NB_ERR_OK);

	va_list args;

	cs->err = err;
	va_start(args, str);
	strbuf_reset(&cs->err_buf);
	strbuf_vprintf_at(&cs->err_buf, 0, str, args);
	va_end(args);

	cs->error_handler(cs, err, cs->error_handler_arg);

	return err;
}


nb_err_t push_block(struct cbor_stream *cs, enum cbor_type type, bool indefinite, uint64_t len)
{
	struct block *block;
	struct nb_group *active_group = NULL;

	if (!stack_is_empty(&cs->blocks))
		active_group = top_block(cs)->group;

	block = stack_push(&cs->blocks);
	if (!block)
		return error(cs, NB_ERR_NOMEM, "No memory to allocate new nesting block.");

	block->type = type;
	block->indefinite = indefinite;
	block->len = len;
	block->num_items = 0;
	block->group = active_group;

	return NB_ERR_OK;
}


struct block *top_block(struct cbor_stream *cs)
{
	assert(!stack_is_empty(&cs->blocks));
	return stack_top(&cs->blocks);
}


void cbor_default_error_handler(struct cbor_stream *cs, nb_err_t err, void *arg)
{
	diag_log_raw(cs->diag, NULL, 0); /* TODO hack */
	fprintf(stderr, "%s: encoding/decoding error #%i: %s.\n",
		__func__, err, cbor_stream_strerror(cs));
	//fprintf(stderr, "Block stack:\n");
	//diag_print_block_stack(cs->diag);
	exit(EXIT_FAILURE);
}
