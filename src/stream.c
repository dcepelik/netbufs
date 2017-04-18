#include "debug.h"
#include "internal.h"
#include "memory.h"
#include "util.h"

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>


void cbor_stream_init(struct cbor_stream *stream)
{
	size_t size = 1024;

	stream->buf = cbor_malloc(size);
	TEMP_ASSERT(stream->buf);
	stream->buf_size = size;
	stream->pos = 0;
	stream->len = 0;
	stream->mode = CBOR_STREAM_MODE_WRITE;
}


cbor_err_t cbor_stream_write(struct cbor_stream *stream, unsigned char *bytes, size_t count)
{
	assert(stream->mode == CBOR_STREAM_MODE_WRITE);

	size_t new_size;
	size_t i;

	for (i = 0; i < count; i++) {
		if (isprint(bytes[i])) {
			DEBUG_PRINTF("Writing byte 0x%02X [%c]", bytes[i], bytes[i]);
		}
		else {
			DEBUG_PRINTF("Writing byte 0x%02X", bytes[i]);
		}
	}

	if (stream->pos + count >= stream->buf_size) {
		new_size = MAX(2 * stream->buf_size, stream->pos + count);
		stream->buf = cbor_realloc(stream->buf, new_size);
		TEMP_ASSERT(stream->buf != NULL);
	}

	memcpy(stream->buf + stream->pos, bytes, count);
	stream->pos += count;
	stream->len = stream->pos;

	return CBOR_ERR_OK;
}


cbor_err_t cbor_stream_read(struct cbor_stream *stream, unsigned char *bytes, size_t offset, size_t count)
{
	size_t avail;

	if (stream->mode != CBOR_STREAM_MODE_READ) {
		stream->mode = CBOR_STREAM_MODE_READ;
		stream->pos = 0;
	}

	avail = stream->len - stream->pos + 1;
	TEMP_ASSERT(avail >= count);

	memcpy(bytes, stream->buf + stream->pos, MIN(count, avail));
	stream->pos += MIN(count, avail);
	return CBOR_ERR_OK;
}
