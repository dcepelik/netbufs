#include "cbor.h"
#include "internal.h"
#include "strbuf.h"
#include "memory.h"


struct cbor_stream *cbor_stream_new(struct buf *buf)
{
	struct cbor_stream *cs;

	cs = cbor_malloc(sizeof(*cs));
	cs->buf = buf;

	strbuf_init(&cs->err_buf, 24);

	if (!stack_init(&cs->blocks, 4, sizeof(struct block))) {
		free(cs);
		return NULL;
	}

	return cs;
}


void cbor_stream_delete(struct cbor_stream *cs)
{
	strbuf_free(&cs->err_buf);
	cbor_free(cs);
}


cbor_err_t push_block(struct cbor_stream *cs, enum major major, bool indefinite, uint64_t u64)
{
	struct block *block;

	block = stack_push(&cs->blocks);
	if (!block)
		return error(cs, CBOR_ERR_NOMEM, "No memory to allocate new nesting block.");

	block->major = major;
	block->indefinite = indefinite;
	block->len = u64;
	block->num_items = 0;

	return CBOR_ERR_OK;
}


cbor_err_t error(struct cbor_stream *cs, cbor_err_t err, char *str, ...)
{
	//assert(err == CBOR_ERR_OK); /* uncomment for easier debugging */

	va_list args;

	cs->err = err;
	va_start(args, str);
	strbuf_vprintf_at(&cs->err_buf, 0, str, args);
	va_end(args);
	return err;
}


char *cbor_stream_strerror(struct cbor_stream *cs)
{
	return cs->err_buf.str;
}
