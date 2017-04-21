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


void buf_init(struct buf *buf)
{ size_t size = 1024;

	buf->buf = cbor_malloc(size);
	TEMP_ASSERT(buf->buf);

	buf->bufsize = size;
	buf->pos = 0;
	buf->len = 0;
	buf->dirty = false;
	buf->eof = false;
}


void buf_free(struct buf *buf)
{
	cbor_free(buf->buf);
}


static void buf_debug_write(struct buf *buf, byte_t *bytes, size_t nbytes)
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


static void buf_flush(struct buf *buf)
{
	buf->flush(buf);
	buf->pos = 0;
	buf->dirty = false;
}


static void buf_fill(struct buf *buf)
{
	buf->fill(buf);
	buf->pos = 0;
}


/* TODO Check that: once EOF is returned for the first time, all successive calls return EOF as well */
cbor_err_t buf_read(struct buf *buf, byte_t *bytes, size_t offset, size_t nbytes)
{
	size_t avail;
	size_t ncpy;

	bytes += offset; /* TODO Get rid of offset arg */

	if (buf->dirty)
		buf_flush(buf);

	buf->last_read_len = 0;

	while (nbytes > 0) {
		avail = buf->len - buf->pos;
		assert(avail >= 0);

		if (!avail) {
			buf_fill(buf);
			avail = buf->len;

			if (avail == 0)
				return CBOR_ERR_EOF;
		}

		ncpy = MIN(avail, nbytes);
		memcpy(bytes, buf->buf + buf->pos, ncpy);
		bytes += ncpy;
		buf->pos += ncpy;
		nbytes -= ncpy;
		buf->last_read_len += ncpy;
	}

	if (nbytes > 0)
		return CBOR_ERR_READ; /* TODO save strerror(errno) somewhere */

	return CBOR_ERR_OK;
}


size_t buf_read_len(struct buf *buf, byte_t *bytes, size_t nbytes)
{
	buf_read(buf, bytes, 0, nbytes);
	return buf_get_last_read_len(buf);
}


cbor_err_t buf_write(struct buf *buf, byte_t *bytes, size_t nbytes)
{
	size_t avail;
	size_t ncpy;

	if (!buf->dirty) {
		buf->pos = 0;
	}

	while (nbytes > 0) {
		avail = buf->bufsize - buf->len;
		assert(avail >= 0);

		if (!avail) {
			buf_flush(buf);
			avail = buf->bufsize;
		}

		ncpy = MIN(avail, nbytes);
		memcpy(buf->buf + buf->pos, bytes, ncpy);
		bytes += ncpy;
		buf->pos += ncpy;
		buf->len = buf->pos;
		nbytes -= ncpy;
		buf->dirty = true;
	}

	assert(nbytes == 0);

	return CBOR_ERR_OK;
}


static void buf_file_close(struct buf *buf)
{
	close(buf->fd);
}

static void buf_file_fill(struct buf *buf)
{
	buf->len = read(buf->fd, buf->buf, buf->bufsize);
	buf->eof = (buf->len == 0);
	TEMP_ASSERT(buf->len >= 0);
}


static void buf_file_flush(struct buf *buf)
{
	ssize_t written;

	written = write(buf->fd, buf->buf, buf->len);
	TEMP_ASSERT(written == buf->len);
}


static void buf_memory_close(struct buf *buf)
{
	cbor_free(buf->memory);
}


static void buf_memory_fill(struct buf *buf)
{
	size_t avail;
	size_t ncpy;
	
	avail = buf->memory_len - buf->memory_pos;
	ncpy = MIN(avail, buf->bufsize);

	memcpy(buf->buf, buf->memory + buf->memory_pos, ncpy);

	buf->len = ncpy;
	buf->memory_pos += ncpy;
}


static void buf_memory_flush(struct buf *buf)
{
	size_t new_mry_size;

	if (buf->memory_len + buf->len > buf->memory_size) {
		new_mry_size = MAX(2 * buf->memory_size, buf->bufsize);
		buf->memory = cbor_realloc(buf->memory, new_mry_size);
		buf->memory_size = new_mry_size;
	}

	memcpy(buf->memory + buf->memory_len, buf->buf, buf->len);
	buf->memory_len += buf->len;

	DEBUG_EXPR("%lu", buf->memory_len);
}


struct buf *buf_new(void)
{
	struct buf *buf;
	
	buf = cbor_malloc(sizeof(*buf));
	if (buf)
		buf_init(buf);

	return buf;
}


void buf_delete(struct buf *buf)
{
	buf_free(buf);
	cbor_free(buf);
}


cbor_err_t buf_open_file(struct buf *buf, char *filename, int flags, int mode)
{
	if ((buf->fd = open(filename, flags, mode)) == -1)
		return CBOR_ERR_NOFILE; /* TODO */

	buf->filename = filename;
	buf->mode = mode;

	buf->close = buf_file_close;
	buf->fill = buf_file_fill;
	buf->flush = buf_file_flush;

	return CBOR_ERR_OK;
}


cbor_err_t buf_open_memory(struct buf *buf)
{
	buf->memory = NULL;
	buf->memory_len = 0;
	buf->memory_size = 0;
	buf->memory_pos = 0;

	buf->close = buf_memory_close;
	buf->fill = buf_memory_fill;
	buf->flush = buf_memory_flush;

	return CBOR_ERR_OK;
}


void buf_close(struct buf *buf)
{
	if (buf->dirty)
		buf->flush(buf);

	if (buf->close)
		buf->close(buf);
}


bool buf_is_eof(struct buf *buf)
{
	assert(!buf->dirty);

	size_t avail;

	avail = buf->len - buf->pos;
	if (!avail) {
		buf->fill(buf);
		return buf->len == 0;
	}

	return false;
}
