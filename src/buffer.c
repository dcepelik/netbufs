#include "buffer.h"
#include "common.h"
#include "debug.h"
#include "memory.h"
#include "util.h"

#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define NB_DEBUG_THIS	0

static ssize_t write_internal(struct nb_buffer *buf, nb_byte_t *bytes, size_t nbytes);
extern ssize_t nb_buffer_read(struct nb_buffer *buf, nb_byte_t *bytes, size_t nbytes);
extern ssize_t nb_buffer_write(struct nb_buffer *buf, nb_byte_t *bytes, size_t nbytes);
extern int nb_buffer_getc(struct nb_buffer *buf);
extern void nb_buffer_ungetc(struct nb_buffer *buf, int c);
extern int nb_buffer_peek(struct nb_buffer *buf);


void nb_buffer_init(struct nb_buffer *buf)
{
	size_t size = 4 * 4096;

	buf->buf = nb_malloc(size);
	TEMP_ASSERT(buf->buf);

	buf->mode = BUF_MODE_IDLE;
	buf->bufsize = size;
	buf->pos = 0;
	buf->len = 0;
	buf->eof = false;
	buf->ungetc = -1;
	buf->written_total = 0;
}


void nb_buffer_flush(struct nb_buffer *buf)
{
	if (buf->mode == BUF_MODE_WRITING) {
		NB_DEBUG_TRACE;
		buf->ops->flush(buf);
	}

	buf->pos = 0;
	buf->len = 0;
	buf->mode = BUF_MODE_IDLE;
}


bool nb_buffer_fill(struct nb_buffer *buf)
{
	buf->ops->fill(buf);
	buf->pos = 0;
	return buf->len > 0;
}


/* TODO Check that: once EOF is returned for the first time, all successive calls return EOF as well */
static ssize_t read_internal(struct nb_buffer *buf, nb_byte_t *bytes, size_t nbytes)
{
	size_t avail;
	size_t ncpy;
	size_t read = 0;

	assert(buf->mode != BUF_MODE_WRITING);
	buf->mode = BUF_MODE_READING;
	buf->last_read_len = 0;

	while (nbytes > 0) {
		avail = buf->len - buf->pos;
		assert(avail >= 0);

		if (!avail) {
			nb_buffer_fill(buf);
			buf->mode = BUF_MODE_READING;
			avail = buf->len;

			NB_DEBUG_TRACE;

			if (avail == 0)
				break;
		}

		ncpy = MIN(avail, nbytes);
		memcpy(bytes, buf->buf + buf->pos, ncpy);
		bytes += ncpy;
		buf->pos += ncpy;
		read += ncpy;
		nbytes -= ncpy;
		buf->last_read_len += ncpy;
	}

	return read;
}


static inline nb_byte_t hexval(char c)
{
	assert(isxdigit(c));

	if (isdigit(c))
		return c - '0';
	else if (c >= 'a' && c <= 'f')
		return 10 + (c - 'a');
	else
		return 10 + (c - 'A');
}


static inline char hexchar(nb_byte_t val)
{
	assert(val >= 0 && val <= 15);

	if (val >= 0 && val <= 9)
		return '0' + val;
	else
		return 'A' + (val - 10);
}


ssize_t nb_buffer_read_slow(struct nb_buffer *buf, nb_byte_t *bytes, size_t nbytes)
{
	return read_internal(buf, bytes, nbytes);
}


ssize_t nb_buffer_write_slow(struct nb_buffer *buf, nb_byte_t *bytes, size_t nbytes)
{
	return write_internal(buf, bytes, nbytes);
}


static ssize_t write_internal(struct nb_buffer *buf, nb_byte_t *bytes, size_t nbytes)
{
	size_t avail;
	size_t ncpy;
	size_t written = 0;

	assert(buf->mode != BUF_MODE_READING);
	buf->mode = BUF_MODE_WRITING;

	while (nbytes > 0) {
		avail = buf->bufsize - buf->len;
		assert(avail >= 0);

		if (__builtin_expect(!avail, 0)) {
			nb_buffer_flush(buf);
			buf->mode = BUF_MODE_WRITING;
			avail = buf->bufsize;
		}

		ncpy = MIN(avail, nbytes);
		memcpy(buf->buf + buf->pos, bytes, ncpy);
		bytes += ncpy;
		buf->pos += ncpy;
		buf->written_total += ncpy;
		buf->len = buf->pos;
		written += ncpy;
		nbytes -= ncpy;
	}

	assert(nbytes == 0);

	return written;
}


bool nb_buffer_is_eof(struct nb_buffer *buf)
{
	assert(buf->mode != BUF_MODE_WRITING);

	size_t avail;

	avail = buf->len - buf->pos;
	if (!avail) {
		nb_buffer_fill(buf);
		return buf->len == 0;
	}

	return false;
}


size_t nb_buffer_tell(struct nb_buffer *buf)
{
	return buf->ops->tell(buf);
}


size_t nb_buffer_read_len(struct nb_buffer *buf, nb_byte_t *bytes, size_t nbytes)
{
	read_internal(buf, bytes, nbytes);
	return buf->last_read_len;
}


void nb_buffer_delete(struct nb_buffer *buf)
{
	if (buf->mode == BUF_MODE_WRITING)
		buf->ops->flush(buf);

	xfree(buf->buf);
	buf->ops->free(buf);
}


size_t nb_buffer_get_written_total(struct nb_buffer *buf)
{
	return buf->written_total;
}
