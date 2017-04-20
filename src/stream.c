#include "debug.h"
#include "internal.h"
#include "memory.h"
#include "util.h"

#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


struct cbor_stream
{
	unsigned char *buf;	/* data buffer */
	size_t bufsize;	/* size of the buffer */
	size_t pos;	/* current read/write position within the stream */
	size_t len;	/* number of valid bytes in the buffer (for reading) */
	bool dirty;	/* do we have data to be written? */
	bool eof;	/* did we hit EOF during last filling? */

	/* for file streams */
	char *filename;	/* filename of currently open file */
	int fd;		/* file descriptor */
	int mode;	/* file flags (@see man 3p open) */

	/* for in-memory streams */
	unsigned char *memory;	/* the memory */
	size_t memory_size;	/* memory size */
	size_t memory_len;	/* number of valid bytes in memory */
	size_t memory_pos;	/* position in memory (for reading/writing) */

	void (*flush)(struct cbor_stream *stream);
	void (*fill)(struct cbor_stream *stream);
	void (*close)(struct cbor_stream *stream);
};


void cbor_stream_init(struct cbor_stream *stream)
{ size_t size = 1024;

	stream->buf = cbor_malloc(size);
	TEMP_ASSERT(stream->buf);

	stream->bufsize = size;
	stream->pos = 0;
	stream->len = 0;
	stream->dirty = false;
	stream->eof = false;
}


void cbor_stream_free(struct cbor_stream *stream)
{
	cbor_free(stream->buf);
}


static void cbor_stream_debug_write(struct cbor_stream *stream, unsigned char *bytes, size_t nbytes)
{
	size_t i;

	for (i = 0; i < nbytes; i++) {
		if (isprint(bytes[i])) {
			DEBUG_PRINTF("Writing byte 0x%02X [%c]", bytes[i], bytes[i]);
		}
		else {
			DEBUG_PRINTF("Writing byte 0x%02X", bytes[i]);
		}
	}
}


static void cbor_stream_flush(struct cbor_stream *stream)
{
	stream->flush(stream);
	stream->pos = 0;
	stream->dirty = false;
}


static void cbor_stream_fill(struct cbor_stream *stream)
{
	stream->fill(stream);
	stream->pos = 0;
}


/* TODO Chech that: once EOF is returned for the first time, all successive calls return EOF as well */
cbor_err_t cbor_stream_read(struct cbor_stream *stream, unsigned char *bytes, size_t offset, size_t nbytes)
{
	size_t avail;
	size_t ncpy;

	bytes += offset; /* TODO Get rid of offset arg */

	if (stream->dirty)
		cbor_stream_flush(stream);

	while (nbytes > 0) {
		avail = stream->len - stream->pos;
		assert(avail >= 0);

		if (!avail) {
			cbor_stream_fill(stream);
			avail = stream->len;

			if (avail == 0) {
				return CBOR_ERR_EOF;
			}
		}

		ncpy = MIN(avail, nbytes);
		memcpy(bytes, stream->buf + stream->pos, ncpy);
		bytes += ncpy;
		stream->pos += ncpy;
		nbytes -= ncpy;
	}

	if (nbytes > 0)
		return CBOR_ERR_READ; /* TODO save strerror(errno) somewhere */

	return CBOR_ERR_OK;
}


cbor_err_t cbor_stream_write(struct cbor_stream *stream, unsigned char *bytes, size_t nbytes)
{
	size_t avail;
	size_t ncpy;

	if (!stream->dirty) {
		stream->pos = 0;
	}

	stream->dirty = true;

	while (nbytes > 0) {
		avail = stream->bufsize - stream->len;
		assert(avail >= 0);

		if (!avail) {
			cbor_stream_flush(stream);
			avail = stream->bufsize;
		}

		ncpy = MIN(avail, nbytes);
		memcpy(stream->buf + stream->pos, bytes, ncpy);
		bytes += ncpy;
		stream->pos += ncpy;
		stream->len += ncpy;
		nbytes -= ncpy;
	}

	return CBOR_ERR_OK;
}


static void cbor_stream_file_close(struct cbor_stream *stream)
{
	close(stream->fd);
}

static void cbor_stream_file_fill(struct cbor_stream *stream)
{
	stream->len = read(stream->fd, stream->buf, stream->bufsize);
}


static void cbor_stream_file_flush(struct cbor_stream *stream)
{
	size_t to_write;
	ssize_t written;

	for (to_write = stream->len; to_write > 0; to_write -= written) {
		written = write(stream->fd, stream->buf, to_write);
	}
}


static void cbor_stream_memory_close(struct cbor_stream *stream)
{
	cbor_free(stream->memory);
}


static void cbor_stream_memory_fill(struct cbor_stream *stream)
{
	size_t avail;
	size_t ncpy;
	
	avail = stream->memory_len - stream->memory_pos;
	ncpy = MIN(avail, stream->bufsize);

	memcpy(stream->buf, stream->memory + stream->memory_pos, ncpy);

	stream->len = ncpy;
	stream->memory_pos += ncpy;
}


static void cbor_stream_memory_flush(struct cbor_stream *stream)
{
	size_t new_mry_size;

	if (stream->memory_len + stream->len > stream->memory_size) {
		new_mry_size = MAX(2 * stream->memory_size, stream->bufsize);
		stream->memory = cbor_realloc(stream->memory, new_mry_size);
		stream->memory_size = new_mry_size;
	}

	memcpy(stream->memory + stream->memory_len, stream->buf, stream->len);
	stream->memory_len += stream->len;

	DEBUG_EXPR("%lu", stream->memory_len);
}


struct cbor_stream *cbor_stream_new(void)
{
	struct cbor_stream *stream;
	
	stream = cbor_malloc(sizeof(*stream));
	if (stream)
		cbor_stream_init(stream);

	return stream;
}


void cbor_stream_delete(struct cbor_stream *stream)
{
	cbor_stream_free(stream);
	cbor_free(stream);
}


cbor_err_t cbor_stream_open_file(struct cbor_stream *stream, char *filename, int flags, int mode)
{
	if ((stream->fd = open(filename, flags, mode)) == -1)
		return CBOR_ERR_NOFILE; /* TODO */

	stream->filename = filename;
	stream->mode = mode;

	stream->close = cbor_stream_file_close;
	stream->fill = cbor_stream_file_fill;
	stream->flush = cbor_stream_file_flush;

	return CBOR_ERR_OK;
}


cbor_err_t cbor_stream_open_memory(struct cbor_stream *stream)
{
	stream->memory = NULL;
	stream->memory_len = 0;
	stream->memory_size = 0;
	stream->memory_pos = 0;

	stream->close = cbor_stream_memory_close;
	stream->fill = cbor_stream_memory_fill;
	stream->flush = cbor_stream_memory_flush;

	return CBOR_ERR_OK;
}


void cbor_stream_close(struct cbor_stream *stream)
{
	if (stream->dirty)
		stream->flush(stream);

	if (stream->close)
		stream->close(stream);
}
