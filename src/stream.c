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
